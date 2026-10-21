# Task: NaiveMapCache baseline + benchmark suite

**Owners:** Sanyam Juyal, Sharwan Kumar · **Phase 4** · **Branch:** `feature/benchmarks`
> Riskiest remaining item (per Phase II report): needs all cache implementations stable first.

## Scope
1. `NaiveMapCache<K,V>` — deliberately simple `std::map` baseline (e.g. linear scan for the
   eviction victim) implementing `EvictionPolicy`.
2. `bench/bench.cpp` — times `get`/`put` for LRU, LFU and NaiveMapCache.

## Method
- Capacities: 100, 1,000, 10,000, 50,000
- Workloads: uniform random and Zipf-skewed keys (the Zipf generator is shared with `feature/webserver-demo`)
- Report ops/sec **and** hit rate (this is what makes the cost of a poor policy measurable)
- Use `std::chrono::steady_clock`, warm up first, repeat runs and report the median
- Build with `-O2`; output CSV for the final report

## Acceptance criteria
- [ ] Time per op stays ~flat for LRU/LFU as capacity grows; grows for the naive baseline
- [ ] `make bench` target runs the suite and writes `bench/results.csv`
- [ ] Results table included in the final report
