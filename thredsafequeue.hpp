#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <utility>

template<typename T>
class ThreadSafeQueue {
	std::vector<T> _data;
	size_t _head, _tail;
	bool _empty;
	size_t _maxSize;
	std::mutex _mutex;
	std::condition_variable _cvEnqueue, _cvDequeue;

public:
	ThreadSafeQueue(size_t maxSize) :
		_data(maxSize), _head(0), _tail(0), _empty(true), _maxSize(maxSize) { }
	
	void enqueue(const T &val) {
		std::unique_lock<std::mutex> lock(_mutex);
		_cvDequeue.wait(lock, [this]() { return _empty || _head != _tail; });
		_data[_tail] = val;
		if(++ _tail == _maxSize) _tail = 0;
		_empty = false;
		_cvEnqueue.notify_one();
	}

	T dequeue() {
		std::unique_lock<std::mutex> lock(_mutex);
		_cvEnqueue.wait(lock, [this]() { return !_empty; });
		T val = std::move(_data[_head]);
		if(++ _head == _maxSize) _head = 0;
		if(_head == _tail) _empty = true;
		_cvDequeue.notify_one();
		return val;
	}
};

