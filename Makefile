CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
INCLUDES := -Iinclude

.PHONY: all test demo compare clean

all: test demo

test: tests/run_tests
	./tests/run_tests

tests/run_tests: tests/test_cache.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/test_cache.cpp -o tests/run_tests

demo: src/cache_demo
	./src/cache_demo lru 3

# Side-by-side LRU vs LFU on a frequency-vs-recency conflict
compare: src/cache_demo
	./src/cache_demo compare 3

src/cache_demo: src/main.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) src/main.cpp -o src/cache_demo

clean:
	rm -f tests/run_tests src/cache_demo
