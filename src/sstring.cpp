#include <string.h>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <utility>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>

/*
  Motivation:

  * I need string which can be effective hashable ans stored when
    size/capacity are small (8B)
  * from the other side I sill want to have possibility to have big strings on heap
  * ideas borrowed from std::string, seastar::sstring and
    '"short continuation" optimization' for seastar::task_holder

  * specialization_proof_of_concept
    - depending on bool constexpr in default constructor sstring should behaves differently
    - init_content internally or externally
    - it's not enaugh to prepare

      template<bool T>
      void init_content() {}

      and specialize this inside sstring class because  explicit specialization in non-namespace scope
      is not allowed :(

    - fortunately Johannes trick with private overloading works.
      ref: https://stackoverflow.com/questions/3052579/explicit-specialization-in-non-namespace-scope

  * possibility of storing small (internal) strings and big (external) strings in only 8B is tricky
    (I need extra bit for recognition which situation we have). So I embedded this one extra bit inside
    pointer (I can do this on 64-bit linux - ref:  https://www.kernel.org/doc/Documentation/vm/pagemap.txt)

  * -fpack-struct is cool (forces attribute packed) but breaks code which was compiled without this flags (like libstdc++).
    With -fpack-struct I get many strange compilation errors from std::iostream and std::vector internals:

/usr/include/c++/6.3.1/bits/ios_base.h: In member function ‘std::ios_base::fmtflags std::ios_base::setf(std::ios_base::fmtflags)’:
/usr/include/c++/6.3.1/bits/ios_base.h:649:7: error: cannot bind packed field ‘((std::ios_base*)this)->std::ios_base::_M_flags’ to ‘std::_Ios_Fmtflags&’
       _M_flags |= __fmtfl;

/usr/include/c++/6.3.1/bits/ios_base.h: In member function ‘void*& std::ios_base::pword(int)’:
/usr/include/c++/6.3.1/bits/ios_base.h:834:21: error: cannot bind packed field ‘__word.std::ios_base::_Words::_M_pword’ to ‘void*&’
       return __word._M_pword;

    So in order to using it I removed iostreams and changed a bit hashmap (no vector::assign).

  * for only-moveable objects I can't do std::fill

  * iteration 6:

    sstring_benchmark__only_hashmap__iter2 vs sstring_benchmark__only_stl_unordered_map__iter2
    shows that in scenario with very huge number of searches when string is NOT in hashmap my hashmap + sstring
    is 5x faster then std::unordered_map + std::string.

  TO DO1: should I use malloc instead new? (Like in sstring.hh?) Any overhead?
  TO DO2: test for 4GB sstring ?
  TO DO3: some variadic helpers?
  TO DO4: hashing?
*/
namespace sstrings
{

template<const unsigned MaxSize>
class sstring final
{
public:
    using value_type = char;
    using reference = char&;
    using size_type = unsigned;

    sstring(const sstring &) = delete;
    sstring& operator=(const sstring &) = delete;
    sstring& operator=(const sstring &&) = delete;

    sstring()
    {
        constexpr auto internal {(MaxSize <= 7)};
        init_content<internal>();
    }

    sstring(const char (&input_cstring)[MaxSize])
    {
        constexpr auto internal {(MaxSize <= 7)};
        init_content<internal>(input_cstring);
    }

    // now it works only for internal strings
    sstring& operator=(sstring &&another) noexcept
    {
        content = another.content;
        another.init_content<true>();
        return *this;
    }

    // now it works only for internal strings
    sstring(sstring&& another) noexcept
    {
        content = another.content;
        another.init_content<true>();
    }

    char& operator[](unsigned pos) {
        return content.internal.buffer[pos];
    }

    bool is_internal() const
    {
        return (content.internal.size & 0x10);
    }

    // that's flat comparision and works only for internal (small) strings
    bool operator==(const sstring& another) const
    {
        return content.internal_for_cmp.value ==
                another.content.internal_for_cmp.value;
    }

    ~sstring()
    {
        if (!is_internal())
            delete[] content.external.buffer;
    }

private:
    union contents
    {
        struct internal_type
        {
            char buffer[7]; //0-55
            char size; //56-63, but size is 0..7 (4bits) so bits 56,57,58,59 are used and bit 60 is unused
            // from the other side from https://www.kernel.org/doc/Documentation/vm/pagemap.txt
            // in linux 64bit virtual address bits 57-60 zero are ALWAYS zero.
            // So finally I may use bit 60! It means I may use bit(4) in size ()
            // Bit(60) == 0 => external
            // Bit(60) == 1 => internal!
        } internal;
        struct internal_type_for_cmp
        {
            uint64_t value;
        } internal_for_cmp;
        struct external_type
        {
            char *buffer;
        } external;
        static_assert(sizeof(internal_type) == 8 && sizeof(external_type) == 8, "storage too big");
    } content;
    static_assert(sizeof(content) == 8, "storage is fucked up");

    template<bool T>
    struct is_internal_helper { constexpr static bool value = T; };

    template<bool T>
    void init_content(const char (&input_cstring)[MaxSize])
    {
        init_content(input_cstring, is_internal_helper<T>());
    }

    template<bool T>
    void init_content()
    {
        init_content(is_internal_helper<T>());
    }

    void init_content(is_internal_helper<true>)
    {
        static_assert(MaxSize <= 7, "input string is too big");
        content.internal.size = 0x10;
    }

    void init_content(is_internal_helper<false>)
    {
        static_assert(MaxSize > 7, "input string is too small");

        unsigned size {MaxSize};
        content.external.buffer = new char[MaxSize + 4];
        content.external.buffer[0] = *reinterpret_cast<char*>(&size);
    }

    void init_content(const char (&input_cstring)[MaxSize], is_internal_helper<true>)
    {
        static_assert(MaxSize <= 7, "input string is too big");
        std::memcpy(content.internal.buffer, input_cstring, MaxSize);
        content.internal.size = (MaxSize & 0xf) | 0x10;
    }

    void init_content(const char (&input_cstring)[MaxSize], is_internal_helper<false>)
    {
        static_assert(MaxSize > 7, "input string is too small");

        content.external.buffer = new char[MaxSize + 4];
        std::memcpy(content.external.buffer + 4, input_cstring, MaxSize);

        unsigned size {MaxSize};
        content.external.buffer[0] = *reinterpret_cast<char*>(&size);
    }
};

static void test_case()
{
    {
        char foo[5] {"foob"};
        sstring<5> str1(foo);
        assert(str1.is_internal());
    }
    {
        char foo[] {"baaz"};
        sstring<5> str1(foo);
        assert(str1.is_internal());
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str1(buf);
        assert(str1.is_internal());
    }
    {
        constexpr size_t size {sizeof("foobar")};
        sstring<size> str1("foobar");
        assert(str1.is_internal());
    }
    {
        sstring<5> str1;
        assert(str1.is_internal());
    }
    //    {
    //        char buf[] = "foobar";
    //        char *buf_ptr = buf;
    //        sstring<sizeof buf> str1(buf_ptr);
    //    }
    {
        char buf[] = "foobarr";
        sstring<sizeof buf> str1(buf);
        assert(!str1.is_internal());
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str(buf);
        str[0] = 'F';
        str[1] = 'O';
        assert(str[0] == 'F' && str[1] == 'O' && str[2] == 'o');
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str1(buf);
        assert(str1.is_internal());

        char buf2[] {"foo894hfnsdjknfsbar"};
        sstring<sizeof buf2> str2(buf2);
        assert(!str2.is_internal());
    }

    {
        std::array<sstring<7>, 123> strings;
        sstring<7> c;
        //std::fill(strings.begin(), strings.end(), c);
    }

    printf("%s ok\n", __FUNCTION__);
}

}

namespace specialization_proof_of_concept
{

template<bool T>
struct another { constexpr static bool value = T; };

template<const int MaxSize>
class sstring final
{
public:
    sstring(const char (&)[MaxSize])
    {
        constexpr bool internal = (MaxSize <= 7);
        init2<internal>();
    }

    template<bool T>
    void init2()
    {
        init2(another<T>());
    }

private:
    void init2(another<true>)
    {
        printf("Internal\n");
    }

    void init2(another<false>)
    {
        printf("External\n");
    }
};

static void test_case()
{
    char buf[] {"foobar"};
    sstring<sizeof buf> str1(buf);

    char buf2[] {"foo894hfnsdjknfsbar"};
    sstring<sizeof buf2> str2(buf2);
}

}

namespace hashing_benchmark
{

constexpr int INF {-1};

struct config
{
    sstrings::sstring<7> content;
    bool mark;

    bool is_empty()
    {
        return content[0] == INF;
    }

	bool operator==(config& cp)
	{
		for (int i = 0; i < 7; i++)
		if (content[i] != cp.content[i])
			return false;
		return true;
	}
//	bool operator==(const config& cp) const
//	{
//		return content == cp.content;
//	}
} __attribute__((packed));

class SStringHash final
{
public:

	static int h(int k, int j, int m)
	{
		return (k + j + j*j)%m;
	}

    static int hash(config &c, int m)
    {
        int result = 0;
        int mul = 1;
        for (int i = 0; i < 7; i++)
        {
            result = (result + (c.content[i]*mul) % m) % m ;
            mul *= 10;
        }
        return result;
    }

//    static int hash(config &c, int m)
//    {
//        int result = 0;
//        result = (result + (c.content[0]*1) % m) % m ;
//        result = (result + (c.content[1]*10) % m) % m ;
//        result = (result + (c.content[2]*100) % m) % m ;
//        result = (result + (c.content[3]*1000) % m) % m ;
//        result = (result + (c.content[4]*10000) % m) % m ;
//        result = (result + (c.content[5]*100000) % m) % m ;
//        result = (result + (c.content[6]*1000000) % m) % m;
//        return result;
//    }
};

//class Limited_quadratic_hash final
//{
//public:

//	static int h(int k, int j, int m)
//	{
//		return (k + j + j*j)%m;
//	}

//    static int hash(config &c, int m)
//    {
//        return c.content % m;
//    }
//};

template<class Hash = SStringHash>
class SStringHashmap final
{
public:
    SStringHashmap(int size)
    {
        config c;
        c.content[0] = INF;
        c.mark = false;
//        table.assign(size, c);
//        table.reserve(size);

//        std::memset(table.data(), 0, size*sizeof(config));
//        std::fill(table.begin(), table.end(), c);

        for (auto &e : table)
        {
            e.mark = false;
            e.content[0] = INF;
        }
    }

    void insert(config &c)
    {
        const int i = process_search__false(c);
        if (!(table[i] == c))
        {
            table[i] = std::move(c);
            n++;
        }
    }

    void erase(config &c)
    {
        const int i = process_search__true(c);
        if (table[i] == c)
        {
            c.mark = true;
            n--;
        }
    }

    bool member(config &c)
    {
        int i = process_search__true(c);
        return (table[i] == c);
    }

    unsigned size() const
    {
        return n;
    }

    unsigned capacity() const
    {
        return table.size();
    }

    void reset()
    {
        n = 0;
        //config c = {INF, false};
        config c;
        c.content[0] = INF;
        c.mark = false;
        //std::fill(table.begin(), table.end(), c);

        for (auto &e : table)
        {
            e.mark = false;
            e.content[0] = INF;
        }
    }

    unsigned collisions {0};

protected:

    int process_search__true(config &c)
    {
        const int m = table.size();
        const int hash_config = Hash::hash(c, m);
        int j = 0;
        int i = Hash::h(hash_config, j, m);

        while ( !(table[i] == c) && (table[i].content[0] != INF))
        {
            j++;
            i = Hash::h(hash_config, j, m);
            collisions++;
        }
        return i;
    }

    int process_search__false(config &c)
    {
        const int m = table.size();
        const int hash_config = Hash::hash(c, m);
        int j = 0;
        int i = Hash::h(hash_config, j, m);

        while ( !(table[i] == c) && (table[i].content[0] != INF) && !table[i].mark)
        {
            j++;
            i = Hash::h(hash_config, j, m);
            collisions++;
        }
        return i;
    }

    unsigned n {0};
public:
    std::array<config, 2000003> table;
};

static sstrings::sstring<7> rand_sstring(unsigned)
{
    sstrings::sstring<7> result;
    for (unsigned i = 0; i < 7; i++)
        result[i] = rand()%128;
    return result;
}

static std::string rand_string(unsigned max_size)
{
    std::string result(max_size, ' ');
    for (unsigned i = 0; i < result.size(); i++)
        result[i] = rand()%128;
    return result;
}

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

static inline uint64_t realtime_now()
{
	struct timespec now_ts;
	clock_gettime(CLOCK_REALTIME, &now_ts);
	return TIMESPEC_NSEC(&now_ts);
}

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

/*
 * std::unordered_map<std::string, std::string>:
 * I get 1380 ms

 * SStringHashmap:
 * With sizeof(config) = 16 + std::vector I get ~640ms

 * SStringHashmap:
 * With sizeof(config) = 9 + static + std::array I get ~390ms
   That's probably because of changing vector -> static std::array but I need to verify

 * SStringHashmap:
   + improved operator== I get sometimes 380 ms (610M -> 570M branches)

   So now my SStringHashmap is ~4x faster then unordered_map with string

 */
//static void sstring_benchmark__only_hashmap()
//{
//    static SStringHashmap<> hash_map(2000003);
//    constexpr unsigned operations_number {3800000};
//    constexpr unsigned string_max_size {7};

//    unsigned inserts_counter {0};
//    unsigned members_counter {0};
//    unsigned members_hits {0};
//    hash_map.reset();

//    config basic_config;
//    basic_config.mark = false;

//    printf("\n%s\n\n", __FUNCTION__);
//    printf("sizeof(config) = %zu, hashmap.capacity() = %u, operations_number = %u, string_max_size = %u\n",
//           sizeof(basic_config), hash_map.capacity(), operations_number, string_max_size);

//    printf("Preprocess data\n");
//    srand(time(nullptr));
//    std::vector<std::pair<char, sstrings::sstring<7>>> ops;
//    for (unsigned i = 0; i < operations_number; i++)
//    {
//        const char operation = get_operation();
//        ops.push_back({operation, rand_sstring(string_max_size)});
//    }

//    printf("Hashmap start watch\n");
//    uint64_t t0 = realtime_now();
//    for (unsigned i = 0; i < operations_number; i++)
//    {
//        const char operation = ops[i].first;

//        if (operation == 'I')
//        {
//            basic_config.content = ops[i].second;
//            hash_map.insert(basic_config);
//            assert(hash_map.member(basic_config));
//            assert(hash_map.size() > 0);
//            inserts_counter++;
//        }
//        else
//            if (operation == 'M')
//            {
//                basic_config.content = ops[i].second;
//                bool hit = hash_map.member(basic_config);
//                if (hit)
//                    members_hits++;
//                members_counter++;
//            }
//    }
//    uint64_t t1 = realtime_now();
//    uint64_t time_ms = (t1 - t0)/1000000;
//    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

//    printf("Summary\n");
//    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
//           inserts_counter, members_counter, members_hits,
//           hash_map.size());
//    printf("hashmap.collisions = %d, colisions per insert = %d\n", hash_map.collisions,
//           (hash_map.collisions/(inserts_counter)));

//    printf("OK :)\n");
//}

static void sstring_benchmark__only_stl_unordered_map()
{
    std::unordered_map<std::string, std::string> stl_unordered_map;
    constexpr unsigned operations_number {3800000};
    constexpr unsigned string_max_size {7};

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    unsigned stl_unordered_members_hits {0};
    stl_unordered_map.clear();

    std::string basic_config;

    printf("\n%s\n\n", __FUNCTION__);

    printf("Preprocess data\n");
    srand(time(nullptr));
    std::vector<std::pair<char, std::string>> ops;
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();
        ops.push_back({operation, rand_string(string_max_size)});
    }

    printf("STL Unordered Map start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config = ops[i].second;
            stl_unordered_map[basic_config] = basic_config;
            assert(stl_unordered_map.find(basic_config) != stl_unordered_map.end());
            assert(stl_unordered_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config = ops[i].second;
                auto stl_iter = stl_unordered_map.find(basic_config);
                if (stl_iter != stl_unordered_map.end())
                    stl_unordered_members_hits++;
                members_counter++;
            }
    }
    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("STL Unordered Map stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %zu\n",
           inserts_counter, members_counter, members_hits,
           stl_unordered_map.size());
    printf("OK :)\n");
}

/* So now my SStringHashmap in iter2 benchmark is ~5x faster then unordered_map with string
 *
 * OK but I have only 10% filled :)
 */

static void sstring_benchmark__only_hashmap__iter2()
{
    static SStringHashmap<> hash_map(2000003);

    constexpr unsigned string_max_size {7};

    constexpr unsigned inserts = 190000;
    constexpr unsigned fixed_members = 1024;
    constexpr unsigned queries = 60000000;

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    hash_map.reset();

    config basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("sizeof(config) = %zu, hashmap.capacity() = %u, inserts = %u, string_max_size = %u, queries = %u\n",
           sizeof(basic_config), hash_map.capacity(), inserts, string_max_size, queries);

    printf("Preprocess data\n");
    srand(time(nullptr));

    std::vector<config> members;
    for (unsigned i = 0; i < inserts; i++)
    {
        basic_config.mark = false;
        basic_config.content = rand_sstring(7);
        hash_map.insert(basic_config);
        inserts_counter++;
    }

    for (unsigned i = 0; i < fixed_members; i++)
    {
        basic_config.mark = false;
        basic_config.content = rand_sstring(7);
        members.emplace_back(std::move(basic_config));
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < queries; i++)
    {
        members_hits += hash_map.member(members[i%fixed_members]);
    }

    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits,
           hash_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hash_map.collisions,
           (hash_map.collisions/(inserts_counter + queries)));

    printf("OK :)\n");
}

static void sstring_benchmark__only_stl_unordered_map__iter2()
{
    std::unordered_map<std::string, std::string> stl_unordered_map;

    constexpr unsigned inserts = 190000;
    constexpr unsigned fixed_members = 1024;
    constexpr unsigned queries = 60000000;

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    stl_unordered_map.clear();

    printf("\n%s\n\n", __FUNCTION__);

    printf("Preprocess data\n");
    srand(time(nullptr));
    for (unsigned i = 0; i < inserts; i++)
    {
        auto str = rand_string(7);
        stl_unordered_map[str] = str;
        inserts_counter++;
    }

    std::vector<std::string> members;
    for (unsigned i = 0; i < fixed_members; i++)
    {
        members.push_back(rand_string(7));
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < queries; i++)
    {
        auto string = members[i%fixed_members];
        members_hits += (stl_unordered_map.find(string) != stl_unordered_map.end());
    }

    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits,
           stl_unordered_map.size());
    printf("OK :)\n");
}

}


int main()
{
    specialization_proof_of_concept::test_case();
    sstrings::test_case();
    hashing_benchmark::sstring_benchmark__only_hashmap__iter2();
    //hashing_benchmark::sstring_benchmark__only_stl_unordered_map__iter2();
    return 0;
}
