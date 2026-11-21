#pragma once
#include "EvictionPolicy.h"
#include <chrono>
#include <queue>
#include <vector>
#include <unordered_map>
#include <memory>

// Decorator adding time-based expiry to any EvictionPolicy.
//
// A min-heap keyed by expiry time yields the next entry to expire in
// O(log n). std::priority_queue can't remove arbitrary items, so deletion is
// lazy: each heap entry carries a generation number, and re-putting or
// erasing a key bumps generation_[key]. Popped entries with an outdated
// generation are stale and simply discarded.
template <typename K, typename V>
class TTLCache : public EvictionPolicy<K, V> {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct HeapEntry {
        TimePoint expireAt;
        K key;
        uint64_t generation;
        bool operator>(const HeapEntry& other) const {
            return expireAt > other.expireAt;
        }
    };

public:
    // defaultTTL applies to put(); use putWithTTL() to override per entry.
    TTLCache(std::unique_ptr<EvictionPolicy<K, V>> underlying,
             std::chrono::milliseconds defaultTTL)
        : underlying_(std::move(underlying)), defaultTTL_(defaultTTL) {}

    std::optional<V> get(const K& key) override {
        purgeExpired();
        return underlying_->get(key);
    }

    void put(const K& key, const V& value) override {
        putWithTTL(key, value, defaultTTL_);
    }

    void putWithTTL(const K& key, const V& value, std::chrono::milliseconds ttl) {
        purgeExpired();
        underlying_->put(key, value);
        uint64_t gen = ++generation_[key];
        TimePoint expireAt = Clock::now() + ttl;
        expiry_[key] = expireAt;
        heap_.push(HeapEntry{expireAt, key, gen});
    }

    bool erase(const K& key) override {
        expiry_.erase(key);
        ++generation_[key];
        return underlying_->erase(key);
    }

    // Doesn't purge, since purging would mutate state in a const method.
    bool contains(const K& key) const override {
        auto it = expiry_.find(key);
        if (it != expiry_.end() && it->second <= Clock::now()) return false;
        return underlying_->contains(key);
    }

    // May include expired entries not yet lazily purged; call expireNow() first for an exact count.
    std::size_t size() const override {
        return underlying_->size();
    }

    std::size_t capacity() const override { return underlying_->capacity(); }

    std::optional<K> peekEvictionCandidate() const override {
        return underlying_->peekEvictionCandidate();
    }

    // Purges all currently expired entries, e.g. from a periodic sweep.
    std::size_t expireNow() { return purgeExpired(); }

    std::size_t ttlQueueSizeForTesting() const { return heap_.size(); }

private:
    std::size_t purgeExpired() {
        std::size_t purged = 0;
        TimePoint now = Clock::now();
        while (!heap_.empty() && heap_.top().expireAt <= now) {
            HeapEntry top = heap_.top();
            heap_.pop();

            auto genIt = generation_.find(top.key);
            bool isStale = (genIt == generation_.end() || genIt->second != top.generation);
            if (isStale) continue;

            underlying_->erase(top.key);
            expiry_.erase(top.key);
            ++purged;
        }
        return purged;
    }

    std::unique_ptr<EvictionPolicy<K, V>> underlying_;
    std::chrono::milliseconds defaultTTL_;
    std::unordered_map<K, TimePoint> expiry_;
    std::unordered_map<K, uint64_t> generation_;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>> heap_;
};
