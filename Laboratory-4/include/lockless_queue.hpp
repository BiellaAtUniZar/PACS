#pragma once

#include <atomic>
#include <memory>

template<typename T>
class lockless_queue {
private:
	struct node {
		std::shared_ptr<T> data;
		std::atomic<node *> next;

		node() : next(nullptr) {}
		node(T value) : data(std::make_shared<T>(std::move(value))), next(nullptr) {}
	};

	std::atomic<node *> head;
	std::atomic<node *> tail;

public:
	lockless_queue() : head(new node), tail(head.load()) {}

	~lockless_queue() {
		while (node *old_head = head.load()) {
			head.store(old_head->next);
			delete old_head;
		}
	}

	lockless_queue(const lockless_queue &other) = delete;
	lockless_queue &operator=(const lockless_queue &) = delete;

	void push(T new_value) {
		node *new_node = new node(std::move(new_value));
		node *old_tail = tail.exchange(new_node);

		old_tail->next.store(new_node, std::memory_order_release);
	}

	bool try_pop(T &value) {
		node *old_head = head.load(std::memory_order_acquire);
		node *next = old_head->next.load(std::memory_order_acquire);

		if (next == nullptr) {
			return false;
		}

		value = std::move(*next->data);
		head.store(next, std::memory_order_release);

		delete old_head;
		return true;
	}

	std::shared_ptr<T> wait_and_pop() {
		std::shared_ptr<T> result;
		node *old_head = head.load(std::memory_order_acquire);
		node *next = old_head->next.load(std::memory_order_acquire);

		while (next == nullptr) {
			next = old_head->next.load(std::memory_order_acquire);
		}

		result = next->data;
		head.store(next, std::memory_order_release);
		delete old_head;

		return result;
	}

	void wait_and_pop(T &value) { value = *wait_and_pop(); }

	bool empty() const { return head.load()->next.load() == nullptr; }
};
