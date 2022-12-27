// TODO:
// (x) thread load balancing
// (x) cache_get/cache_set fusion? no
// - parameters autodiscovery
// - int vs uint
// (x) mmap? Not really helpful (on my machine)
// - testing

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "include/BS_thread_pool.hpp"

using namespace std;

#define F_UNROLL_AMNT       6
#define CACHE_EN            1
#define CACHE_ELEMS         (1 << 28)
#define CACHE_OPT_ACCESS_EN 1 //0= cache all values, 1= cache only odd numbers
#define CACHE_INVALID       0 //Default invalid cache item value
#define CACHE_MUTEX         0 //0= use atomic data type, 1= use mutex (don't)

/*****************************************************************************/
/* Caching stuff                                                             */
/*****************************************************************************/

#if CACHE_MUTEX
#include <mutex>
mutex                       g_cache_mutex;
vector<int16_t>             g_cache(CACHE_ELEMS);
#else
#include <atomic>
vector<atomic<int16_t>>     g_cache(CACHE_ELEMS);
#endif

#if CACHE_EN

static int cache_get(int64_t n) {
#if CACHE_MUTEX
	lock_guard<mutex> guard(g_cache_mutex);
#endif
#if CACHE_OPT_ACCESS_EN
	//Caching only the odd values of n:
	//Counts the number of trailing 0s
	unsigned int tlzs = __builtin_ctzl(n);
	//Removes the trailing zeroes and divides by 2 to compute
	//the cache's index
	int64_t key = n >> (tlzs + 1);
	if (key < CACHE_ELEMS) {
		int16_t cache_val = g_cache[key].load(memory_order_relaxed);
		if (cache_val != CACHE_INVALID)
			//We removed tlzs trailing zero bits that must
			//be taken into account
			return cache_val + tlzs;
	}
#else
	//Caching every n:
	if (n < CACHE_ELEMS) {
		int16_t cache_val = g_cache[n];
		if (cache_val != CACHE_INVALID)
			return cache_val;
	}
#endif
	return CACHE_INVALID;
}

//Always returns cycles (for conveniency)
static int cache_set(int64_t n, int16_t cycles) {
#if CACHE_MUTEX
	lock_guard<mutex> guard(g_cache_mutex);
#endif
#if CACHE_OPT_ACCESS_EN
	//Caching only the odd values of n:
	//Computes the cache index
	int64_t key = n >> 1;
	//If n is odd and fits into the cache
	if ((n & 1) && (key < CACHE_ELEMS))
		//...store it
		g_cache[key].store(cycles, memory_order_relaxed);
#else
	//Caching every n:
	//If it fits, store it
	if (n < CACHE_ELEMS)
		//...store it
		g_cache[n] = cycles;
#endif
	return cycles;
}

#endif

/*****************************************************************************/
/* The actual meat                                                           */
/*****************************************************************************/

int64_t collatz(int64_t n) {
	return n & 1 ? 3 * n + 1 : n >> 1;
}

static inline int collatz_stopping_time(int64_t n) {
	int16_t cycles = 0;
	for (int64_t n_it = n; n_it != 1;) {
#if CACHE_EN
		int16_t cached_cycles = cache_get(n_it);
		if (cached_cycles != CACHE_INVALID) {
			cycles += cached_cycles;
			break;
		}
#endif
		//Unrolling still works :)
		for (int u = 0; u < F_UNROLL_AMNT; u++) {
			n_it = collatz(n_it);
			cycles++;
			if (n_it == 1) {
				break;
			}
		}
	}
	//Updates the cache for n and return cycles
#if CACHE_EN
	return cache_set(n, cycles);
#else
	return cycles;
#endif
}

//Computes the maximum stopping time in the interval [i, j] using
//the provided step size (single thread).
int collatz_max_stop_cycles_st(int i, int j) {
	//You better not be kidding me
	//assert((i > 0) && (i < j) && (j < 1'000'000'000));
	int m = 0;
	for (int n = i; n <= j; ++n)
		m = max(m, collatz_stopping_time(n));
	return m;
}

//Computes the maximum stopping time in the interval [i, j] using
//the provided step size (multi thread)
int collatz_max_stop_cycles_mt(int i, int j, int threads, int step) {
	assert((i > 0) && (i < j) && (j < 1'000'000'000));

	BS::thread_pool pool(threads);

	vector<future<int>> max_cycles;
	for (int n = i; n <= j; n += step)
		max_cycles.push_back(
			pool.submit(
				collatz_max_stop_cycles_st,
				n,
				min(n + step - 1, j)
			)
		);
	pool.wait_for_tasks();

	int m = 0;
	for (auto &partial_max : max_cycles)
		m = max(m, partial_max.get());

	return m;
}

int main() {
	//Check needed for __builtin_ctzl()
	assert(sizeof(unsigned long int) == sizeof(int64_t));
	assert(sizeof(atomic<int16_t>) == sizeof(int16_t));

	cout << "Cache size: " << CACHE_ELEMS << endl;
	assert(collatz_max_stop_cycles_mt(1, 999'999'999, 4, 1'000'000) == 986);
}
