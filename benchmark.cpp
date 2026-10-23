#include "LRUCache.hpp"
#include "ShardedCache.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono;

void sanity_check()
{
    LRUCache<int, std::string> cache(2);

    cache.put(1, "a");
    cache.put(2, "b");
    assert(cache.get(1) == "a");

    cache.put(3, "c");
    assert(!cache.get(2));
    assert(cache.get(1) == "a");
    assert(cache.get(3) == "c");

    cache.put(4, "d", milliseconds(50));
    assert(cache.get(4) == "d");
    std::this_thread::sleep_for(milliseconds(60));
    assert(!cache.get(4));
}

template <typename Cache>
void bench(const char *name, Cache &cache, int threads, int ops, double read_ratio)
{
    auto start = steady_clock::now();

    std::vector<std::thread> workers;
    for (int t = 0; t < threads; t++)
    {
        workers.emplace_back([&, t] {
            std::mt19937 rng(1337 + t);
            std::uniform_int_distribution<int> key(1, 1000);
            std::uniform_real_distribution<double> coin(0, 1);

            for (int i = 0; i < ops; i++)
            {
                int k = key(rng);
                if (coin(rng) < read_ratio)
                    cache.get(k);
                else
                    cache.put(k, k * 10);
            }
        });
    }
    for (auto &w : workers)
        w.join();

    double secs = duration<double>(steady_clock::now() - start).count();
    long total = (long)threads * ops;
    std::cout << name << ": " << total << " ops in " << secs << "s, "
              << (long)(total / secs) << " ops/s\n";
}

int main()
{
    sanity_check();

    int threads = 8;
    int ops = 250'000;
    double read_ratio = 0.8;

    LRUCache<int, int> single(10'000);
    bench("single lock", single, threads, ops, read_ratio);

    ShardedCache<int, int> sharded(10'000, 16);
    bench("16 shards  ", sharded, threads, ops, read_ratio);
}
