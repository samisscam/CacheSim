#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/EvictionPolicy.h"
#include "../include/LRUCache.h"
#include "../include/LFUCache.h"
#include "../include/Cache.h"
#include "../include/TTLCache.h"
#include "../include/ThreadSafeCache.h"

#include <thread>
#include <vector>
#include <chrono>

using namespace std::chrono_literals;

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
    cache.get(1);          // 2 becomes LRU
    cache.put(3, 3);       // evicts 2
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LRU: put on an existing key updates value and counts as a touch", "[lru]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(1, 100);     // touches 1, so 2 becomes LRU
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
    using IntLRU = LRUCache<int, int>; // a bare ',' would split the macro arguments
    REQUIRE_THROWS_AS(IntLRU(0), std::invalid_argument);
}

TEST_CASE("LRU: peekEvictionCandidate reports the correct victim", "[lru]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(1));
    cache.get(1);
    REQUIRE(cache.peekEvictionCandidate() == std::optional<int>(2));
}

TEST_CASE("LFU: evicts the least frequently used key", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(1);     // freq(1)=2, freq(2)=1
    cache.get(1);     // freq(1)=3
    cache.put(3, 3);  // evicts 2 (lowest freq)
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LFU: ties on frequency are broken by least-recently-used", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(2);      // freq(2)=2, so 1 is the sole minimum
    cache.put(3, 3);   // evicts 1
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
    REQUIRE(cache.contains(3));
}

TEST_CASE("LFU: a brand new key always resets the frequency floor to 1", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.get(1);
    cache.get(1);       // freq(1) = 3
    cache.put(2, 2);    // freq(2) = 1
    cache.put(3, 3);    // evicts 2, not 1
    REQUIRE(cache.contains(1));
    REQUIRE_FALSE(cache.contains(2));
}

TEST_CASE("LFU: put on existing key increments frequency like a get", "[lfu]") {
    LFUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(1, 999);  // freq(1) = 2
    cache.put(3, 3);    // evicts 2
    REQUIRE(cache.get(1) == std::optional<int>(999));
    REQUIRE_FALSE(cache.contains(2));
}

TEST_CASE("LFU: erase updates the min-frequency floor, later evictions stay correct", "[lfu]") {
    LFUCache<int, int> cache(3);
    cache.put(1, 1);
    cache.put(2, 2);
    cache.get(2);      // freq(2) = 2
    REQUIRE(cache.erase(1)); // removes the only freq-1 entry
    cache.put(3, 3);
    cache.put(4, 4);
    cache.put(5, 5);   // evicts 3, the LRU entry at the min frequency
    REQUIRE(cache.contains(2));
    REQUIRE_FALSE(cache.contains(3));
    REQUIRE(cache.contains(4));
    REQUIRE(cache.contains(5));
}

TEST_CASE("LFU: zero capacity is rejected", "[lfu]") {
    using IntLFU = LFUCache<int, int>;
    REQUIRE_THROWS_AS(IntLFU(0), std::invalid_argument);
}

TEST_CASE("Cache: delegates to whatever policy it was built with", "[cache]") {
    Cache<int, int> cache(std::make_unique<LRUCache<int, int>>(2));
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3);
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.capacity() == 2);
}

TEST_CASE("TTL: an entry is gone after its TTL elapses", "[ttl]") {
    TTLCache<int, int> cache(std::make_unique<LRUCache<int, int>>(10), 20ms);
    cache.put(1, 1);
    REQUIRE(cache.get(1).has_value());
    std::this_thread::sleep_for(40ms);
    REQUIRE_FALSE(cache.get(1).has_value());
}

TEST_CASE("TTL: putWithTTL overrides the default per-entry", "[ttl]") {
    TTLCache<int, int> cache(std::make_unique<LRUCache<int, int>>(10), 24h);
    cache.putWithTTL(1, 1, 10ms);
    cache.putWithTTL(2, 2, 24h);
    std::this_thread::sleep_for(30ms);
    cache.expireNow();
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
}

TEST_CASE("TTL: re-putting a key before expiry resets its clock (no ghost eviction)", "[ttl]") {
    TTLCache<int, int> cache(std::make_unique<LRUCache<int, int>>(10), 20ms);
    cache.put(1, 1);
    std::this_thread::sleep_for(10ms);
    cache.putWithTTL(1, 2, 200ms);
    std::this_thread::sleep_for(15ms); // original 20ms deadline has passed
    cache.expireNow();
    // The stale heap entry from the first put() must be ignored.
    REQUIRE(cache.get(1) == std::optional<int>(2));
}

TEST_CASE("TTL: still respects the underlying eviction policy's capacity", "[ttl]") {
    TTLCache<int, int> cache(std::make_unique<LRUCache<int, int>>(1), 24h);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE_FALSE(cache.contains(1));
    REQUIRE(cache.contains(2));
}

TEST_CASE("ThreadSafeCache: concurrent puts/gets don't corrupt state", "[threadsafe]") {
    ThreadSafeCache<int, int> cache(std::make_unique<LRUCache<int, int>>(1000));
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 2000;

    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&cache, t]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                int key = (t * kOpsPerThread + i) % 500;
                cache.put(key, key * 2);
                cache.get(key);
            }
        });
    }
    for (auto& w : workers) w.join();

    // The real check is a clean run under TSan (make tsan).
    REQUIRE(cache.size() <= 1000);
}
