#pragma once
#include "thredsafequeue.hpp"
#include "message.hpp"

class WorkQueueInterface {
public:
	virtual ~WorkQueueInterface() = default;

	virtual void enqueue(const WorkMessage &message) = 0;
	virtual WorkMessage dequeue() = 0;
};

class ResponceQueueInterface {
public:
	virtual ~ResponceQueueInterface() = default;

	virtual void enqueue(const ResponceMessage &message) = 0;
	virtual ResponceMessage dequeue() = 0;
};

class MultithreadWorkQueue : public WorkQueueInterface {
	ThreadSafeQueue<WorkMessage> _q;
public:
	explicit MultithreadWorkQueue(size_t maxSize) : _q(maxSize) {}

	void enqueue(const WorkMessage &message) override { _q.enqueue(message); }
	WorkMessage dequeue() override { return _q.dequeue(); }
};

class MultithreadResponceQueue : public ResponceQueueInterface {
	ThreadSafeQueue<ResponceMessage> _q;
public:
	explicit MultithreadResponceQueue(size_t maxSize) : _q(maxSize) {}

	void enqueue(const ResponceMessage &message) override { _q.enqueue(message); }
	ResponceMessage dequeue() override { return _q.dequeue(); }
};
