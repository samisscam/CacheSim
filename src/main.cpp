// CLI demo for the LRU/LFU cache simulator.
//
// Usage:
//   ./cache_demo [lru|lfu] [capacity] [--ttl-ms N]
//                                       run a small demo on one policy,
//                                       optionally wrapped in a TTL decorator
//   ./cache_demo compare [capacity]     run LRU and LFU side by side on a
//                                       frequency-vs-recency conflict case
#include "../include/Cache.h"
#include "../include/EvictionPolicy.h"
#include "../include/LFUCache.h"
#include "../include/LRUCache.h"
#include "../include/TTLCache.h"

#include <iostream>
#include <memory>
#include <string>

using StrPolicy = EvictionPolicy<int, std::string>;

static std::unique_ptr<StrPolicy> makePolicy(const std::string& name, int capacity) {
    if (name == "lru") return std::make_unique<LRUCache<int, std::string>>(capacity);
    if (name == "lfu") return std::make_unique<LFUCache<int, std::string>>(capacity);
    return nullptr;
}

static void basicDemo(const std::string& policyName, int capacity, int ttlMs) {
    auto policy = makePolicy(policyName, capacity);
    if (ttlMs > 0) {
        policy = std::make_unique<TTLCache<int, std::string>>(
            std::move(policy), std::chrono::milliseconds(ttlMs));
    }
    Cache<int, std::string> cache(std::move(policy));

    std::cout << "=== " << policyName << " demo (capacity=" << capacity
              << (ttlMs > 0 ? ", ttl_ms=" + std::to_string(ttlMs) : "") << ") ===\n\n";
    cache.put(1, "value-1");
    cache.put(2, "value-2");
    std::cout << "get(1) = " << cache.get(1).value_or("(miss)") << "\n";
    cache.put(3, "value-3");
    std::cout << "get(2) = " << cache.get(2).value_or("(miss)") << "\n";
    std::cout << "get(3) = " << cache.get(3).value_or("(miss)") << "\n";
    std::cout << "current size = " << cache.size() << "\n";
}

// Key 1 is hot (touched 3x) but old; keys 2..capacity are touched once, more
// recently. A new key then forces an eviction:
//   LRU evicts the oldest-touched key (the hot key 1 if it is stalest)
//   LFU evicts the least-frequently-used key (never the hot key 1)
static void compareDemo(int capacity) {
    std::cout << "=== LRU vs LFU (capacity=" << capacity << ") ===\n\n";
    for (const std::string name : {"lru", "lfu"}) {
        Cache<int, std::string> cache(makePolicy(name, capacity));
        cache.put(1, "hot");
        cache.get(1);
        cache.get(1);
        for (int k = 2; k <= capacity; ++k) cache.put(k, "cold");
        auto victim = cache.peekEvictionCandidate();
        cache.put(capacity + 1, "new");
        std::cout << name << ": next victim was key "
                  << (victim ? std::to_string(*victim) : "(none)")
                  << " | hot key 1 " << (cache.contains(1) ? "SURVIVED" : "was EVICTED") << "\n";
    }
}

int main(int argc, char** argv) {
    std::string mode = "lru";
    int capacity = 3;
    int ttlMs = 0;
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--ttl-ms" && i + 1 < argc) {
            ttlMs = std::stoi(argv[++i]);
        } else if (positional == 0) {
            mode = arg; ++positional;
        } else if (positional == 1) {
            capacity = std::stoi(arg); ++positional;
        }
    }

    try {
        if (mode == "compare") {
            compareDemo(capacity);
        } else if (mode == "lru" || mode == "lfu") {
            basicDemo(mode, capacity, ttlMs);
        } else {
            std::cerr << "Unknown mode '" << mode << "', expected 'lru', 'lfu' or 'compare'\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
