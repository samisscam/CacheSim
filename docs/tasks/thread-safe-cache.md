# Task: ThreadSafeCache decorator

**Owners:** Shikhar Gangwar (with Sanyam Juyal) · **Phase 3** · **Branch:** `feature/thread-safe-cache`

## Scope
A decorator implementing `EvictionPolicy<K,V>` that wraps any policy and guards every call
with a single `std::mutex`.

## Notes
- `get()` mutates recency/frequency state, so it needs the **exclusive** lock too — a
  `shared_mutex` reader lock is *not* safe for `get`. Use `std::mutex` / `lock_guard`.
- `contains`/`size`/`capacity`/`peekEvictionCandidate` are `const`; make the mutex `mutable`.
- Must compose: `ThreadSafeCache(TTLCache(LRUCache))` should work.

## Acceptance criteria
- [ ] `include/ThreadSafeCache.h` implements every `EvictionPolicy` method
- [ ] Tests: N threads doing mixed get/put never crash, `size() <= capacity()` always holds
- [ ] `make test` passes
- [ ] Clean under ThreadSanitizer (final check lives on `test/tsan-validation`)
