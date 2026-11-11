CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
INCLUDES := -Iinclude

.PHONY: all test bench demo tsan clean

all: test bench demo

# Unit tests (Catch2, single-header, vendored in tests/catch.hpp)
test: tests/run_tests
	./tests/run_tests

tests/run_tests: tests/test_cache.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/test_cache.cpp -o tests/run_tests

# Same tests, built with ThreadSanitizer, for the thread-safety test case
tsan: tests/test_cache.cpp include/*.h
	$(CXX) -std=c++17 -O1 -g -fsanitize=thread -pthread $(INCLUDES) \
		tests/test_cache.cpp -o tests/run_tests_tsan
	./tests/run_tests_tsan "[threadsafe]"

# Benchmarks: LRU/LFU vs naive std::map baseline
bench: benchmarks/run_benchmarks
	./benchmarks/run_benchmarks | tee benchmarks/results.csv

benchmarks/run_benchmarks: benchmarks/benchmark.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) benchmarks/benchmark.cpp -o benchmarks/run_benchmarks

# CLI demo: ./src/cache_demo [lru|lfu] [capacity] [num_requests] [--ttl-ms N]
demo: src/cache_demo

src/cache_demo: src/main.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) src/main.cpp -o src/cache_demo

clean:
	rm -f tests/run_tests tests/run_tests_tsan benchmarks/run_benchmarks src/cache_demo
