#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/EvictionPolicy.h"
#include "../include/LRUCache.h"
#include "../include/LFUCache.h"
#include "../include/Cache.h"

TEST_CASE("LRU: basic get/put and miss handling", "[lru]") {
    LRUCache<int, std::string> cache(2);
    REQUIRE_FALSE(cache.get(1).has_value());
    cache.put(1, "a");
    REQUIRE(cache.get(1) == std::optional<std::string>("a"));
}

TEST_CASE("LRU: evicts the least recently used key on overflow", "[lru]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(1);
    cache.put(3, 3);       // evicts 2
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LRU: put on an existing key updates value and counts as a touch", "[lru]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(1, 100);
    cache.put(3, 3);       // evicts 2
    REQUIRE(cache.get(1) == std::optional<int>(100));
    REQUIRE_FALSE(cache.contains(2));
}

TEST_CASE("LRU: capacity of 1 always evicts the previous single entry", "[lru]") {
    LRUCache<int, int> cache(1);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
}

TEST_CASE("LRU: erase removes a key without disturbing others", "[lru]") {
    LRUCache<int, int> cache(3);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.erase(1));
    REQUIRE_FALSE(cache.erase(1));
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
}

TEST_CASE("LRU: zero capacity is rejected", "[lru]") {
    using IntLRU = LRUCache<int, int>;
    REQUIRE_THROWS_AS(IntLRU(0), std::invalid_argument);
}

TEST_CASE("LFU: evicts the least frequently used key", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(1);
    cache.get(1);
    cache.put(3, 3);  // evicts 2 (lowest freq)
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LFU: ties on frequency are broken by least-recently-used", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(2);
    cache.put(3, 3);   // evicts 1 (lowest freq)
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LFU: a brand new key always resets the frequency floor to 1", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.get(1);
    cache.get(1);
    cache.put(2, 2);
    cache.put(3, 3);    // evicts 2 (freq 1), not 1 (freq 3)
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
}

TEST_CASE("LFU: zero capacity is rejected", "[lfu]") {
    using IntLFU = LFUCache<int, int>;
    REQUIRE_THROWS_AS(IntLFU(0), std::invalid_argument);
}

TEST_CASE("Cache: delegates to whatever policy it was built with", "[cache]") {
    Cache<int, int> cache(std::make_unique<LRUCache<int, int>>(2));
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3); // evicts 1
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.capacity() == 2);
}

TEST_CASE("LRU: peekEvictionCandidate reports the tail without evicting", "[lru]") {
    LRUCache<int, int> cache(3);
    REQUIRE_FALSE(cache.peekEvictionCandidate().has_value());
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(1));
    cache.get(1);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(2));
}

TEST_CASE("LFU: peekEvictionCandidate follows lowest frequency, then recency", "[lfu]") {
    LFUCache<int, int> cache(3);
    REQUIRE_FALSE(cache.peekEvictionCandidate().has_value());
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(1));
    cache.get(1);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(2));
}

TEST_CASE("LFU: put on an existing key updates value and counts as a touch", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(1, 10);   // freq(1) = 2
    cache.put(3, 3);    // evicts 2 (freq 1)
    REQUIRE(cache.get(1) == std::optional<int>(10));
    REQUIRE_FALSE(cache.contains(2));
}

TEST_CASE("LFU: erasing the minimum-frequency bucket keeps eviction correct", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(2);       // freq(2) = 2, bucket 1 holds only key 1
    REQUIRE(cache.erase(1));
    cache.put(3, 3);
    cache.put(4, 4);    // evicts 3 (freq 1), not 2 (freq 2)
    REQUIRE(cache.contains(2));
    REQUIRE_FALSE(cache.contains(3));
}
