#pragma once
#include "EvictionPolicy.h"
#include <mutex>
#include <memory>

// Decorator making any EvictionPolicy thread-safe with a single coarse-grained
// mutex. Simple and correct, but it serializes all access; see ShardedCache
// and the README for the scaling trade-offs.
template <typename K, typename V>
class ThreadSafeCache : public EvictionPolicy<K, V> {
public:
    explicit ThreadSafeCache(std::unique_ptr<EvictionPolicy<K, V>> underlying)
        : underlying_(std::move(underlying)) {}

    std::optional<V> get(const K& key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->get(key);
    }

    void put(const K& key, const V& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        underlying_->put(key, value);
    }

    bool erase(const K& key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->erase(key);
    }

    bool contains(const K& key) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->contains(key);
    }

    std::size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->size();
    }

    std::size_t capacity() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->capacity();
    }

    std::optional<K> peekEvictionCandidate() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return underlying_->peekEvictionCandidate();
    }

private:
    std::unique_ptr<EvictionPolicy<K, V>> underlying_;
    mutable std::mutex mutex_;
};
