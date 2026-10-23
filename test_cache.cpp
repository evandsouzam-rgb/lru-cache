#include "LRUCache.hpp"
#include "ShardedCache.hpp"

#include <chrono>
#include <string>
#include <thread>

#include <gtest/gtest.h>

TEST(LRUCache, PutGet)
{
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_EQ(cache.get(1), "one");
    EXPECT_EQ(cache.get(2), "two");
    EXPECT_FALSE(cache.get(3));
}

TEST(LRUCache, EvictsLeastRecent)
{
    LRUCache<int, int> cache(2);
    cache.put(1, 100);
    cache.put(2, 200);
    cache.get(1);
    cache.put(3, 300);

    EXPECT_TRUE(cache.get(1));
    EXPECT_FALSE(cache.get(2));
    EXPECT_EQ(cache.get(3), 300);
}

TEST(LRUCache, PeekDoesNotUpdateOrder)
{
    LRUCache<int, int> cache(2);
    cache.put(1, 100);
    cache.put(2, 200);
    cache.peek(1);
    cache.put(3, 300);

    EXPECT_FALSE(cache.get(1));
    EXPECT_TRUE(cache.get(2));
}

TEST(LRUCache, Expiry)
{
    LRUCache<int, std::string> cache(5);
    cache.put(1, "temp", std::chrono::milliseconds(50));
    cache.put(2, "forever");
    EXPECT_EQ(cache.get(1), "temp");

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    EXPECT_EQ(cache.purge_expired(), 1u);
    EXPECT_FALSE(cache.get(1));
    EXPECT_EQ(cache.get(2), "forever");
}

TEST(ShardedCache, PutGet)
{
    ShardedCache<int, std::string> cache(100, 4);
    cache.put(10, "apple");
    cache.put(20, "banana");

    EXPECT_EQ(cache.get(10), "apple");
    EXPECT_EQ(cache.get(20), "banana");
    EXPECT_FALSE(cache.get(99));
    EXPECT_EQ(cache.size(), 2u);
}
