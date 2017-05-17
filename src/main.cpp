#include <cstdio>
#include <utility>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

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

 */

constexpr int INF = -1;

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

    void reset()
    {
        n = 0;
        config c = {INF, false};
        std::fill(table.begin(), table.end(), c);
    }

    unsigned collisions {0};

protected:
    static int h1(int x, int m)
    {
        return x % m;
    }

    static int h2(int)
    {
        return 1;
    }

	// linear hashing
	static int h(int k, int j, int m)
	{
		//assert(0 <= k && k <= 1000000000);
		//assert(0 <= j && j <= 1000000000);
		//assert(0 <= m && m <= 1000000000);
		//assert(0 <= h1(k, m) && h1(k, m) <= 1000000000);
		return (h1(k, m) + j)%m;
	}

    static int hash(config &c, int m)
    {
        return c.content % m;
    }

    int process_search(config &c, bool ommit_marked)
    {
        const int m = table.size();
        const int hash_config = hash(c, m);
        int j = 0;
        int i = h(hash_config, j, m);

        while ( !(table[i] == c) && !table[i].is_empty())
        {
            // mark breaks
            if (!ommit_marked && table[i].mark)
                break;
            j++;
            i = h(hash_config, j, m);
            collisions++;
        }
        return i;
    }

    unsigned n {0};
    std::vector<config> table;
};


namespace basics
{

Hashmap hashmap(500);
std::map<int, config> stl_map;

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

    unsigned members_hits {0};
    unsigned stl_members_hits {0};

    hashmap.reset();
    stl_map.clear();

    config basic_config;
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

                printf("member = %d, hit = %d\n", basic_config.content, hit);
                printf("member = %d, hit = %d STL\n", basic_config.content,
                       (stl_iter != stl_map.end()));
            }
    }
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d\n",
           inserts_counter, members_counter, members_hits, stl_members_hits);
}

}


namespace real_tests
{

Hashmap hashmap(100000);
std::map<int, config> stl_map;

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

    config basic_config;
    basic_config.mark = false;

    std::vector<std::pair<int, int>> hashmap_log, stl_map_log;

    printf("\n%s\n\n", __FUNCTION__);
    printf("operations_number = %u, uniwersum_size = %u\n", operations_number,
           uniwersum_size);

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

namespace benchmarks
{

Hashmap hashmap(2000000);
std::map<int, config> stl_map;
std::unordered_map<int, config> stl_unordered_map;

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
    1. add stop watch
    2. add unordered_map and compare
    3. compare timings/collisions number for many different hash functions/primes/
       optimisation techniques
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

    config basic_config;
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
    real_tests::real_test_case();
    benchmarks::benchmark();
    return 0;
}
