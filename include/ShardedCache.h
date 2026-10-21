#pragma once
#include "EvictionPolicy.h"
#include <functional>
#include <memory>
#include <vector>

// TODO(feature/sharded-cache): N independently-locked shards routed by
// std::hash<K>. See docs/tasks/sharded-cache.md.
template <typename K, typename V>
class ShardedCache; // not implemented yet
