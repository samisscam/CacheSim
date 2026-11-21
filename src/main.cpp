// CLI demo: replays a Zipf-skewed request log against a cache and prints
// HIT/MISS lines followed by a hit-rate summary.
//
// Usage:
//   ./cache_demo [lru|lfu] [capacity] [num_requests] [--ttl-ms N]
//
// Examples:
//   ./cache_demo lru 50 500
//   ./cache_demo lfu 20 300 --ttl-ms 800
#include "../include/EvictionPolicy.h"
#include "../include/LRUCache.h"
#include "../include/LFUCache.h"
#include "../include/Cache.h"
#include "../include/TTLCache.h"
#include "../include/ThreadSafeCache.h"

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <memory>
#include <cstdio>

using namespace std::chrono_literals;

struct Args {
    std::string policyName = "lru";
    std::size_t capacity = 20;
    int numRequests = 200;
    int ttlMs = 0; // 0 disables TTL
};

Args parseArgs(int argc, char** argv) {
    Args a;
    std::vector<std::string> pos;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--ttl-ms" && i + 1 < argc) {
            a.ttlMs = std::stoi(argv[++i]);
        } else {
            pos.push_back(arg);
        }
    }
    if (pos.size() >= 1) a.policyName = pos[0];
    if (pos.size() >= 2) a.capacity = static_cast<std::size_t>(std::stoul(pos[1]));
    if (pos.size() >= 3) a.numRequests = std::stoi(pos[2]);
    return a;
}

// Zipf-distributed URL ids (s=1): a few hot ids and a long cold tail, which
// resembles real traffic far more than uniform keys.
std::vector<int> generateRequestLog(int numRequests, int urlSpace, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<double> weights(urlSpace);
    for (int i = 0; i < urlSpace; ++i) weights[i] = 1.0 / (i + 1);
    std::discrete_distribution<int> dist(weights.begin(), weights.end());

    std::vector<int> log;
    log.reserve(numRequests);
    for (int i = 0; i < numRequests; ++i) log.push_back(dist(rng));
    return log;
}

std::string simulateDbFetch(int urlId) {
    std::this_thread::sleep_for(2ms);
    return "response-body-for-/api/resource/" + std::to_string(urlId);
}

int main(int argc, char** argv) {
    Args args = parseArgs(argc, argv);

    std::unique_ptr<EvictionPolicy<int, std::string>> policy;
    if (args.policyName == "lru") {
        policy = std::make_unique<LRUCache<int, std::string>>(args.capacity);
    } else if (args.policyName == "lfu") {
        policy = std::make_unique<LFUCache<int, std::string>>(args.capacity);
    } else {
        std::cerr << "Unknown policy '" << args.policyName << "', expected 'lru' or 'lfu'\n";
        return 1;
    }

    if (args.ttlMs > 0) {
        policy = std::make_unique<TTLCache<int, std::string>>(
            std::move(policy), std::chrono::milliseconds(args.ttlMs));
    }
    // Decorators stack: LRU/LFU -> TTL (optional) -> thread-safe.
    Cache<int, std::string> cache(
        std::make_unique<ThreadSafeCache<int, std::string>>(std::move(policy)));

    std::printf("=== Cache demo: policy=%s capacity=%zu requests=%d ttl_ms=%d ===\n\n",
                args.policyName.c_str(), args.capacity, args.numRequests, args.ttlMs);

    int urlSpace = static_cast<int>(args.capacity * 3); // URL space exceeds capacity to force evictions
    std::vector<int> requestLog = generateRequestLog(args.numRequests, urlSpace, 7);

    int hits = 0, misses = 0;
    auto wallStart = std::chrono::steady_clock::now();

    for (int i = 0; i < args.numRequests; ++i) {
        int urlId = requestLog[i];
        std::string path = "/api/resource/" + std::to_string(urlId);

        auto cached = cache.get(urlId);
        if (cached.has_value()) {
            ++hits;
            if (i < 40) std::printf("HIT   %-28s\n", path.c_str());
        } else {
            ++misses;
            std::string value = simulateDbFetch(urlId);
            cache.put(urlId, value);
            if (i < 40) std::printf("MISS  %-28s (fetched + cached)\n", path.c_str());
        }
    }
    auto wallEnd = std::chrono::steady_clock::now();
    double wallMs = std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();

    if (args.numRequests > 40) {
        std::printf("... (%d more requests, truncated for display) ...\n", args.numRequests - 40);
    }

    double hitRate = 100.0 * hits / args.numRequests;
    double dbTimeSavedMs = hits * 2.0;

    std::printf("\n--- Summary ---\n");
    std::printf("Total requests   : %d\n", args.numRequests);
    std::printf("Cache hits       : %d\n", hits);
    std::printf("Cache misses     : %d\n", misses);
    std::printf("Hit rate         : %.1f%%\n", hitRate);
    std::printf("Wall time        : %.1f ms\n", wallMs);
    std::printf("Simulated backend load avoided by caching: ~%.0f ms of DB fetch time\n",
                dbTimeSavedMs);

    return 0;
}
