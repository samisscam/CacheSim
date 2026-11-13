#pragma once
#include <optional>
#include <cstddef>

// Strategy interface implemented by every eviction policy.
// capacity() is fixed for the lifetime of the policy, and get() counts as a
// "touch" (same recency/frequency side effects as a hit).
template <typename K, typename V>
class EvictionPolicy {
public:
    virtual ~EvictionPolicy() = default;

    // Returns std::nullopt on a miss.
    virtual std::optional<V> get(const K& key) = 0;

    // Inserts or updates. If size() would exceed capacity(), evicts exactly one entry.
    virtual void put(const K& key, const V& value) = 0;

    // No-op if the key is absent.
    virtual bool erase(const K& key) = 0;

    virtual bool contains(const K& key) const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t capacity() const = 0;

    // Key that would be evicted next, without evicting it.
    virtual std::optional<K> peekEvictionCandidate() const = 0;
};
