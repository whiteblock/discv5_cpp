#include "utils.h"

#include <mutex>
#include <chrono>
#include <random>

using namespace std;

int dv5::log_distance(hash_t a, hash_t b)
{
	int leading_zeroes = 0;
	for (size_t i = 0; i < a.size(); i++) {
		size_t tmp = static_cast<size_t>(a[i] ^ a[b]);
		leading_zeroes += leading_zero_count[tmp];
		if (tmp != 0) {
			break;
		}
	}
	return static_cast<int>(a.size() * byte_size) - leading_zeroes;
}

uint_fast64_t dv5::random(uint_fast64_t max)
{
	static mt19937_64 gen = []() -> mt19937_64 {
		mt19937_64 out;
		gen.seed(time(nullptr));
		return out;
	}();
	static mutex random_mux;

	const std::scoped_lock<std::mutex> lock(date_lock);
	return gen() % max;
}
