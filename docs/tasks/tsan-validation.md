# Task: ThreadSanitizer validation + final write-up

**Owners:** All members · **Phase 4** · **Branch:** `test/tsan-validation`
**Depends on:** `feature/thread-safe-cache`, `feature/sharded-cache`

## Scope
- Add `make tsan`: build the tests with `-fsanitize=thread -g -O1`
- Stress tests: many threads, mixed get/put/erase on `ThreadSafeCache` and `ShardedCache`
- Fix any data races found; document each one in the results write-up

## Acceptance criteria
- [ ] `make tsan` runs with zero reported races
- [ ] Concurrency test cases added to the Catch2 suite
- [ ] Final results write-up completed (feeds the Phase 4 report)
