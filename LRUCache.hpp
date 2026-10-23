#pragma once

#include <chrono>
#include <cstddef>
#include <iterator>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

template <typename Key, typename Value>
class LRUCache
{
public:
    using Clock = std::chrono::steady_clock;
    using Millis = std::chrono::milliseconds;

    explicit LRUCache(size_t capacity) : capacity_(capacity)
    {
        if (capacity_ == 0)
            throw std::invalid_argument("capacity must be > 0");
    }

    LRUCache(const LRUCache &) = delete;
    LRUCache &operator=(const LRUCache &) = delete;

    void put(const Key &key, Value value, std::optional<Millis> ttl = std::nullopt)
    {
        std::unique_lock lock(mutex_);

        std::optional<Clock::time_point> expiry;
        if (ttl)
            expiry = Clock::now() + *ttl;

        auto it = map_.find(key);
        if (it != map_.end())
        {
            it->second->value = std::move(value);
            it->second->expiry = expiry;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }

        if (map_.size() >= capacity_)
            remove(std::prev(items_.end()));

        items_.push_front({key, std::move(value), expiry});
        map_[key] = items_.begin();
    }

    std::optional<Value> get(const Key &key)
    {
        std::unique_lock lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end())
            return std::nullopt;

        auto node = it->second;
        if (expired(*node))
        {
            remove(node);
            return std::nullopt;
        }

        items_.splice(items_.begin(), items_, node);
        return node->value;
    }

    std::optional<Value> peek(const Key &key) const
    {
        std::shared_lock lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end() || expired(*it->second))
            return std::nullopt;
        return it->second->value;
    }

    bool erase(const Key &key)
    {
        std::unique_lock lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end())
            return false;
        remove(it->second);
        return true;
    }

    size_t purge_expired()
    {
        std::unique_lock lock(mutex_);

        size_t count = 0;
        for (auto it = items_.begin(); it != items_.end();)
        {
            auto next = std::next(it);
            if (expired(*it))
            {
                remove(it);
                ++count;
            }
            it = next;
        }
        return count;
    }

    size_t size() const
    {
        std::shared_lock lock(mutex_);
        return map_.size();
    }

    size_t capacity() const { return capacity_; }

    void clear()
    {
        std::unique_lock lock(mutex_);
        map_.clear();
        items_.clear();
    }

private:
    struct Entry
    {
        Key key;
        Value value;
        std::optional<Clock::time_point> expiry;
    };
    using Iter = typename std::list<Entry>::iterator;

    static bool expired(const Entry &e)
    {
        return e.expiry && Clock::now() >= *e.expiry;
    }

    void remove(Iter it)
    {
        map_.erase(it->key);
        items_.erase(it);
    }

    size_t capacity_;
    std::list<Entry> items_;
    std::unordered_map<Key, Iter> map_;
    mutable std::shared_mutex mutex_;
};
