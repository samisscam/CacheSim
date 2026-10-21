# CacheSim — LRU/LFU Cache Simulator

**A Benchmarked, Thread-Safe Caching Library in C++17**
Project-Based Learning (PBL), CSE — Graphic Era University · Team **Syntax Squad**

A from-scratch, header-only caching library with true **O(1)** `get`/`put` for both
LRU and LFU eviction, unified behind a single `EvictionPolicy` interface (Strategy pattern).
Most caches hide their eviction structures inside a library (Redis, Guava/Caffeine) or are built
ad-hoc on `std::map` without measuring what a poor eviction policy costs. This project makes
that cost measurable.

## Status — Phase II (~50% complete)

| Component | Status | Phase |
|---|---|---|
| `EvictionPolicy<K,V>` Strategy interface | Done | 1 |
| `LRUCache<K,V>` — hash map + doubly linked list | Done | 1 |
| `LFUCache<K,V>` — frequency buckets, incremental `minFreq_` | Done | 2 |
| `Cache<K,V>` facade | Done | 1 |
| Catch2 suite — 15 cases / 36 assertions | Done | 2 |
| `TTLCache` decorator (min-heap, generation-tagged lazy deletion) | Pending → `feature/ttl-cache` | 3 |
| `ThreadSafeCache` decorator / `ShardedCache` | Pending → `feature/thread-safe-cache`, `feature/sharded-cache` | 3 |
| `NaiveMapCache` baseline + benchmark suite | Pending → `feature/benchmarks` | 4 |
| Zipf web-server CLI demo | Pending → `feature/webserver-demo` | 4 |
| ThreadSanitizer validation | Pending → `test/tsan-validation` | 4 |

## Build & run

Requires a C++17 compiler (g++ or clang++) and `make`. Catch2 is vendored in `tests/catch.hpp`.

```bash
make test      # build + run the unit tests
make demo      # single-policy demo (LRU, capacity 3)
make compare   # LRU vs LFU side by side on a frequency-vs-recency conflict
```

```bash
./src/cache_demo lfu 5
./src/cache_demo compare 3
```

## Usage

```cpp
#include "Cache.h"
#include "LRUCache.h"
#include "LFUCache.h"

Cache<std::string, int> cache(std::make_unique<LRUCache<std::string, int>>(100));
cache.put("a", 1);
auto v = cache.get("a");                       // std::optional<int>

// Swap policy without touching client code (existing entries are not migrated):
cache.setPolicy(std::make_unique<LFUCache<std::string, int>>(100));
```

## Repository layout

```
include/   EvictionPolicy.h  LRUCache.h  LFUCache.h  Cache.h     (header-only library)
src/       main.cpp                                              (CLI demo)
tests/     test_cache.cpp  catch.hpp                             (Catch2 suite)
docs/      ARCHITECTURE.md  BRANCHING.md  tasks/                 (design + workflow)
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — patterns, class diagram, design decisions
- [Branching & workflow](docs/BRANCHING.md) — branch map, owners, PR rules

## Team

| Name | Student ID | Role this phase |
|---|---|---|
| Shikhar Gangwar (Team Lead) | 2028120 | `EvictionPolicy` interface, Makefile, docs |
| Sanyam Juyal | 2028091 | `LFUCache`, test suite |
| Razee Sajid Khan | 2028032 | `LRUCache`, test suite |
| Sharwan Kumar | 2029084 | `Cache` facade, docs |
