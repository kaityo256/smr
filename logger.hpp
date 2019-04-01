#pragma once
#include <mutex>
#include <fstream>
#include <sstream>
#include <string>
#include "queueinterfaces.hpp"

class Logger {
	std::mutex _mutex;
	std::ofstream _file;
	bool _output;
	ResponceQueueInterface *_queue;

public:
	explicit Logger(bool output);
	Logger(const char *filename, bool output);
	explicit Logger(ResponceQueueInterface &queue);

	template<typename ...Ts>
	void log(Ts... values) {
		std::stringstream ss;
		ssRec(ss, values...);
		ss << '\n';
		logString(ss.str());
	}

	void logString(const std::string &str);

private:
	static void ssRec(std::stringstream &) {}
	template<typename Head, typename ...Tail>
	static void ssRec(std::stringstream &ss, const Head &head, Tail... tails) {
		ss << head;
		return ssRec(ss, tails...);
	}
};
