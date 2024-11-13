#include <future>
#include <iostream>
#include <random>
#include <vector>
#include "include/thread_pool.hpp"

// Helper function to check if a number is prime
bool is_prime(int n) {
	if (n <= 1)
		return false;
	if (n <= 3)
		return true;
	if (n % 2 == 0 || n % 3 == 0)
		return false;
	for (int i = 5; i * i <= n; i += 6) {
		if (n % i == 0 || n % (i + 2) == 0)
			return false;
	}
	return true;
}

/**
 * Calculates all primes from 0 to limit and returns them in a vector
 */
std::vector<int> find_primes(int limit) {
	std::vector<int> primes;
	for (int i = 2; i <= limit; ++i) {
		if (is_prime(i)) {
			primes.push_back(i);
		}
	}
	return primes;
}

// Main function
int main() {
	const int num_tasks = 920;
	std::vector<std::future<std::vector<int>>> futures;
	thread_pool pool(std::thread::hardware_concurrency());

	// Randon numbers generator
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(10000, 15000);

	for (int i = 0; i < num_tasks; ++i) {
		int random_number = dist(gen);

		// Submit the task to calculate primes up to the random number
		futures.push_back(pool.submit([random_number]() { return find_primes(random_number); }));
	}

	// Retrieve and print the results
	for (int i = 0; i < num_tasks; ++i) {
		std::vector<int> primes = futures[i].get();
		std::cout << "Task " << i + 1 << " generated up to " << primes.size() << " primes.\n";
	}

	return 0;
}
