# Task: ShardedCache extension

**Owner:** Sanyam Juyal (with Shikhar Gangwar) · **Phase 3** · **Branch:** `feature/sharded-cache`
**Depends on:** `feature/thread-safe-cache`

## Scope
`ShardedCache<K,V>`: N independent shards, each a `ThreadSafeCache` (or a policy + its own mutex).
A key maps to a shard via `std::hash<K>{}(key) % N`, so threads touching different shards don't contend.

## Notes
- Capacity is split per shard (`ceil(capacity / N)`); eviction is therefore *per shard*, not global —
  document this trade-off.
- Shard count should be a constructor parameter (default e.g. 8 or `hardware_concurrency`).

## Acceptance criteria
- [ ] `include/ShardedCache.h` implements `EvictionPolicy`
- [ ] Tests: same-key operations stay consistent; multi-threaded mixed workload
- [ ] Benchmark hook: constructible with N=1 (equivalent to a single lock) for comparison
