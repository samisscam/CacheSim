#pragma once
#include "EvictionPolicy.h"
#include <memory>
#include <mutex>

// TODO(feature/thread-safe-cache): Decorator that serializes all access to any
// EvictionPolicy with a single mutex. See docs/tasks/thread-safe-cache.md.
template <typename K, typename V>
class ThreadSafeCache; // not implemented yet
