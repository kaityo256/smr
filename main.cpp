#include "mpisupport.hpp"
#include "util.hpp"
#include "commands.hpp"

#include <vector>
#include <string>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
#ifdef ENABLE_MPI
	if(::MPI_Init(&argc, &argv) != MPI_SUCCESS) {
		cerr << "MPI_Init failed" << endl;
		return 1;
	}
#endif

	vector<string> args;
#ifdef MY_LOCAL_RUN
	if(argc == 1) {
		args = { "combine", "--uasize=14", "--dclb=1", "--problems=q2_top100.txt", "--output=output.txt", "--workers=1" };
	}
#endif
	if(args.empty()) {
		args.resize(argc - 1);
		rep(i, argc - 1) args[i] = argv[i + 1];
	}
	int res = commandsMain(args);

#ifdef ENABLE_MPI
	if(::MPI_Finalize() != MPI_SUCCESS) {
		cerr << "MPI_Finalize failed" << endl;
		return 1;
	}
#endif
	return res;
}
