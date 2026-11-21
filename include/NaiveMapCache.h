#pragma once
#include "EvictionPolicy.h"
#include <map>
#include <stdexcept>
#include <cstdint>

// Baseline for BENCHMARKS.md: a std::map cache that evicts the oldest
// insertion via an O(n) scan. get() is O(log n); put() is O(log n) plus an
// O(n) scan on every eviction, which LRUCache's linked list exists to avoid.
template <typename K, typename V>
class NaiveMapCache : public EvictionPolicy<K, V> {
    struct Entry {
        V value;
        uint64_t insertedAt;
    };

public:
    explicit NaiveMapCache(std::size_t capacity) : capacity_(capacity), clock_(0) {
        if (capacity_ == 0) {
            throw std::invalid_argument("NaiveMapCache capacity must be > 0");
        }
    }

    // Reads don't update recency, so eviction behaves like FIFO rather than LRU.
    std::optional<V> get(const K& key) override {
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        return it->second.value;
    }

    void put(const K& key, const V& value) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second.value = value;
            return;
        }
        if (map_.size() >= capacity_) {
            evictOldest();
        }
        map_[key] = Entry{value, clock_++};
    }

    bool erase(const K& key) override { return map_.erase(key) > 0; }
    bool contains(const K& key) const override { return map_.find(key) != map_.end(); }
    std::size_t size() const override { return map_.size(); }
    std::size_t capacity() const override { return capacity_; }

    std::optional<K> peekEvictionCandidate() const override {
        if (map_.empty()) return std::nullopt;
        auto oldest = map_.begin();
        for (auto it = map_.begin(); it != map_.end(); ++it) {
            if (it->second.insertedAt < oldest->second.insertedAt) oldest = it;
        }
        return oldest->first;
    }

private:
    void evictOldest() {
        auto victim = map_.begin();
        for (auto it = map_.begin(); it != map_.end(); ++it) {
            if (it->second.insertedAt < victim->second.insertedAt) victim = it;
        }
        map_.erase(victim);
    }

    std::size_t capacity_;
    uint64_t clock_;
    std::map<K, Entry> map_;
};
