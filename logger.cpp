#include "logger.hpp"
#include "mpiresponcequeue.hpp"
#include "mpisupport.hpp"
#include <iostream>
#include <cstdlib>
using namespace std;

Logger::Logger(bool output) : _file(), _output(output), _queue(nullptr) {}

Logger::Logger(const char *filename, bool output) : _output(output), _queue(nullptr) {
	_file.open(filename, ios_base::app);
	if(!_file.is_open()) {
		cerr << "can't open " << filename << endl;
		abort();
	}
}

Logger::Logger(ResponceQueueInterface & queue) : _file(), _output(false), _queue(&queue) { }

void Logger::logString(const string & str) {
	{
		lock_guard<mutex> lock(_mutex);

		if(_file.is_open()) {
			_file << str;
			_file.flush();
		}
		if(_output) {
			cout << str;
			cout.flush();
		}
	}
	if(_queue != nullptr) {
		ResponceMessage message;
		message.type = ResponceMessage::Type::LogInfo;
		message.info = str;
		_queue->enqueue(message);
	}
}
