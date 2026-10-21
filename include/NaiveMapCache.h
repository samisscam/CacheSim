#pragma once
#include "EvictionPolicy.h"
#include <map>

// TODO(feature/benchmarks): naive std::map baseline used to validate the O(1)
// claims of LRUCache/LFUCache. See docs/tasks/benchmarks.md.
template <typename K, typename V>
class NaiveMapCache; // not implemented yet
