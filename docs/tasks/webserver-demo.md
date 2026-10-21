# Task: Web-server response-cache CLI demo

**Owner:** Sharwan Kumar · **Phase 4** · **Branch:** `feature/webserver-demo`

## Scope
Simulate a web server's response cache under **Zipf-distributed** request traffic
(few hot URLs, long tail of cold ones) and show LRU vs LFU (optionally with TTL) side by side.

## Output
Per policy: requests, hits, misses, hit rate, evictions. Parameters: `--policy`, `--capacity`,
`--requests`, `--zipf-s`, `--ttl-ms`, `--seed` (fixed seed for reproducible runs).

## Acceptance criteria
- [ ] `make webdemo` builds and runs
- [ ] Deterministic with a fixed seed
- [ ] Reuses the Zipf generator from `feature/benchmarks` (coordinate to avoid duplicating it)
