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
 * iteration 0.

 * Dummy Hashmap.
   - size is not any special number
   - linear hashing/probing by dummy 'modulo':
     (h1(k, m) + j)%m where h1(k, m) = k % m
   - operations performed on long long if uniwersum is not limited due to overflows possibility

   - Results:

     benchmark

     operations_number = 3800000, uniwersum_size = 1000000000
     Preprocess data
     Hashmap start watch
     Hashmap stop watch: Time = 829 ms.
     STL Map start watch
     STL Map stop watch: Time = 3404 ms.
     STL Unordered Map start watch
     STL Unordered Map stop watch: Time = 934 ms.
     Summary
     inserts = 1899892, members = 1900108, hits = 1860, stl hits = 1860, hashmap.size = 1898067, stl map size = 1898067
     hashmap.collisions = 51957874, colisions per insert = 27
     OK :)

   - Even dummy implementation (829 ms) is much better then std::map (3404 ms) and better then
     std::unordered_map (934 ms). Wow :)
   - colisions per insert = 27 when n/m = 1898067/2000000 ~ 95% on the end. Quite high.


 * iteration 1.

 * Still dummy hashmap with little change
   - in benchmark uniwersum is limited by 10^9 so long long arithmetic and conversions in h are useless

   - Results:

     benchmark

     operations_number = 3800000, uniwersum_size = 1000000000
     Preprocess data
     Hashmap start watch
     Hashmap stop watch: Time = 368 ms.
     STL Map start watch
     STL Map stop watch: Time = 3424 ms.
     STL Unordered Map start watch
     STL Unordered Map stop watch: Time = 862 ms.
     Summary
     inserts = 1897496, members = 1902504, hits = 1852, stl hits = 1852, hashmap.size = 1895666, stl map size = 1895666
     hashmap.collisions = 50350327, colisions per insert = 26
     OK :)

   - impressive. I hit hot spot. This little change give 2x speed up!

 * iteration 2.

 * Seems that hashmap size = prime number doesn't help at all.

 * iteration 3.
   - refactoring, Hashmap becomes tamplate with policy-based design.
     We have many different hash functions so it's necessary otherwise mess will happen.
     Limited_linear_hash is default now.
     After refactoring timings are the same.

 * iteration 4.
  - real_test_case_only_hashmap shows content of hashmap and gather statistics about islands
  - for limited_linear_hash_prime policy:

    hashmap capacity = 100000, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94762, members = 95238, hits = 4, hashmap.size = 94756
    hashmap.collisions = 2640501, colisions per insert = 27
    hashmap.islands = 3198, hashmap.sum = 94755, avg length = 29, max len = 2250

  - for limited_quadratic_hash policy:

    hashmap capacity = 100000, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94851, members = 95149, hits = 3, hashmap.size = 94848
    hashmap.collisions = 731920, colisions per insert = 7
    hashmap.islands = 4855, hashmap.sum = 94847, avg length = 19, max len = 352

    So distribution of elements for quadratic probing is much better then for linear -
    more much shorter islands, dumps confirms this.

  - Limited_linear_hash_prime vs Limited_quadratic_hash in benchmark.

    Thanks to much lower 'colisions per insert' for limited_quadratic_hash we have fastest hashmap so far:

    benchmark

    operations_number = 3800000, uniwersum_size = 1000000000
    Preprocess data
    Hashmap start watch
    Hashmap stop watch: Time = 296 ms.
    STL Map start watch
    STL Map stop watch: Time = 3029 ms.
    STL Unordered Map start watch
    STL Unordered Map stop watch: Time = 859 ms.
    Summary
    inserts = 1899862, members = 1900138, hits = 1831, stl hits = 1831, hashmap.size = 1897985, stl map size = 1897985
    hashmap.collisions = 14806907, colisions per insert = 7
    OK :)

    Now my hashmpa is ~10x faster then std::map and ~3x faster then std::unordered_map. WOW :)

  * iteration 5.
  - add extra Double_hash.

    hashmap capacity = 100003, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94814, members = 95186, hits = 3, hashmap.size = 94810
    hashmap.collisions = 759955, colisions per insert = 8
    hashmap.islands = 4925, hashmap.sum = 94810, avg length = 19, max len = 202

    In terms of islands: number of islands and maximal island length are slightly better then for quadratic hash.

  - real_test_case_theory_vs_practice.
    Scenario - freeze hashmap with arbitrary set alfa and perform many search operations.
            With record_hits = false we test case when key is NOT in dictionary (with very high probability).
            With record_hits = true we test case when key IS in dictionary (wth probability = 1).
    During tests we collect comparisions number and compare with theoretical expected comparisions number.

    Because comparisions = collisions+1 I had to do extra collisions++; on the end of process_search.

    KEY IS IN HASHMAP

    real_test_case_theory_vs_practice
    Linear alpha = 0.849985, quadratic alpha = 0.849985, double alpha = 0.849985
    Measured: Linear comparisions per search = 3.756040, quadratic comparisions per search = 2.469440,double comparisions per search = 2.493920
    Theory: Linear comparisions per search = 3.833334, quadratic comparisions per search = 0.000000,double comparisions per search = 2.231906

    real_test_case_theory_vs_practice
    Linear alpha = 0.949971, quadratic alpha = 0.949971, double alpha = 0.949971
    Measured: Linear comparisions per search = 10.854580, quadratic comparisions per search = 3.601860,double comparisions per search = 3.637720
    Theory: Linear comparisions per search = 10.499998, quadratic comparisions per search = 0.000000,double comparisions per search = 3.153402

    KEY IS NOT IN HASHMAP

    real_test_case_theory_vs_practice
    Linear alpha = 0.849975, quadratic alpha = 0.849975, double alpha = 0.849975
    Measured: Linear comparisions per search = 21.083759, quadratic comparisions per search = 7.843280,double comparisions per search = 8.182100
    Theory: Linear comparisions per search = 22.722229, quadratic comparisions per search = 0.000000,double comparisions per search = 6.666668

    real_test_case_theory_vs_practice
    Linear alpha = 0.949971, quadratic alpha = 0.949971, double alpha = 0.949971
    Measured: Linear comparisions per search = 149.973877, quadratic comparisions per search = 24.930901,double comparisions per search = 24.391979
    Theory: Linear comparisions per search = 200.499908, quadratic comparisions per search = 0.000000,double comparisions per search = 19.999996

    Results:
    1. Theoretical avaerage number of comparisions is very accurate in most cases.
    2. In such scenarios I don't see clear advantage Double_hash over Limited_quadratic_hash. From the other side Limited_quadratic_hash is much faster
       (no overflows = no long longs).
 */

namespace common
{

constexpr int INF {-1};

struct config
{
    int content;
    bool mark;

    bool is_empty()
    {
        return content == INF;
    }

    bool operator==(const config& cp)
    {
        return (content == cp.content);
    }
};// __attribute__((packed));

class Linear_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

    static int h(int k, int j, int m)
    {
        return int( ( (long long)(h1(k, m)) + (long long)(j)  )%m );
    }

    static int hash(config &c, int m)
    {
        return c.content % m;
    }
};

/*
 * in our benchmark uniwersum is limited by 10^9 so long long arithmetic and conversions in h are useless
 */
class Limited_linear_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

	static int h(int k, int j, int m)
	{
		return (h1(k, m) + j)%m;
	}

    static int hash(config &c, int m)
    {
        return c.content % m;
    }
};

constexpr int p {100003};
constexpr int a {5};
constexpr int b {7};

class Limited_linear_hash_prime final
{
public:
    static int h1(int x, int m)
    {
        return ((a*x + b) % p) % m;
    }

	static int h(int k, int j, int m)
	{
		return (h1(k, m) + j)%m;
	}

    static int hash(config &c, int m)
    {
        return c.content % m;
    }
};

/*
 */
class Limited_quadratic_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

	static int h(int k, int j, int m)
	{
		return (h1(k, m) + j + j*j)%m;
	}

    static int hash(config &c, int m)
    {
        return c.content % m;
    }
};

/*
 * Here it's required that m is prime number.
 */
class Double_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

    static int h2(int x, int m)
    {
        return 1 + x%(m-1);
    }

	static int h(int k, long long int j, int m)
	{
		return int( ( (long long)(h1(k, m)) + j*(long long)(h2(k, m))  )%m );
	}

    static int hash(config &c, int m)
    {
        return c.content % m;
    }
};

template<class Hash = Limited_quadratic_hash>
class Hashmap final
{
public:
    Hashmap(int size)
    {
        config c = {INF, false};
        table.assign(size, c);
    }

    void insert(config &c)
    {
        const int i = process_search(c, false);
        if (!(table[i] == c))
        {
            table[i] = c;
            n++;
        }
    }

    void erase(config &c)
    {
        const int i = process_search(c, true);
        if (table[i] == c)
        {
            c.mark = true;
            n--;
        }
    }

    bool member(config &c)
    {
        const int i = process_search(c, true);
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
        config c = {INF, false};
        std::fill(table.begin(), table.end(), c);
    }

    unsigned collisions {0};

protected:

    int process_search(config &c, bool ommit_marked)
    {
        const int m = table.size();
        const int hash_config = Hash::hash(c, m);
        int j = 0;
        int i = Hash::h(hash_config, j, m);

        while ( !(table[i] == c) && !table[i].is_empty())
        {
            // mark breaks
            if (!ommit_marked && table[i].mark)
                break;
            j++;
            i = Hash::h(hash_config, j, m);
            collisions++;
        }
        return i;
    }

    unsigned n {0};
public:
    std::vector<config> table;
};

}


namespace basics
{

common::Hashmap<> hashmap(500);
std::map<int, common::config> stl_map;

static inline char basic_get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void basic_test_case()
{
    printf("\n%s\n\n", __FUNCTION__);
    srand(time(nullptr));

    constexpr unsigned operations_number {100};
    constexpr unsigned uniwersum_size {100};
    unsigned inserts_counter {0};
    unsigned members_counter {0};
    constexpr bool debug {false};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};

    hashmap.reset();
    stl_map.clear();

    common::config basic_config;
    basic_config.mark = false;

    printf("operations_number = %d\n", operations_number);

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = basic_get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            stl_map[basic_config.content] = basic_config;

            inserts_counter++;

            if (debug)
                printf("insert = %d\n", basic_config.content);
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;

                members_counter++;

                if (debug)
                {
                    printf("member = %d, hit = %d\n", basic_config.content, hit);
                    printf("member = %d, hit = %d STL\n", basic_config.content,
                           (stl_iter != stl_map.end()));
                }
            }
    }
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d\n",
           inserts_counter, members_counter, members_hits, stl_members_hits);
}

}


namespace real_tests
{

common::Hashmap<> hashmap(100003);
std::map<int, common::config> stl_map;

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void real_test_case()
{
    constexpr bool debug {false};
    constexpr unsigned operations_number {190000};
    constexpr unsigned uniwersum_size {1000000000};

    unsigned inserts_counter {0};
    unsigned members_counter {0};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};

    hashmap.reset();
    stl_map.clear();

    assert(hashmap.size() == 0);
    assert(stl_map.size() == 0);

    common::config basic_config;
    basic_config.mark = false;

    std::vector<std::pair<int, int>> hashmap_log, stl_map_log;

    printf("\n%s\n\n", __FUNCTION__);
    printf("hashmap capacity = %u, operations_number = %u, uniwersum_size = %u\n",
           hashmap.capacity(), operations_number, uniwersum_size);

    srand(time(nullptr));

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            stl_map[basic_config.content] = basic_config;
            assert(stl_map.find(basic_config.content) != stl_map.end());

            assert(hashmap.size() == stl_map.size());
            inserts_counter++;

            if (debug)
                printf("insert = %d\n", basic_config.content);
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;

                members_counter++;
                hashmap_log.push_back({(int)basic_config.content, hit});
                stl_map_log.push_back({(int)basic_config.content, (stl_iter != stl_map.end())});
            }
    }

    if (debug)
    {
        printf("hashmap log\n");
        for (unsigned i = 0; i < hashmap_log.size(); i++)
            printf("member = %d, hit = %d\n", hashmap_log[i].first, hashmap_log[i].second);

        printf("STL log\n");
        for (unsigned i = 0; i < stl_map_log.size(); i++)
            printf("member = %d, hit = %d\n", stl_map_log[i].first, stl_map_log[i].second);
    }

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d, hashmap.size = %d, stl map size = %ld\n",
           inserts_counter, members_counter, members_hits, stl_members_hits,
           hashmap.size(), stl_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/inserts_counter));

    assert(members_hits == stl_members_hits);
    assert(hashmap.size() == stl_map.size());

    assert(hashmap_log.size() == stl_map_log.size());

    for (unsigned i = 0; i < hashmap_log.size(); i++)
    {
        assert(hashmap_log[i].first == stl_map_log[i].first);
        assert(hashmap_log[i].second == stl_map_log[i].second);
    }
    printf("OK :)\n");
}

}


namespace hashmap_tests
{

common::Hashmap<> hashmap(100003);

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void real_test_case_only_hashmap()
{
    constexpr unsigned operations_number {190000};
    constexpr unsigned uniwersum_size {1000000000};
    constexpr bool dump {false};

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};

    hashmap.reset();

    assert(hashmap.size() == 0);

    common::config basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("hashmap capacity = %u, operations_number = %u, uniwersum_size = %u\n",
           hashmap.capacity(), operations_number, uniwersum_size);

    srand(time(nullptr));

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                members_counter++;
            }
    }

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits, hashmap.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/inserts_counter));

    unsigned i = 0, number = 0, max_len = 0;
    uint64_t sum = 0;
    for (; i < hashmap.table.size();)
    {
        unsigned base = i;
        while ((i < hashmap.table.size()) && !hashmap.table[i++].is_empty());

        unsigned len = i-base-1;
        if (!hashmap.table[base].is_empty())
        {
            sum += len;
            number++;

            if (len > max_len)
                max_len = len;
        }
    }

    printf("hashmap.islands = %u, hashmap.sum = %lu, avg length = %lu, max len = %u\n", number, sum,
           (sum/number), max_len);

    if (dump)
    {
        for (unsigned j = 0; j < hashmap.table.size(); j++)
        {
            printf("%u", (!hashmap.table[j].is_empty()));
            if (j % 250 == 0)
                printf("\n");
        }
    }
    printf("OK :)\n");
}

static void real_test_case_theory_vs_practice(float alpha, bool record_hits)
{
    printf("\n%s\n\n", __FUNCTION__);

    constexpr unsigned capacity {100003};
    constexpr unsigned search_num {50000};
    constexpr unsigned uniwersum_size {1000000000};

    common::Hashmap<common::Limited_linear_hash> linear_hashmap(capacity);
    common::Hashmap<common::Limited_quadratic_hash> quadratic_hashmap(capacity);
    common::Hashmap<common::Double_hash> double_hashmap(capacity);

    common::config basic_config;
    basic_config.mark = false;
    std::vector<int> log_hits;

    srand(time(nullptr));

    // only inserts;
    unsigned inserts = (alpha*capacity*1.0f);
    for (unsigned i = 0; i < inserts; i++)
    {
        basic_config.content = (rand()%uniwersum_size);
        if (record_hits)
            log_hits.push_back(basic_config.content);
        linear_hashmap.insert(basic_config);
        quadratic_hashmap.insert(basic_config);
        double_hashmap.insert(basic_config);
    }

    printf("Linear alpha = %f, quadratic alpha = %f, double alpha = %f\n",
           (linear_hashmap.size()*1.0f/capacity),
           (quadratic_hashmap.size()*1.0f/capacity),
           (double_hashmap.size()*1.0f/capacity));

    // only search;Here collisions as comparisions number.
    linear_hashmap.collisions = 0;
    quadratic_hashmap.collisions = 0;
    double_hashmap.collisions = 0;
    unsigned members_hits = 0;
    for (unsigned i = 0; i < search_num; i++)
    {
        if (!record_hits)
            basic_config.content = (rand()%uniwersum_size);
        else
        {
            unsigned index = (rand()%log_hits.size());
            basic_config.content = log_hits[index];
        }
        bool hit = linear_hashmap.member(basic_config);
        if (hit)
            members_hits++;
        hit = quadratic_hashmap.member(basic_config);
        if (hit)
            members_hits++;
        hit = double_hashmap.member(basic_config);
        if (hit)
            members_hits++;
    }

    printf("Measured:\n");
    printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
           "double comparisions per search = %f\n",
           (linear_hashmap.collisions*1.0f/search_num),
           (quadratic_hashmap.collisions*1.0f/search_num),
           (double_hashmap.collisions*1.0f/search_num));
    printf("%u\n", members_hits);

    printf("Theory:\n");

    if (!record_hits)
    {
        const float linear_comp = 0.5f + 1.0f/(2.0f*(1.0f-alpha)*(1.0f-alpha));
        const float double_comp = 1.0f/(1.0f - alpha);
        printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
               "double comparisions per search = %f\n",
               linear_comp, 0.0f, double_comp);
    }
    else
    {
        const float linear_comp = 0.5f + 1.0f/(2.0f*(1.0f-alpha));
        const float double_comp = (-1.0f/alpha)*log(1.0f - alpha);
        printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
               "double comparisions per search = %f\n",
               linear_comp, 0.0f, double_comp);
    }
    printf(":)");
}

}


namespace benchmarks
{

common::Hashmap<> hashmap(2000003);
std::map<int, common::config> stl_map;
std::unordered_map<int, common::config> stl_unordered_map;

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

/* This benchmark test only I+M.
 *
 * TO DO:
    4. Choose the best one and use as reference to cuckoo hashing :)
    5. Delete?
 */
static void benchmark()
{
    constexpr unsigned operations_number {3800000};
    constexpr unsigned uniwersum_size {1000000000};

    unsigned inserts_counter {0};
    unsigned members_counter {0};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};
    unsigned stl_unordered_members_hits {0};

    hashmap.reset();
    stl_map.clear();
    stl_unordered_map.clear();

    assert(hashmap.size() == 0);
    assert(stl_map.size() == 0);
    assert(stl_unordered_map.size() == 0);

    common::config basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("operations_number = %u, uniwersum_size = %u\n", operations_number,
           uniwersum_size);

    printf("Preprocess data\n");
    srand(time(nullptr));
    std::vector<std::pair<char, int>> ops;
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();
        ops.push_back({operation, rand()%uniwersum_size});
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            assert(hashmap.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                members_counter++;
            }
    }
    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);


    printf("STL Map start watch\n");
    t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            stl_map[basic_config.content] = basic_config;
            assert(stl_map.find(basic_config.content) != stl_map.end());
            assert(stl_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;
                members_counter++;
            }
    }
    t1 = realtime_now();
    time_ms = (t1 - t0)/1000000;
    printf("STL Map stop watch: Time = %lu ms.\n", time_ms);


    printf("STL Unordered Map start watch\n");
    t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            stl_unordered_map[basic_config.content] = basic_config;
            assert(stl_unordered_map.find(basic_config.content) != stl_unordered_map.end());
            assert(stl_unordered_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                auto stl_iter = stl_unordered_map.find(basic_config.content);
                if (stl_iter != stl_unordered_map.end())
                    stl_unordered_members_hits++;
                members_counter++;
            }
    }
    t1 = realtime_now();
    time_ms = (t1 - t0)/1000000;
    printf("STL Unordered Map stop watch: Time = %lu ms.\n", time_ms);


    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d, hashmap.size = %d, stl map size = %ld\n",
           inserts_counter/3, members_counter/3, members_hits, stl_members_hits,
           hashmap.size(), stl_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/(inserts_counter/3)));

    assert(members_hits == stl_members_hits);
    assert(members_hits == stl_unordered_members_hits);
    assert(hashmap.size() == stl_map.size());
    assert(hashmap.size() == stl_unordered_map.size());

    printf("OK :)\n");
}

}

int main()
{
    basics::basic_test_case();
    hashmap_tests::real_test_case_theory_vs_practice(0.65f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.75f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.85f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.95f, false);

    hashmap_tests::real_test_case_theory_vs_practice(0.65f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.75f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.85f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.95f, true);

    real_tests::real_test_case();
    hashmap_tests::real_test_case_only_hashmap();
    benchmarks::benchmark();
    return 0;
}
