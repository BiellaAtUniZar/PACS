#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <type_traits>
#include <vector>

#include "join_threads.hpp"
#include "threadsafe_queue.hpp"

class thread_pool {
	using task_type = std::function<void()>;

	std::atomic<bool> _done;
	threadsafe_queue<task_type> _queue;
	std::vector<std::thread> _pool;
	join_threads _joiner;

	void worker_thread() {
		while (!_done) {
			task_type task;
			if (_queue.try_pop(task)) {
				task();
			} else {
				std::this_thread::yield();
			}
		}
	}

public:
	/**
	 * Constructor
	 * @params num_threads the number of working threads to put in the pool
	 */
	thread_pool(size_t num_threads = std::thread::hardware_concurrency()) :
		_done(false), _pool(), _joiner(_pool) {

		for (size_t i = 0; i < num_threads; i++) {
			_pool.emplace_back(&thread_pool::worker_thread, this);
		}
	}

	/**
	 * Destructor
	 */
	~thread_pool() { _done = true; }

	/**
	 * Blocking call until the queue is done doing the job
	 */
	void wait() {
		while (!_queue.empty() || !_done) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	/**
	 * Send the threads a new task
	 */
	template<typename F>
	auto submit(F f) -> std::future<typename std::result_of<F()>::type> {
		using result_type = typename std::result_of<F()>::type;

		auto task = std::make_shared<std::packaged_task<result_type()>>(f);
		std::future<result_type> res = task->get_future();

		_queue.push([task]() { (*task)(); });

		return res;
	}
};
