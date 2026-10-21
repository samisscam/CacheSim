#pragma once
#include "EvictionPolicy.h"
#include <memory>
#include <utility>

// Policy-agnostic facade that forwards every call to its eviction policy.
//
//   Cache<std::string, int> cache(std::make_unique<LRUCache<std::string, int>>(100));
template <typename K, typename V>
class Cache {
public:
    explicit Cache(std::unique_ptr<EvictionPolicy<K, V>> policy)
        : policy_(std::move(policy)) {}

    std::optional<V> get(const K& key) { return policy_->get(key); }
    void put(const K& key, const V& value) { policy_->put(key, value); }
    bool erase(const K& key) { return policy_->erase(key); }
    bool contains(const K& key) const { return policy_->contains(key); }
    std::size_t size() const { return policy_->size(); }
    std::size_t capacity() const { return policy_->capacity(); }
    std::optional<K> peekEvictionCandidate() const { return policy_->peekEvictionCandidate(); }

    // Existing entries are not migrated to the new policy.
    void setPolicy(std::unique_ptr<EvictionPolicy<K, V>> policy) {
        policy_ = std::move(policy);
    }

private:
    std::unique_ptr<EvictionPolicy<K, V>> policy_;
};
