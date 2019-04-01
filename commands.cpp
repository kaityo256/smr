#include "commands.hpp"
#include "mpisupport.hpp"
#include "util.hpp"
#include "rater.hpp"
#include "solver.hpp"
#include "uafinder.hpp"
#include "symmetry.hpp"
#include "symmetrysearcher.hpp"
#include "canonicalizer.hpp"
#include "combiner.hpp"
#include "commandcombine.hpp"
#include "utilforcommands.hpp"
#include "subsetsearcher.hpp"
#include "fixingsymmetries.hpp"
#include "fullsearcher.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <bitset>
#include <sstream>
#include <cctype>
#include <thread>
#include <mutex>
#include <cassert>
#include <queue>

#if defined(_WIN32) && !defined(_WINDOWS_)
struct FILETIME {
	unsigned dwLowDateTime, dwHighDateTime;
};
extern "C" void* __stdcall GetCurrentProcess(void);
extern "C" int __stdcall GetProcessTimes(void *hProcess, FILETIME *lpCreationTime, FILETIME *lpExitTime, FILETIME *lpKernelTime, FILETIME *lpUserTime);
#endif
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif

static void getCPUTime(double &userTime, double &systemTime) {
#ifdef _WIN32
	void *handle = GetCurrentProcess();
	FILETIME dummy1, dummy2, kernel, user;
	GetProcessTimes(handle, &dummy1, &dummy2, &kernel, &user);
	userTime = user.dwHighDateTime * 429.4967296 + user.dwLowDateTime * 1e-7;
	systemTime = kernel.dwHighDateTime * 429.4967296 + kernel.dwLowDateTime * 1e-7;
#else
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	userTime = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1e-6;
	systemTime = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1e-6;
#endif
}

struct CPUTimeIt {
	double user, sys;
	const char *msg;

	CPUTimeIt(const char *msg_) : msg(msg_) { getCPUTime(user, sys); }
	~CPUTimeIt() {
		double userEnd, sysEnd;
		getCPUTime(userEnd, sysEnd);
		fprintf(stderr, "%s: user %.6fs / sys %.6fs\n", msg, userEnd - user, sysEnd - sys);
	}

	operator bool() { return false; }
};
#define CPUTIMEIT(s) if(CPUTimeIt cputimeit_##__LINE__ = s); else

using namespace utilforcommands;
using namespace std;

int commandRate(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	int memoSize = 100000;
	bool nocheck = false;
	int threads = 1;
	for(const auto &p : arguments) {
		if(p.first == "memosize") {
			memoSize = parseInt(p.second, 0, 10000000);
		} if(p.first == "nocheck") {
			checkArgument(p.second.empty());
			nocheck = true;
		} else if(p.first == "threads") {
			threads = parseInt(p.second, 1, 1024);
		} else {
			checkArgument(false);
		}
	}
	if(threads != 1) {
		vector<string> problems;
		{
			string problem;
			while(cin >> problem) {
				checkProblem(problem);
				problems.push_back(problem);
			}
		}
		cerr << problems.size() << " problems" << endl;
		ThreadSafeQueue<int> workQ(problems.size() + threads);
		rep(i, problems.size())
			workQ.enqueue(i);
		rep(i, threads)
			workQ.enqueue(-1);
		ThreadSafeQueue<bool> endQ(threads);
		vector<long long> res(problems.size());
		int progress = 0;
		mutex mx;
		rep(i, threads) {
			thread([memoSize, &problems, &res, &workQ, &endQ, &progress, &mx]() {
				Rater rater(memoSize);
				Solver solver(0);
				while(1) {
					int problemID = workQ.dequeue();
					if(problemID == -1) break;
					const char *problem = problems[problemID].c_str();
					res[problemID] = solver.solve(problem) != 1 ? -1 : rater.rate(problem);
					{
						lock_guard<mutex> lock(mx);
						++ progress;
						if(progress * 1000LL / problems.size() != (progress - 1) * 1000LL / problems.size())
							cerr << progress << " / " << problems.size() << " (" << progress * 1000LL / problems.size() / 10. << "%)...\r";
					}
				}
				endQ.enqueue(true);
			}).detach();
		}
		rep(i, threads)
			endQ.dequeue();
		rep(i, problems.size()) {
			long long rate = res[i];
			cout << problems[i] << " " << rate << " " << Rater::squash(rate) << endl;
		}
		return 0;
	} else {
		string problem;
		Rater rater(memoSize);
		Solver solver(0);
		while(cin >> problem) {
			checkProblem(problem);
			bool ok = true;
			if(!nocheck) {
				int num = solver.solve(problem.c_str());
				ok = num == 1;
			}
			long long rate = !ok ? -1 : rater.rate(problem.c_str());
			cout << problem << " " << rate << " " << Rater::squash(rate) << endl;
		}
	}
	return 0;
}

int commandSolve(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	int memoSize = 100000;
	bool nocheck = false;
	bool randomize = false;
	for(const auto &p : arguments) {
		if(p.first == "memosize") {
			memoSize = parseInt(p.second, 0, 10000000);
		} else if(p.first == "nocheck") {
			checkArgument(p.second.empty());
			nocheck = true;
		} else if(p.first == "randomize") {
			checkArgument(p.second.empty());
			randomize = true;
		} else {
			checkArgument(false);
		}
	}
	Solver solver(memoSize);
	if(randomize)
		solver.setRandomEngine(default_random_engine{ random_device{}() });
	string problem;
	while(cin >> problem) {
		char solution[82];
		checkProblem(problem);
		int num = nocheck ? 1 : solver.solve(problem.c_str());
		if(num > 0) {
			if(!solver.findSolution(problem.c_str(), solution, randomize))
				num = 0;
		}
		if(num == 0)
			fill(solution, solution + 81, '0');
		solution[81] = 0;
		if(nocheck)
			cout << solution << endl;
		else
			cout << solution << " " << num << endl;
	}
	return 0;
}

int commandFindUASets(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	int sizeLimit = 12;
	for(const auto &p : arguments) {
		if(p.first == "size") {
			sizeLimit = parseInt(p.second, 1, 81);
		} else {
			checkArgument(false);
		}
	}
	UAFinder finder;
	string solution;
	while(cin >> solution) {
		checkSolution(solution);
		finder.findAll(solution.c_str(), sizeLimit);
		cout << solution << " " << sizeLimit << endl;
		finder.saveTo(cout);
	}
	return 0;
}

int commandPrintSymmetries(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	for(const auto &p : arguments) {
		VALUE_IS_UNUSED(p);
		checkArgument(false);
	}
	string grid;
	while(cin >> grid) {
		checkInput(grid.size() == 81, grid);
		vector<string> generated;
		generated.reserve(Symmetry::CELL_PERMUTATIONS_ORDER);
		string permuted(81, '?');
		for(const auto &perm : Symmetry::allCellPermutations()) {
			rep(i, 81)
				permuted[perm.get(i)] = grid[i];
			generated.push_back(permuted);
		}
		sort(generated.begin(), generated.end());
		generated.erase(unique(generated.begin(), generated.end()), generated.end());
		cout << generated.size() << endl;
		for(const string &s : generated)
			cout << s << endl;
	}
	return 0;
}

int commandCanonicalize(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 2);
	bool showPerm = false;
	for(const auto &p : arguments) {
		if(p.first == "permutation") {
			checkArgument(p.second.empty());
			showPerm = true;
		} else {
			checkArgument(false);
		}
	}
	Canonicalizer canonicalizer;
	if(positional[1] == "mask") {
		string str;
		while(cin >> str) {
			Mask81 mask; mask.clear();
			checkInput(str.size() == 81, str);
			rep(i, 81) {
				char c = str[i];
				checkInput(c == '0' || c == '1', str);
				if(c == '1') mask.set(i);
			}
			Symmetry::CellPermutation perm;
			Mask81 lexMin = canonicalizer.canonicalizeMask(mask, &perm);
			cout << lexMin.toBitString();
			if(showPerm)
				cout << " " << perm;
			cout << endl;
		}
	} else if(positional[1] == "solution") {
		string solution;
		while(cin >> solution) {
			checkSolution(solution);
			Canonicalizer::GridPermutation perm;
			string lexMin = canonicalizer.canonicalizeSolution(solution.c_str(), &perm);
			cout << lexMin;
			if(showPerm)
				cout << " " << perm;
			cout << endl;
		}
	} else if(positional[1] == "problem") {
		string problem;
		while(cin >> problem) {
			checkProblem(problem);
			Canonicalizer::GridPermutation perm;
			string lexMin = canonicalizer.canonicalizeProblem(problem.c_str(), &perm);
			cout << lexMin;
			if(showPerm)
				cout << " " << perm;
			cout << endl;
		}
	} else if(positional[1] == "solved") {
		string problem;
		Solver solver;
		char solution[82] = {};
		while(cin >> problem) {
			checkProblem(problem);
			checkInput(solver.solve(problem.c_str()) == 1, problem);
			solver.findSolution(problem.c_str(), solution);
			Canonicalizer::GridPermutation perm;
			string lexMinSolution = canonicalizer.canonicalizeSolution(solution, &perm);
			string canonProblem = perm.applyTo(problem.c_str());
			cout << lexMinSolution << endl;
			cout << canonProblem;
			if(showPerm)
				cout << " " << perm;
			cout << endl;
		}
	} else if(positional[1] == "naive") {
		string grid;
		while(cin >> grid) {
			checkInput(grid.size() == 81, grid);
			Symmetry::CellPermutation minPerm;
			string lexMin, permuted(81, '?');
			for(const auto &perm : Symmetry::allCellPermutations()) {
				rep(i, 81)
					permuted[perm.get(i)] = grid[i];
				if(lexMin.empty() || lexMin > permuted) {
					lexMin = permuted;
					minPerm = perm;
				}
			}
			cout << lexMin;
			if(showPerm)
				cout << " " << minPerm;
			cout << endl;
		}
	} else {
		checkArgument(false);
	}
	return 0;
}

int commandSquash(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1 && arguments.empty());
	long long a;
	while(cin >> a) {
		cout << "squash(" << a << ") = " << Rater::squash(a) << endl;
		cout << a << " = squash(" << Rater::unsquash((int)a) << ")" << endl;
	}
	return 0;
}

inline int nextCombination(int comb) {
	int x = comb & -comb, y = comb + x;
	return (((comb & ~y) / x) >> 1) | y;
}

static void fillGroupsRec(int groupid, Mask81 &mask, vector<Mask81> &res, const vector<pair<vector<int>, int>> &v) {
	if(groupid == (int)v.size()) {
		res.push_back(mask);
		return;
	}
	const auto &indices = v[groupid].first;
	int n = (int)indices.size(), r = v[groupid].second;
	if(r == 12 || r == 13) {
		//12: ブロック中に2つで、行・列それぞれにたかだか1つ
		//13: ブロック中に3つで、行・列それぞれにたかだか1つ(置換になっている)
		vector<int> patterns[2] = {
			{ 17,33,129,257,10,34,66,258,12,20,68,132,136,264,80,272,96,160 },
			{ 273,161,266,98,140,84 },
		};
		for(int comb : patterns[r - 12]) {
			for(int i : EachBit(comb))
				mask.set(indices[i]);
			fillGroupsRec(groupid + 1, mask, res, v);
			for(int i : EachBit(comb))
				mask.unset(indices[i]);
		}
	} else {
		for(int comb = (1 << r) - 1; comb < (1 << n); comb = nextCombination(comb)) {
			for(int i : EachBit(comb))
				mask.set(indices[i]);
			fillGroupsRec(groupid + 1, mask, res, v);
			for(int i : EachBit(comb))
				mask.unset(indices[i]);
		}
	}
}

int commandMountains(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1 && arguments.empty());
	string pattern;
	while(cin >> pattern) {
		checkInput(pattern.size() == 81, pattern);
		map<char, pair<vector<int>, int>> groups;
		Mask81 initmask; initmask.clear();
		rep(i, 81) {
			char c = pattern[i];
			if(!('0' <= c && c <= '9'))
				groups[c].first.push_back(i);
			else if(c != '0')
				initmask.set(i);
		}
		for(const auto &p : groups)
			checkInput(p.second.first.size() <= 20, pattern);
		if(!groups.empty()) {
			int rem = (int)groups.size();
			string group; int num;
			while(cin >> group >> num) {
				checkInput(group.size() == 1, group);
				checkInput(groups.count(group[0]) > 0, group);
				auto &p = groups[group[0]];
				checkInput(p.second == 0, group);
				if(num == 12 || num == 13) {
					checkInput(p.first.size() == 9, to_string(num));
				} else {
					checkInput(0 < num && num <= (int)p.first.size(), to_string(num));
				}
				p.second = num;
				if(-- rem == 0) break;
			}
		}
		vector<pair<vector<int>, int>> v;
		for(const auto &p : groups)
			v.push_back(p.second);
		vector<Mask81> res;
		fillGroupsRec(0, initmask, res, v);
		map<string, Mask81> maskSet;
		Canonicalizer canonicalizer;
		for(Mask81 mask : res) {
			Mask81 cmask = canonicalizer.canonicalizeMask(mask);
			maskSet.emplace(make_pair(cmask.toBitString(), mask));
		}
		cout << maskSet.size() << endl;
		for(const auto &p : maskSet) {
			string str = pattern;
			rep(i, 81) if(!p.second.get(i))
				str[i] = '0';
			cout << p.first << " " << str << endl;
		}
	}
	return 0;
}

int commandCommonMask(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1 && arguments.empty());
	int num;
	while(cin >> num) {
		checkInput(num > 0 && num <= 9, to_string(num));
		vector<Mask81> masks(num);
		vector<string> problems(num);
		rep(i, num) {
			string s;
			cin >> s;
			checkProblem(s);
			problems[i] = s;
			Mask81 mask; mask.clear();
			rep(j, 81) if(s[j] != '0')
				mask.set(j);
			masks[i] = mask;
			cout << mask.count() << " hints" << endl;
		}
		Mask81 baseMask = masks[0];
		vector<int> poses;
		vector<int> posIndex(81, -1);
		for(int pos : baseMask) {
			posIndex[pos] = (int)poses.size();
			poses.push_back(pos);
		}
		int card = (int)poses.size();
		checkInput(card <= 30, baseMask.toBitString());
		vector<uint8_t> intersectionMasks((size_t)1 << card);
		for(int i = 1; i < num; ++ i) {
			Mask81 mask = masks[i];
			for(const auto &perm : Symmetry::allCellPermutations()) {
				unsigned intersection = 0;
				for(int pos : mask) {
					int j = posIndex[perm.get(pos)];
					if(j != -1)
						intersection |= 1U << j;
				}
				intersectionMasks[intersection] |= 1 << (i - 1);
			}
		}
		int maxIntersection = 0;
		for(unsigned s = 0; s < 1U << card; ++ s) {
			if(intersectionMasks[s] == ((1 << (num - 1)) - 1)) {
				int cnt = countOneBits(s);
				if(maxIntersection < cnt)
					maxIntersection = cnt;
			}
		}
		cout << "max intersection: " << maxIntersection << endl;
		for(unsigned s = 0; s < 1U << card; ++ s) {
			if(intersectionMasks[s] != ((1U << (num - 1)) - 1) ||
				countOneBits(s) != maxIntersection)
				continue;
			vector<unsigned> str(81);
			for(int i : baseMask)
				str[i] |= 1U << 0;
			for(int i = 1; i < num; ++ i) {
				Mask81 mask = masks[i];
				for(const auto &perm : Symmetry::allCellPermutations()) {
					unsigned intersection = 0;
					for(int pos : mask) {
						int j = posIndex[perm.get(pos)];
						if(j != -1)
							intersection |= 1U << j;
					}
					if(intersection == s) {
						for(int pos : mask) {
							int p = perm.get(pos);
							str[p] |= 1U << i;
						}
						break;
					}
				}
			}
			map<unsigned, char> chars;
			chars[0] = '0';
			chars[(1U << num) - 1] = '1';
			rep(i, 81)
				chars.emplace(make_pair(str[i], 'A' - 2 + (int)chars.size()));
			string res(81, '?');
			rep(i, 81)
				res[i] = chars[str[i]];
			rep(i, 81) if(res[i] == '1')
				res[i] = problems[0][i];
			cout << res << endl;
			for(auto p : chars) if(p.second != '0' && p.second != '1') {
				cout << p.second << ":";
				for(int i : EachBit(p.first))
					cout << " " << i;
				cout << endl;
			}
		}
	}
	return 0;
}

int commandUAStatistics(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	int sizeLimit = 8;
	int threads = 1;
	int interval = 1;
	for(const auto &p : arguments) {
		if(p.first == "size") {
			sizeLimit = parseInt(p.second, 1, 81);
		} else if(p.first == "threads") {
			threads = parseInt(p.second, 1, 1024);
		} else if(p.first == "interval") {
			interval = parseInt(p.second, 1, 1000000000);
		} else {
			checkArgument(false);
		}
	}
	vector<map<string, long long>> cmaskCounts(sizeLimit + 1);
	long long samples = 0;
	mutex mx;
	condition_variable cv;
	unique_lock<mutex> lock(mx);
	ThreadSafeQueue<string> solutionQueue(threads);
	rep(threadid, threads) {
		thread([sizeLimit, interval, &cmaskCounts, &samples, &mx, &cv, &solutionQueue]() {
			UAFinder uafinder;
			Canonicalizer canonicalizer;
			while(1) {
				string solution = solutionQueue.dequeue();
				uafinder.findAll(solution.c_str(), sizeLimit);
				{
					lock_guard<mutex> lock(mx);
					++ samples;
					for(int size = 0; size <= sizeLimit; ++ size) {
						for(auto mask : uafinder.getList(size)) {
							Mask81 cmask = canonicalizer.canonicalizeMask(mask);
							++ cmaskCounts[size][cmask.toBitString()];
						}
					}
					if(samples % interval == 0)
						cv.notify_one();
				}
			}
		}).detach();
	}
	thread([&solutionQueue]() {
		string emptyProblem(81, '0');
		char solution[82]; solution[81] = 0;
		Solver solver;
		solver.setRandomEngine(default_random_engine{ random_device{}() });
		while(1) {
			solver.findSolution(emptyProblem.c_str(), solution, true);
			solutionQueue.enqueue(solution);
		}
	}).detach();

	long long lastSamples = 0;
	while(1) {
		cv.wait(lock, [&]() { return lastSamples < samples && samples % interval == 0; });
		{
			ofstream f("uastatistics.txt");
			f << sizeLimit << " " << samples << '\n';
			for(int size = 0; size <= sizeLimit; ++ size) {
				for(const auto &p : cmaskCounts[size])
					f << p.first << " " << p.second << '\n';
			}
		}

		cout << "samples = " << samples << endl;
		for(int size = 0; size <= sizeLimit; ++ size) {
			if(cmaskCounts[size].empty()) continue;
			cout << "size = " << size << ":" << endl;
			for(const auto &p : cmaskCounts[size]) {
				cout << p.first << ": " << p.second << " ";
				cout.precision(10);
				cout << p.second * 1. / samples << endl;
			}
		}
		cout << endl;
		lastSamples = samples;
	}
	return 0;
}

int commandSubsetSearch(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	for(const auto &p : arguments) {
		VALUE_IS_UNUSED(p);
		checkArgument(false);
	}

	SubsetSearcher searcher;
	Solver solver;
	Rater rater(1000000);
	UAFinder uaFinder;

	//random masks
	string sol, maskStr; int uaSize, lb, ub;
	char solution[82] = {};
	sol = "980700600007090050000000907400300020006020500000001000100004000030800000002070005";
	lb = 18, ub = 23, uaSize = 20;
	solver.findSolution(sol.c_str(), solution);
	{
		string cacheFile = "uasets_cache.txt";
		fstream f(cacheFile);
		string c_solution; int c_uaSize = -1;
		f >> c_solution >> c_uaSize;
		if(c_solution == solution && c_uaSize >= uaSize) {
			uaFinder.loadFrom(f);
		} else {
			f.close();
			CPUTIMEIT("finding ua sets...")
				uaFinder.findAll(solution, uaSize);
			f.open(cacheFile, ios_base::out);
			f << solution << " " << uaSize << '\n';
			uaFinder.saveTo(f);
			f.close();
		}
	}
	vector<Mask81> uaSets;
	for(int size = 1; size <= uaSize; ++ size) {
		const auto &list = uaFinder.getList(size);
		uaSets.insert(uaSets.end(), list.begin(), list.end());
	}
	searcher.setSolution(solution, uaSets);
	std::mt19937 re;
	while(1) {
		maskStr = string(81, '0');
//		maskStr = sol;
		{
			vector<int> poses;
			rep(i, 81) if(maskStr[i] == '0')
				poses.push_back(i);
			random_shuffle(poses.begin(), poses.end(), [&re](int x) {
				return re() % x;
			});
			rep(i, 40 - (int)(81 - poses.size()))
				maskStr[poses[i]] = 'X';
		}
		Mask81 mask = Mask81::fromBitString(maskStr.c_str());
		if(!searcher.setSuperSet(mask))
			continue;
		vector<string> problems;
		CPUTIMEIT("searchForSubsets")
			searcher.searchForSubsets(make_pair(lb, ub), problems);
		/*
		for(const string &problem : problems) {
			auto rate = rater.rate(problem.c_str());
			if(rate >= 1000) {
				cerr << problem << ": " << rate << endl;
			}
		}
		*/
	}

	return 0;
}

int commandFixingSymmetries(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	for(const auto &p : arguments) {
		VALUE_IS_UNUSED(p);
		checkArgument(false);
	}

	string str;
	while(cin >> str) {
		checkInput(str.size() == 81, str);
		Mask81 mask = Mask81::fromBitString(str.c_str());
		vector<FixingSymmetries::HintPosPermutation> perms;
		bool dup = FixingSymmetries::enumerateFixingSymmetries(mask, perms);

		std::vector<int> hintPoses;
		for(int pos : mask)
			hintPoses.push_back(pos);
		int numHints = (int)hintPoses.size();

		int posIndex[81];
		std::fill(posIndex, posIndex + 81, -1);
		for(int i = 0; i < numHints; ++ i)
			posIndex[hintPoses[i]] = i;
		
		cout << (dup ? "dup" : "nodup") << " ";
		cout << perms.size() << endl;
		for(const auto &perm : perms) {
			string permutedStr(81, '0');
			rep(i, numHints)
				permutedStr[hintPoses[perm.get(i)]] = str[hintPoses[i]];
			cout << permutedStr << " ";
			rep(i, numHints) {
				if(i != 0) cout << ' ';
				cout << perm.get(i);
			}
			cout << endl;

			/*
			rep(i, 9) {
				rep(j, 9) {
					cerr.width(2);
					if(posIndex[i * 9 + j] == -1)
						cerr << '.';
					else
						cerr << perm.get(posIndex[i * 9 + j]);
				}
				cerr << endl;
			}
			cerr << endl;
			*/
		}
	}

	return 0;
}

template<typename Callback>
static void stronglyUniqueRec(int i, const vector<int> &boxPattern, uint32_t digits, uint32_t bandStack[6], vector<uint8_t> &cur, Callback callback) {
	if(i == (int)boxPattern.size()) {
		callback(cur);
		return;
	}
	int box = boxPattern[i], band = box / 3, stack = box % 3;
	int mask = ((1 << 9) - 1) & ~bandStack[band] & ~bandStack[3 + stack];
	mask &= digits << 1 | 1;

	for(int d : EachBit(mask)) {
		uint32_t dmask = 1 << d;

		cur.push_back(d);
		bandStack[band] |= dmask;
		bandStack[3 + stack] |= dmask;

		uint32_t ndigits = digits | dmask;
		stronglyUniqueRec(i + 1, boxPattern, ndigits, bandStack, cur, callback);

		bandStack[3 + stack] &= ~dmask;
		bandStack[band] &= ~dmask;
		cur.pop_back();
	}
}

int commandStronglyUnique(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);
	bool dlist = false, nocnt = false, nolist = false;
	for(const auto &p : arguments) {
		if(p.first == "dlist") {
			checkArgument(p.second.empty(), p.first);
			dlist = true;
		} else if(p.first == "nocnt") {
			checkArgument(p.second.empty(), p.first);
			nocnt = true;
		} else if(p.first == "nolist") {
			checkArgument(p.second.empty(), p.first);
			nolist = true;
		} else {
			checkArgument(false);
		}
	}

	string str;
	while(cin >> str) {
		checkInput(str.size() == 81, str);
		vector<int> poses;
		rep(i, 81) if(str[i] != '0')
			poses.push_back(i);
		auto calcBox = [](int a) { return a / 27 * 3 + a % 9 / 3; };
		sort(poses.begin(), poses.end());
		vector<int> boxPattern;
		for(auto p : poses)
			boxPattern.push_back(calcBox(p));
		uint32_t bandStack[6] = {};
		vector<uint8_t> tmp;

		if(!nocnt) {
			long long cnt = 0;
			stronglyUniqueRec(0, boxPattern, 0, bandStack, tmp, [&cnt](const vector<uint8_t> &) {
				++ cnt;
			});
			cout << cnt << endl;
		}

		if(!nolist) {
			stronglyUniqueRec(0, boxPattern, 0, bandStack, tmp, [&dlist, &poses](const vector<uint8_t> &v) {
				if(dlist) {
					rep(i, v.size())
						cout << (int)v[i] + 1;
					cout << '\n';
				} else {
					string s(81, '0');
					rep(i, v.size()) s[poses[i]] = '1' + v[i];
					cout << s << '\n';
				}
			});
			cout.flush();
		}
	}
	return 0;
}

int commandFullsearch(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	string inputFilename = "";
	string outputFilename = "output.txt";
	checkArgument(positional.size() == 1);
	size_t memoSize = 1 << 16;
	int rateThreshold = 10000;
	int workers = -1;

	for(const auto &p : arguments) {
		if(p.first == "masks") {
			checkArgument(!p.second.empty());
			inputFilename = p.second;
		} else if(p.first == "output") {
			checkArgument(!p.second.empty());
			outputFilename = p.second;
		} else if(p.first == "memosize") {
			memoSize = parseInt(p.second, 0, numeric_limits<int>::max());
		} else if(p.first == "threshold") {
			rateThreshold = parseInt(p.second, 0, numeric_limits<int>::max());
		} else if(p.first == "workers") {
			workers = parseInt(p.second, 1, 1024);
		} else {
			checkArgument(false);
		}
	}
	Logger outputLogger(outputFilename.c_str(), false), infoLogger(true);

	if(workers == -1)
		workers = (int)thread::hardware_concurrency();

	ThreadSafeQueue<Mask81> maskQueue(workers + 1);
	vector<thread> threads(workers);

	rep(threadi, workers) {
		threads[threadi] = thread([&] {
			FullSearcher searcher(memoSize, rateThreshold, outputLogger, infoLogger);
			while(1) {
				Mask81 mask = maskQueue.dequeue();
				if(mask.count() == 0)
					break;
				searcher.searchStronglyUnique(mask);
			}
		});
	}

	{
		ifstream fin(inputFilename);
		if(!inputFilename.empty() && !fin)
			checkInput(false, inputFilename);
		istream &is = inputFilename.empty() ? cin : fin;
		string str;
		while(is >> str) {
			if(str.size() != 81) continue;
			Mask81 mask = Mask81::fromBitString(str.c_str());
			checkInput(mask.count() != 0, str);
			maskQueue.enqueue(mask);
		}
	}

	rep(i, workers) {
		Mask81 empty; empty.clear();
		maskQueue.enqueue(empty);
	}

	rep(i, workers)
		threads[i].join();

	return 0;
}

int commandsMain(const vector<string> &args) {
	vector<string> positional;
	unordered_map<string, string> arguments;
	for(const string &s : args) {
		if(s[0] == '-') {
			string option(s.substr(s[1] == '-' ? 2 : 1));
			size_t x = option.find('=');
			string name, value;
			if(x == string::npos) {
				name = option;
			} else {
				name = option.substr(0, x);
				value = option.substr(x + 1);
			}
			for(char &c : name)
				c = tolower(c);
			checkArgument(arguments.emplace(make_pair(name, value)).second);
		} else {
			positional.emplace_back(s);
		}
	}
	if(arguments.count("h") != 0 || arguments.count("help") != 0)
		help();
	checkArgument(!positional.empty());
	if(positional[0] == "rate") {
		return commandRate(positional, arguments);
	} else if(positional[0] == "solve") {
		return commandSolve(positional, arguments);
	} else if(positional[0] == "finduasets") {
		return commandFindUASets(positional, arguments);
	} else if(positional[0] == "printsymmetries") {
		return commandPrintSymmetries(positional, arguments);
	} else if(positional[0] == "canonicalize") {
		return commandCanonicalize(positional, arguments);
	} else if(positional[0] == "combine") {
		return commandCombine(positional, arguments);
	} else if(positional[0] == "squash") {
		return commandSquash(positional, arguments);
	} else if(positional[0] == "mountains") {
		return commandMountains(positional, arguments);
	} else if(positional[0] == "commonmask") {
		return commandCommonMask(positional, arguments);
	} else if(positional[0] == "uastatistics") {
		return commandUAStatistics(positional, arguments);
	} else if(positional[0] == "subsetsearch") {
		return commandSubsetSearch(positional, arguments);
	} else if(positional[0] == "fixingsymmetries") {
		return commandFixingSymmetries(positional, arguments);
	} else if(positional[0] == "stronglyunique") {
		return commandStronglyUnique(positional, arguments);
	} else if(positional[0] == "fullsearch") {
		return commandFullsearch(positional, arguments);
	} else {
		checkArgument(false, "error: no command");
	}
	return 0;
}
