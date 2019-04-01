#pragma once
#include "message.hpp"
#include "queueinterfaces.hpp"

class MPIWorkQueue : public WorkQueueInterface {
	static const int RANK_MANAGER = 0;
	enum {
		TAG_NOTIFY = 1,
		TAG_WORK = 2,
	};
	MPIWorkQueue() = default;
public:
	static MPIWorkQueue &getInstance() { static MPIWorkQueue instance; return instance; }

	void enqueue(const WorkMessage &message) override;
	WorkMessage dequeue() override;
};
