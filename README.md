# LRU Cache

Thread-safe LRU cache in C++17 with optional TTL, plus a sharded version to cut down on lock contention. It's header only, so you just include `LRUCache.hpp` or `ShardedCache.hpp`.

I built this to see how much a single lock actually slows a cache down when a bunch of threads are hitting it at once, and whether splitting it into shards fixes that.

```cpp
#include "ShardedCache.hpp"

ShardedCache<std::string, int> cache(10'000, 16);

cache.put("a", 1);
cache.put("session", 42, std::chrono::milliseconds(500));

if (auto v = cache.get("a")) {
    // *v == 1
}
```

## How it works

The cache is a linked list plus a hash map. The list keeps everything in order of when it was last used (most recent at the front), and the map points each key to its node in the list. That makes get and put both O(1): find the node through the map, then splice it to the front of the list. When the cache is full, the node at the back gets evicted.

Entries can also have a TTL. Expired entries don't get cleaned up on a timer. They get removed the next time someone calls `get` on them, or you can call `purge_expired()` to sweep everything at once.

For thread safety there's one `shared_mutex` around everything. `get` still has to take the exclusive lock since it moves the node to the front of the list, so really only `peek` (which doesn't change the order) and `size` can run at the same time.

`ShardedCache` is the fix for that. It's 16 separate LRU caches, each with its own lock, and a key goes to shard `hash(key) % 16`. Two threads only wait on each other if their keys land in the same shard. Each shard is also aligned to 64 bytes so two shards' locks never end up on the same cache line (false sharing). The downside is that eviction happens per shard, so it's only approximately LRU across the whole cache.

## Results

8 threads doing 250k operations each, 80% reads and 20% writes over 1000 keys, capacity 10k. I ran it on my laptop (Intel Core Ultra 9 185H) in WSL2 with a release build.

- single lock: 3-5M ops/s
- 16 shards: 10-14M ops/s

So roughly 3x faster with sharding. The exact numbers move around between runs, but the sharded version wins every time. I also ran the benchmark under ThreadSanitizer and it didn't find any data races.

## Building

You need CMake and a C++17 compiler (I used g++ in WSL). The tests use GoogleTest and only get built if CMake can find it.

```bash
cmake -B build
cmake --build build
./build/benchmark
./build/run_tests
```

To build with ThreadSanitizer:

```bash
cmake -B build-tsan -DUSE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tsan
./build-tsan/benchmark
```
