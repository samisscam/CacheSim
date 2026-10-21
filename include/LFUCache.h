#pragma once
#include "EvictionPolicy.h"
#include <list>
#include <stdexcept>
#include <unordered_map>

// LFU eviction with O(1) get/put using frequency buckets.
//
// Each bucket holds the entries at one frequency, ordered by recency
// (front = most recent). Ties on frequency are broken by evicting the
// least recently used entry. minFreq_ tracks the lowest non-empty bucket
// so eviction is a pop_back on that bucket.
template <typename K, typename V>
class LFUCache : public EvictionPolicy<K, V> {
    struct Entry {
        K key;
        V value;
        int freq;
        Entry(const K& k, const V& v, int f) : key(k), value(v), freq(f) {}
    };
    using BucketList = std::list<Entry>;
    using BucketIter = typename BucketList::iterator;

public:
    explicit LFUCache(std::size_t capacity) : capacity_(capacity), minFreq_(0) {
        if (capacity_ == 0) {
            throw std::invalid_argument("LFUCache capacity must be > 0");
        }
    }

    std::optional<V> get(const K& key) override {
        auto it = keyLocator_.find(key);
        if (it == keyLocator_.end()) return std::nullopt;
        touch(it->second);
        return it->second->value;
    }

    void put(const K& key, const V& value) override {
        auto it = keyLocator_.find(key);
        if (it != keyLocator_.end()) {
            it->second->value = value;
            touch(it->second);
            return;
        }

        if (keyLocator_.size() >= capacity_) {
            evictLFU();
        }

        freqBuckets_[1].emplace_front(key, value, 1);
        keyLocator_[key] = freqBuckets_[1].begin();
        minFreq_ = 1;
    }

    bool erase(const K& key) override {
        auto it = keyLocator_.find(key);
        if (it == keyLocator_.end()) return false;
        int f = it->second->freq;
        freqBuckets_[f].erase(it->second);
        if (freqBuckets_[f].empty()) {
            freqBuckets_.erase(f);
            if (minFreq_ == f) minFreq_ = recomputeMinFreq();
        }
        keyLocator_.erase(it);
        return true;
    }

    bool contains(const K& key) const override {
        return keyLocator_.find(key) != keyLocator_.end();
    }

    std::size_t size() const override { return keyLocator_.size(); }
    std::size_t capacity() const override { return capacity_; }

    std::optional<K> peekEvictionCandidate() const override {
        if (keyLocator_.empty()) return std::nullopt;
        auto bucketIt = freqBuckets_.find(minFreq_);
        return bucketIt->second.back().key;
    }

private:
    // Moves an entry to the front of the next-higher frequency bucket.
    void touch(BucketIter& it) {
        int f = it->freq;
        Entry moved = *it;
        moved.freq = f + 1;

        freqBuckets_[f].erase(it);
        if (freqBuckets_[f].empty()) {
            freqBuckets_.erase(f);
            if (minFreq_ == f) minFreq_ = f + 1;
        }

        freqBuckets_[f + 1].emplace_front(moved);
        it = freqBuckets_[f + 1].begin();
        keyLocator_[moved.key] = it;
    }

    void evictLFU() {
        auto bucketIt = freqBuckets_.find(minFreq_);
        if (bucketIt == freqBuckets_.end() || bucketIt->second.empty()) return;
        const K victimKey = bucketIt->second.back().key;
        bucketIt->second.pop_back();
        if (bucketIt->second.empty()) {
            freqBuckets_.erase(bucketIt);
        }
        keyLocator_.erase(victimKey);
    }

    // Only needed when erase() empties the minimum-frequency bucket.
    int recomputeMinFreq() const {
        if (keyLocator_.empty()) return 0;
        int best = -1;
        for (const auto& [f, bucket] : freqBuckets_) {
            if (!bucket.empty() && (best == -1 || f < best)) best = f;
        }
        return best;
    }

    std::size_t capacity_;
    int minFreq_;
    std::unordered_map<int, BucketList> freqBuckets_;
    std::unordered_map<K, BucketIter> keyLocator_;
};
