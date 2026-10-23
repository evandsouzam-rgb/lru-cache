#pragma once

#include "LRUCache.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ShardedCache
{
public:
    using Millis = std::chrono::milliseconds;

    explicit ShardedCache(size_t capacity, size_t num_shards = 16)
    {
        num_shards = std::max<size_t>(num_shards, 1);
        size_t per_shard = std::max<size_t>(capacity / num_shards, 1);
        for (size_t i = 0; i < num_shards; ++i)
            shards_.push_back(std::make_unique<Shard>(per_shard));
    }

    void put(const Key &key, Value value, std::optional<Millis> ttl = std::nullopt)
    {
        shard(key).put(key, std::move(value), ttl);
    }

    std::optional<Value> get(const Key &key) { return shard(key).get(key); }
    std::optional<Value> peek(const Key &key) const { return shard(key).peek(key); }
    bool erase(const Key &key) { return shard(key).erase(key); }

    size_t purge_expired()
    {
        size_t count = 0;
        for (auto &s : shards_)
            count += s->purge_expired();
        return count;
    }

    size_t size() const
    {
        size_t total = 0;
        for (auto &s : shards_)
            total += s->size();
        return total;
    }

private:
    struct alignas(64) Shard : LRUCache<Key, Value>
    {
        using LRUCache<Key, Value>::LRUCache;
    };

    Shard &shard(const Key &key) const
    {
        return *shards_[Hash{}(key) % shards_.size()];
    }

    std::vector<std::unique_ptr<Shard>> shards_;
};
