# Branching & Workflow

## Branch map

```
main                         ← stable, tagged releases (v0.2-phase2 = current)
└── develop                  ← integration branch; all feature PRs target this
    ├── feature/ttl-cache            Razee Sajid Khan          Phase 3
    ├── feature/thread-safe-cache    Shikhar Gangwar           Phase 3
    ├── feature/sharded-cache        Sanyam Juyal              Phase 3
    ├── feature/benchmarks           Sanyam Juyal, Sharwan Kumar   Phase 4
    ├── feature/webserver-demo       Sharwan Kumar             Phase 4
    └── test/tsan-validation         All members               Phase 4
```

| Branch | Delivers | Depends on |
|---|---|---|
| `feature/ttl-cache` | `TTLCache` decorator + TTL tests + `--ttl-ms` CLI flag | — |
| `feature/thread-safe-cache` | `ThreadSafeCache` (single mutex) + concurrency tests | — |
| `feature/sharded-cache` | `ShardedCache` (N locked shards) | `thread-safe-cache` |
| `feature/benchmarks` | `NaiveMapCache` baseline + benchmark suite (capacities up to 50,000) | LRU, LFU (done) |
| `feature/webserver-demo` | Zipf-traffic web-server response-cache demo | `benchmarks` (shares Zipf generator) |
| `test/tsan-validation` | ThreadSanitizer build + concurrent stress tests | `thread-safe-cache`, `sharded-cache` |

Each feature branch contains a task spec in `docs/tasks/` with scope and acceptance criteria.

## Rules

1. Never commit directly to `main` or `develop`. Open a Pull Request into `develop`.
2. `make test` must pass before requesting review. Add tests for new behaviour.
3. At least one teammate reviews every PR (the Team Lead reviews anything touching `EvictionPolicy`).
4. Changing the `EvictionPolicy` interface needs the whole team's agreement — LRU/LFU depend on it.
5. Merge feature branches with a normal merge (or squash). At each phase end, merge `develop` → `main` and tag it.
6. Commit messages: short imperative summary (`Add TTLCache decorator`, `Fix minFreq_ on erase`).

## Everyday commands

```bash
git checkout develop && git pull
git checkout feature/ttl-cache          # work on your branch
git add -A && git commit -m "Add TTLCache heap purge"
git push origin feature/ttl-cache       # then open a PR into develop on GitHub

# keep your branch current
git fetch origin && git merge origin/develop
```
