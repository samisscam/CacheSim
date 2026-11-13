# LRU/LFU Cache Simulator (C++17)

A from-scratch, O(1) LRU + LFU cache library with pluggable eviction
strategies, TTL expiry, thread-safety, benchmarks against a naive
`std::map` baseline, and a small CLI demo simulating a web server's
response cache.

Built as a DSA-fundamentals portfolio project: every design decision below
is something you should be able to defend live in an interview, not just
paste from a repo.

## Build & run

Requires g++ with C++17 (no external dependencies — Catch2 is vendored as
a single header in `tests/catch.hpp`).

```bash
make test    # build + run the unit test suite
make bench   # build + run benchmarks, writes benchmarks/results.csv
make demo    # build the CLI demo -> src/cache_demo
make tsan    # run the concurrency test under ThreadSanitizer
make all     # test + bench + demo
```

CLI demo usage:

```bash
./src/cache_demo [lru|lfu] [capacity] [num_requests] [--ttl-ms N]

# Examples
./src/cache_demo lru 50 500
./src/cache_demo lfu 20 300 --ttl-ms 800
```

## Architecture

```
EvictionPolicy<K,V>          <-- Strategy interface: get/put/erase/contains/size/capacity
    |
    +-- LRUCache<K,V>        <-- unordered_map + intrusive doubly linked list, O(1)
    +-- LFUCache<K,V>        <-- unordered_map + per-frequency doubly linked lists, O(1)
    +-- NaiveMapCache<K,V>   <-- std::map baseline used only in benchmarks (O(log n)/O(n))
    +-- TTLCache<K,V>        <-- Decorator: wraps any policy, adds a min-heap expiry queue
    +-- ThreadSafeCache<K,V> <-- Decorator: wraps any policy behind a std::mutex

Cache<K,V>                   <-- thin facade that owns a unique_ptr<EvictionPolicy<K,V>>
ShardedCache<K,V>            <-- bonus: N independent ThreadSafeCache shards keyed by hash(key)
```

`TTLCache` and `ThreadSafeCache` both implement `EvictionPolicy` themselves
and wrap another `EvictionPolicy` internally (Decorator pattern), so they
compose freely:

```cpp
auto policy = std::make_unique<LRUCache<int, std::string>>(100);
auto withTtl = std::make_unique<TTLCache<int, std::string>>(std::move(policy), 5s);
auto safe    = std::make_unique<ThreadSafeCache<int, std::string>>(std::move(withTtl));
Cache<int, std::string> cache(std::move(safe));
```

That's exactly what `src/main.cpp` does — stacks LRU/LFU → TTL → thread-safe
into one `Cache`, so the demo doubles as a usage example.

## How each policy achieves O(1)

**LRU** — `unordered_map<K, Node*>` for O(1) lookup straight to a node, plus
a doubly linked list ordered by recency (head = MRU, tail = LRU). A hit
unlinks the node and relinks it at the head; eviction removes the tail.
All pointer surgery is O(1); the hash map avoids ever scanning the list.

**LFU** — the classic frequency-bucket construction (LeetCode 460 /
Shah–Mitra–Matani 2010): `unordered_map<int, list<Entry>>` maps each
frequency to a doubly linked list of entries at that frequency (itself
LRU-ordered, front = most recent), and `unordered_map<K, list::iterator>`
gives O(1) lookup into the right bucket. A running `minFreq_` means
eviction is a plain `pop_back()` on the minimum bucket — no scan needed.
**Tie-break rule:** when several keys share the minimum frequency, the
least-recently-used one is evicted, because each bucket is itself LRU-
ordered.

**NaiveMapCache** (benchmark baseline only) — `std::map<K,V>` gives
O(log n) get, but has no notion of recency, so eviction requires an
O(n) linear scan to find the oldest entry. This is what LRUCache's linked
list specifically exists to avoid, and `BENCHMARKS.md` measures exactly how
much that costs as the cache grows.

## Design notes / known simplifications

- `Cache::setPolicy()` swaps the policy but does **not** migrate existing
  entries — switching from LRU to LFU mid-flight starts the new policy
  empty. A production cache would need an explicit migration step.
- `LFUCache::recomputeMinFreq()` is O(number of distinct frequencies), not
  O(1) — it only runs on the *explicit erase* path (e.g. TTL expiry
  removing a key), never on the hot get()/put() path, so it doesn't affect
  the O(1) amortized bound of normal operation. Worth naming as a
  trade-off if asked to make it airtight.
- `TTLCache` uses lazy expiry (checked on `get`/`put`, or forced via
  `expireNow()`) via a min-heap with generation-tagged entries, since
  `std::priority_queue` has no decrease-key. Stale heap entries from
  overwritten/erased keys are discarded when popped instead of removed
  eagerly — see the comments in `include/TTLCache.h`.
- `ThreadSafeCache` uses one coarse-grained mutex. `ShardedCache` shows the
  scale-up path (see interview Q&A below).

## Interview Q&A (talking points this repo backs up with code)

**"Design an LRU cache with O(1) get/put — implement it live."**
See `include/LRUCache.h`. Sentinel head/tail nodes remove null-pointer
edge cases; walk through: map lookup → unlink → relink at head.

**"How would you make this thread-safe? Single global lock vs sharded locks?"**
`ThreadSafeCache` (single mutex) is correct and simple but serializes ALL
access — readers block readers, one hot key can bottleneck everything.
`ShardedCache` (see `include/ShardedCache.h`) splits the keyspace across N
independently-locked shards so unrelated keys never contend, at the cost
of: (1) LRU/LFU order only holds *within* a shard, not globally — a skewed
key distribution can overfill one shard while others sit empty; (2)
`size()`/aggregate stats become approximate under concurrent writers
unless you lock every shard at once, which defeats the purpose.

**"How does LFU differ from LRU in eviction reasoning? Walk through a tie."**
LRU evicts by *recency* alone; LFU evicts by *frequency*, falling back to
recency only to break ties. Concretely: two keys both at freq 1, touch one
of them (freq 2) — now the *untouched* one is the unique minimum-frequency
key and gets evicted next, even if the touched key hasn't been touched
"more recently" in absolute terms. See the "ties on frequency are broken
by least-recently-used" test in `tests/test_cache.cpp` for a worked
example, including what happens when a brand-new key resets the frequency
floor back to 1.

**"How would you extend this to a distributed cache (invalidation across nodes)?"**
Not implemented here (out of scope for a single-process library), but the
shape of the answer this codebase sets up: `ShardedCache` already
demonstrates key-based routing; a distributed version replaces
`hash(key) % shardCount` with consistent hashing across *nodes* (not
in-process shards) to avoid a full remap on node add/remove, and needs
either (a) a pub/sub invalidation broadcast on every `put`/`erase` so
other nodes evict their local copy, or (b) short TTLs (already supported
via `TTLCache`) as a bounded-staleness fallback when broadcast isn't
feasible.

## Testing

`tests/test_cache.cpp` (Catch2) covers, per policy, the eviction edge
cases that are easy to get subtly wrong: overflow eviction, update-vs-insert
touch semantics, capacity-1 caches, erase on an absent key, frequency ties,
frequency-floor resets, TTL expiry + refresh (including the generation-
tagged stale-heap-entry case), and a concurrent stress test for
`ThreadSafeCache` (also run under ThreadSanitizer via `make tsan`).

## Benchmarks

See [`BENCHMARKS.md`](BENCHMARKS.md) for the full write-up and timing graph.
