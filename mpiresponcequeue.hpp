#pragma once
#include "message.hpp"
#include "queueinterfaces.hpp"

class MPIResponceQueue : public ResponceQueueInterface {
	static const int RANK_MANAGER = 0;
	enum {
		TAG_RESPONCE = 3,
	};
	MPIResponceQueue() = default;
public:
	static MPIResponceQueue &getInstance() { static MPIResponceQueue instance; return instance; }

	void enqueue(const ResponceMessage &message) override;
	ResponceMessage dequeue() override;
};
