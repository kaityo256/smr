#include "loadlist.hpp"
#include "util.hpp"
#include "mask81.hpp"
#include "canonicalizer.hpp"
#include "solver.hpp"
#include "utilforcommands.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>

using namespace utilforcommands;
using namespace std;

void loadHintMasks(const string &maskListFilename, const string &maskListCacheFilename, vector<Mask81> &hintMasks) {
	if(!maskListCacheFilename.empty()) {
		ifstream fcache(maskListCacheFilename);
		string sig;
		fcache >> sig;
		if(sig == maskListFilename) {
			cout << "loading masks... (cached)" << endl;
			string str;
			while(fcache >> str) {
				checkInput(str.size() == 81, str);
				Mask81 mask; mask.clear();
				rep(i, 81) {
					if(str[i] == '1')
						mask.set(i);
					else
						checkInput(str[i] == '0', str);
				}
				hintMasks.push_back(mask);
			}
		}
	}
	if(hintMasks.empty()) {
		Canonicalizer canonicalizer;
		cout << "loading masks..." << endl;
		ifstream fin(maskListFilename, ios_base::in);
		checkInput(fin.is_open(), maskListFilename);
		unordered_set<string> hintMaskSet;
		string grid;
		int dups = 0;
		while(fin >> grid) {
			if(grid.size() != 81) continue;
			Mask81 hintMask;
			hintMask.clear();
			rep(cell, 81) {
				char c = grid[cell];
				if(c == '0' || c == '.')
					;
				else if('1' <= c && c <= '9')
					hintMask.set(cell);
				else
					checkInput(false, grid);
			}
			Mask81 lexMin = canonicalizer.canonicalizeMask(hintMask);
			if(hintMaskSet.emplace(lexMin.toBitString()).second)
				hintMasks.push_back(lexMin);
			else
				++ dups;
		}
		if(!maskListCacheFilename.empty()) {
			ofstream fcache(maskListCacheFilename);
			fcache << maskListFilename << endl;
			for(auto m : hintMasks)
				fcache << m.toBitString() << endl;
		}
	}
	cout << hintMasks.size() << " masks loaded" << endl;
}

void loadSolutions(const string &solutionListFilename, const string &solutionListCacheFilename, const string &knownProblemListCacheFilename, vector<string> &solutions, unordered_set<string> &knownProblemSet) {
	if(!solutionListCacheFilename.empty()) {
		ifstream fcache(solutionListCacheFilename);
		string sig;
		fcache >> sig;
		if(sig == solutionListFilename) {
			cout << "loading solutions... (cached)" << endl;
			string str;
			while(fcache >> str) {
				checkSolution(str);
				solutions.push_back(str);
			}
			ifstream fcache2(knownProblemListCacheFilename);
			fcache2 >> sig;
			checkInput(sig == solutionListFilename, knownProblemListCacheFilename);
			while(fcache2 >> str) {
				checkProblem(str);
				knownProblemSet.insert(str);
			}
		}
	}
	if(solutions.empty()) {
		cout << "loading solutions..." << endl;
		ifstream fin(solutionListFilename, ios_base::in);
		checkInput(fin.is_open(), solutionListFilename);
		unordered_set<string> solutionSet;
		string grid;
		Solver solver;
		Canonicalizer canonicalizer;
		int dups = 0;
		while(fin >> grid) {
			if(grid.size() != 81) continue;
			for(char &c : grid) if(c == '.') c = '0';
			checkProblem(grid);

			string problem = canonicalizer.canonicalizeProblem(grid.c_str());
			knownProblemSet.insert(problem);

			char solution[82];
			if(!solver.findSolution(grid.c_str(), solution))
				checkInput(false, grid);
			solution[81] = 0;
			string lexMin = canonicalizer.canonicalizeSolution(solution);
			if(solutionSet.emplace(lexMin).second)
				solutions.push_back(lexMin);
			else
				++ dups;
		}

		if(!solutionListCacheFilename.empty()) {
			ofstream fcache(solutionListCacheFilename);
			fcache << solutionListFilename << endl;
			for(auto s : solutions)
				fcache << s << endl;
		}
		if(!knownProblemListCacheFilename.empty()) {
			ofstream fcache(knownProblemListCacheFilename);
			fcache << solutionListFilename << endl;
			for(auto s : knownProblemSet)
				fcache << s << endl;
		}
	}
	cout << solutions.size() << " solutions loaded" << endl;
}
