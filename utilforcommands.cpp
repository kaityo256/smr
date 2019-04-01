#include "utilforcommands.hpp"
#include "rater.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
using namespace std;

namespace utilforcommands {

void help(string message) {
	if(!message.empty())
		cerr << message << endl;
	cerr << R"(
smr <command> <options>

smr rate [--memosize=<size>] [--nocheck] [--threads=<num>]
	Rate problems
	nocheck: do not check if a problem has multiple solutions

smr solve [--memosize=<size>] [--nocheck] [--randomize]
	Solve problems
	nocheck: do not check if a problem has multiple solutions
	randomize: randomize the solution search

smr finduasets [--size=<size>]
	Find unavoidable sets
	size: upper bound of UA set size

smr printsymmetries
	Print symmetries

smr canonicalize <mask|solution|problem|naive> [--permutation]
	Canonicalize grid
	mask: canonicalize hint masks
	solution: canonicalize solution grids
	problem: canonicalize problems
	naive: canonicalize arbitrary strings of length 81
	permutation: show permutation

smr combine [--problems=<filename>] [--masks=<filename>] [--solutions=<filename | RANDOM>] [--output=<filename>] [--threshold=<raw rate>] [--uasize=<size>] [--memosize=<size>] [--workers=<number of threads>] [--dclb=<digit count lower bound>] [--dcub=<digit count upper bound>] [--verboseness=<level>]
	Search combined problems
	problems=<filename>: same as --masks=<filename> --solutions=<filename>

smr squash
	Calculate squashed rating and inverse of that

smr mountains
	Fill and list mask patterns
	Example: "fuji-san pattern"
		100000003020000040005060700000XXX800000X0X000009XXX0000080096000400000YY3000000YY 
		X 5
		Y 2

smr commonmask
	Show common masks
	Example:
		2
		000000001000002340003010250001005004060000700800900000002004003070080000900600000
		000000001000002340003000250001005604060070000800600000002004005090800000700090000

smr uastatistics [--size=<ua set size>] [--threads=<num of threads>] [--interval=<output interval>]
)";
	exit(1);
}

void checkArgument(bool b, std::string message) {
	if(!b) {
		cerr << message << endl;
		help("error in the arguments");
	}
}

void checkInput(bool b, const string &input) {
	if(!b) {
		cout << "error: " << input << endl;
		exit(1);
	}
}

int parseInt(const string &s, int L, int U) {
	char *endptr = nullptr;
	int res = strtol(s.c_str(), &endptr, 0);
	checkArgument(endptr != nullptr && *endptr == 0);
	checkArgument(L <= res && res <= U);
	return res;
}

void checkProblem(const string &problem) {
	checkInput(problem.size() == 81 && all_of(problem.begin(), problem.end(), [](char c) { return '0' <= c && c <= '9'; }), problem);
}

void checkSolution(const string &solution) {
	checkInput(solution.size() == 81 && all_of(solution.begin(), solution.end(), [](char c) { return '1' <= c && c <= '9'; }), solution);
}

}
