#pragma once
#include <cstddef>
#include <optional>

// Strategy interface implemented by every eviction policy.
// get() counts as a "touch" and must run in O(1) amortized time.
template <typename K, typename V>
class EvictionPolicy {
public:
    virtual ~EvictionPolicy() = default;

    virtual std::optional<V> get(const K& key) = 0;

    // Inserts or updates a key; evicts exactly one entry if capacity is exceeded.
    virtual void put(const K& key, const V& value) = 0;

    // Returns false if the key is absent.
    virtual bool erase(const K& key) = 0;

    virtual bool contains(const K& key) const = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t capacity() const = 0;

    // Key that would be evicted next, without evicting it.
    virtual std::optional<K> peekEvictionCandidate() const = 0;
};
