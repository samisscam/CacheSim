#pragma once
#include "EvictionPolicy.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

// Decorator that adds time-based expiry to any EvictionPolicy.
//
// A min-heap ordered by expiry time finds the next entry to expire in
// O(log n). Since std::priority_queue can't remove arbitrary items, stale
// heap entries (from overwritten or erased keys) are skipped lazily using a
// per-key generation counter.
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

    bool contains(const K& key) const override {
        auto it = expiry_.find(key);
        if (it != expiry_.end() && it->second <= Clock::now()) return false;
        return underlying_->contains(key);
    }

    // May include expired entries that haven't been purged yet.
    std::size_t size() const override { return underlying_->size(); }

    std::size_t capacity() const override { return underlying_->capacity(); }

    std::optional<K> peekEvictionCandidate() const override {
        return underlying_->peekEvictionCandidate();
    }

    // Purges all expired entries and returns how many were removed.
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
