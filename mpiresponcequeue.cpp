#include "mpiresponcequeue.hpp"
#include "mpisupport.hpp"
#include <cassert>
#include <memory>
using namespace std;

#ifdef ENABLE_MPI

void MPIResponceQueue::enqueue(const ResponceMessage &message) {
	auto send = [](const void *data, MPI_Datatype type, int count = 1) {
		::MPI_Send(const_cast<void*>(data), count, type, RANK_MANAGER, TAG_RESPONCE, MPI_COMM_WORLD);
	};

	int messageType = (int)message.type;
	send(&messageType, MPI_INT);
	switch(message.type) {
	case ResponceMessage::Type::WorkerIsIdle:
		break;
	case ResponceMessage::Type::FoundProblem:
		assert(message.problem.size() == 81);
		send(message.problem.c_str(), MPI_CHAR, 81);
		send(&message.rate, MPI_LONG_LONG);
		break;
	case ResponceMessage::Type::LogInfo:
	{
		int size = (int)message.info.size();
		send(&size, MPI_INT);
		send(message.info.c_str(), MPI_CHAR, size);
	}
		break;
	case ResponceMessage::Type::AddNumberOfValidProblems:
	{
		int size = (int)message.validProblemCounts.size();
		send(&size, MPI_INT);
		send(message.validProblemCounts.data(), MPI_INT, size);
	}
		break;
	case ResponceMessage::Type::WorkerEnded:
		break;
	}
}

ResponceMessage MPIResponceQueue::dequeue() {
	int source = MPI_ANY_SOURCE;
	auto recv = [&source](void *res, MPI_Datatype type, int count = 1) {
		MPI_Status status;
		::MPI_Recv(res, count, type, source, TAG_RESPONCE, MPI_COMM_WORLD, &status);
		if(source == MPI_ANY_SOURCE)
			source = status.MPI_SOURCE;
	};
	int messageType = -1;
	recv(&messageType, MPI_INT);
	ResponceMessage message;
	message.type = (ResponceMessage::Type)messageType;
	switch(message.type) {
	case ResponceMessage::Type::WorkerIsIdle:
		break;
	case ResponceMessage::Type::FoundProblem:
	{
		char problem[81];
		recv(problem, MPI_CHAR, 81);
		message.problem = string(problem, problem + 81);
		recv(&message.rate, MPI_LONG_LONG);
	}
		break;
	case ResponceMessage::Type::LogInfo:
	{
		int size = -1;
		recv(&size, MPI_INT);
		unique_ptr<char[]> buf(new char[size]);
		recv(buf.get(), MPI_CHAR, size);
		message.info = string(buf.get(), buf.get() + size);
	}
		break;
	case ResponceMessage::Type::AddNumberOfValidProblems:
	{
		int size = -1;
		recv(&size, MPI_INT);
		message.validProblemCounts.resize(size);
		recv(message.validProblemCounts.data(), MPI_INT, size);
	}
		break;
	case ResponceMessage::Type::WorkerEnded:
		break;
	}
	return message;
}

#endif
