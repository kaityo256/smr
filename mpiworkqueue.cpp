#include "mpiworkqueue.hpp"
#include "mpisupport.hpp"
#include <cassert>
using namespace std;

#ifdef ENABLE_MPI

void MPIWorkQueue::enqueue(const WorkMessage &message) {
	int flag = 0;
	MPI_Status status;
	::MPI_Recv(&flag, 1, MPI_INT, MPI_ANY_SOURCE, TAG_NOTIFY, MPI_COMM_WORLD, &status);
	assert(flag == 1);
	int dest = status.MPI_SOURCE;
	auto send = [dest](const void *data, MPI_Datatype type, int count = 1) {
		::MPI_Send(const_cast<void*>(data), count, type, dest, TAG_WORK, MPI_COMM_WORLD);
	};

	int messageType = (int)message.type;
	send(&messageType, MPI_INT);
	switch(message.type) {
	case WorkMessage::Type::End:
		break;
	case WorkMessage::Type::SearchForSolution:
		assert(message.solution.size() == 81);
		send(message.solution.c_str(), MPI_CHAR, 81);
		break;
	}
}

WorkMessage MPIWorkQueue::dequeue() {
	auto recv = [](void *res, MPI_Datatype type, int count = 1) {
		MPI_Status status;
		::MPI_Recv(res, count, type, RANK_MANAGER, TAG_WORK, MPI_COMM_WORLD, &status);
	};

	int flag = 1;
	::MPI_Send(&flag, 1, MPI_INT, RANK_MANAGER, TAG_NOTIFY, MPI_COMM_WORLD);
	int messageType = -1;
	recv(&messageType, MPI_INT);
	WorkMessage message;
	message.type = (WorkMessage::Type)messageType;
	switch(message.type) {
	case WorkMessage::Type::End:
		break;
	case WorkMessage::Type::SearchForSolution:
		char str[81];
		recv(str, MPI_CHAR, 81);
		message.solution = string(str, str + 81);
		break;
	}
	return message;
}

#endif
