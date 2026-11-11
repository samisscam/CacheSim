# Benchmarks: O(1) vs the naive approach

This measures `LRUCache` and `LFUCache` (both designed for O(1) amortized
`get`/`put`) against `NaiveMapCache` — a `std::map`-based cache built the
way someone reaches for a cache without thinking about eviction cost:
O(log n) lookup, but an O(n) linear scan to find the eviction victim on
every insert past capacity.

## Methodology

- `benchmarks/benchmark.cpp` builds one fixed, pre-generated workload per
  cache size (same random seed across all three policies, so they all see
  the *identical* sequence of keys — a fair comparison, not three
  independently-random runs).
- Each run: `numOps = 200,000` puts, immediately followed by `200,000`
  gets, with keys drawn uniformly from a key space of `2 × capacity` — so
  the cache is guaranteed to be under real, continuous eviction pressure
  at every capacity tested, not just filled once and left alone.
- Capacities tested: 100, 1,000, 10,000, 50,000.
- Timed with `std::chrono::steady_clock`; `ns_per_op` = total elapsed time
  divided by 400,000 (puts + gets combined).
- Build: `g++ -std=c++17 -O2`. Run `make bench` to regenerate
  `benchmarks/results.csv` on your own machine — absolute numbers will
  differ by hardware, but the *shape* of the curves (flat vs. exploding)
  should not.

## Results

| Policy   | Capacity | Ops     | Elapsed (ms) | ns/op    |
|----------|---------:|--------:|-------------:|---------:|
| LRU      | 100      | 200,000 | 11.9         | 29.8     |
| LFU      | 100      | 200,000 | 29.3         | 73.2     |
| NaiveMap | 100      | 200,000 | 87.1         | 217.8    |
| LRU      | 1,000    | 200,000 | 14.0         | 35.1     |
| LFU      | 1,000    | 200,000 | 24.9         | 62.3     |
| NaiveMap | 1,000    | 200,000 | 652.3        | 1,630.9  |
| LRU      | 10,000   | 200,000 | 16.3         | 40.7     |
| LFU      | 10,000   | 200,000 | 26.0         | 65.0     |
| NaiveMap | 10,000   | 200,000 | 9,741.4      | 24,353.6 |
| LRU      | 50,000   | 200,000 | 20.7         | 51.7     |
| LFU      | 50,000   | 200,000 | 37.6         | 94.1     |
| NaiveMap | 50,000   | 200,000 | 106,561.2    | 266,402.9|

(Raw CSV: `benchmarks/results.csv`. Regenerate any time with `make bench`.)

![LRU/LFU vs NaiveMapCache timing](benchmarks/timing_graph.png)

*Left: linear scale — NaiveMapCache's cost is invisible on the same axis
as LRU/LFU until it dwarfs them. Right: log-log scale — LRU and LFU stay
nearly flat as capacity grows two orders of magnitude, while NaiveMap's
line keeps climbing at the same slope, which is exactly what an O(n) term
dominating an O(log n) term looks like.*

## Big-O, and why the graph looks like that

| Operation                | LRU        | LFU                          | NaiveMap (baseline)     |
|---------------------------|-----------|-------------------------------|--------------------------|
| `get` (hit or miss)       | O(1)      | O(1)                          | O(log n)                 |
| `put`, no eviction        | O(1)      | O(1)                          | O(log n)                 |
| `put`, triggers eviction  | O(1)      | O(1)                          | O(log n) + **O(n) scan** |
| `erase`                   | O(1)      | O(1)                          | O(log n)                 |

The proof is structural, not empirical — the benchmark just confirms it:

- **LRU**: eviction is `tail_->prev`, a pointer dereference — independent
  of how many entries exist. The hash map means we never walk the list to
  *find* anything.
- **LFU**: eviction is `freqBuckets_[minFreq_].pop_back()` — again, no
  scan, because `minFreq_` is maintained incrementally on every touch
  rather than recomputed by scanning. (The one exception —
  `recomputeMinFreq()` on the explicit-erase path — is O(number of
  distinct frequencies), not O(n) entries, and is off the hot path; see
  README "Design notes".)
- **NaiveMap**: `std::map` gives ordered O(log n) lookup, but has no
  built-in notion of "which entry is oldest." `evictOldest()` therefore
  walks every entry to find the minimum insertion timestamp — that's a
  literal `for` loop over the whole map, so cost per eviction scales
  linearly with `n`. At `n = 50,000` that's the difference between a
  pointer swap and 50,000 comparisons, on *every single eviction* — which
  is why NaiveMap's `ns/op` grows roughly linearly with capacity while
  LRU/LFU's barely move.

## Why LFU is consistently slower than LRU despite both being O(1)

The benchmark shows LFU running at roughly 1.5–2.5x LRU's time per op even
though both are O(1). This is a real, expected constant-factor gap, not a
bug:

- LRU's per-op work is one hash lookup + one list splice.
- LFU's per-op work is one hash lookup + a splice **out of** the current
  frequency bucket + a splice **into** the next frequency bucket + an
  `unordered_map` insert/lookup on `freqBuckets_` itself (keyed by `int`
  frequency) — strictly more pointer/hash-table work per touch, even
  though none of it depends on `n`.

Worth saying out loud in an interview: "same asymptotic class, different
constant factor" is a distinct claim from "same speed."
