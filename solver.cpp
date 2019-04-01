#include "solver.hpp"
#include "util.hpp"
#include <algorithm>
#include <tuple>
#include <cassert>
#include <random>

using namespace std;

int Solver::solve(const char *problem) {
	SolverGrid grid;
	grid.load(problem);
	return solveGrid(grid);
}

int Solver::solveGrid(SolverGrid grid) {
	_findSolution = false;
	_randomize = false;
	return dfs(grid);
}

bool Solver::findSolution(const char *problem, char *solution, bool randomize) {
	_findSolution = true;
	_randomize = randomize;
	SolverGrid grid;
	grid.load(problem);
	int solutions = dfs(grid);
	if(solutions == 0) {
		rep(cell, 81)
			solution[cell] = '0';
		return false;
	} else {
		rep(cell, 81)
			solution[cell] = '1' + grid.getSolutionDigit(cell);
		return true;
	}
}

int Solver::dfs(SolverGrid &grid) {
	vector<SolverGrid::Hash> hashHistory;
	auto setResultMemo = [this, &hashHistory](int solutions) -> int {
		for(SolverGrid::Hash h : hashHistory)
			_resultMemo.insert(h, solutions);
		hashHistory.clear();
		return solutions;
	};

	while(1) {
		if(!_findSolution) {
			SolverGrid::Hash gridHash = grid.getHash();
			int *p = _resultMemo.find(gridHash);
			if(p != nullptr) {
				setResultMemo(*p);
				grid.init();
				return *p;
			}
			hashHistory.push_back(gridHash);
		}

		SolverGrid::StepResult result;
		grid.makeStep(result);

		switch(result.type) {
		case SolverGrid::ResultType::Invalid:
			return setResultMemo(0);
		case SolverGrid::ResultType::Solved:
			return setResultMemo(1);
		case SolverGrid::ResultType::MadeMoves:
			continue;
		case SolverGrid::ResultType::TimeToGuess:
			if(_randomize)
				shuffle(result.smallestTuple, result.smallestTuple + result.tupleSize, _randomEngine);

			int solutions = 0;
			rep(i, result.tupleSize) {
				auto p = result.smallestTuple[i];
				SolverGrid copied = grid;
				copied.assign(p.cell, p.digit);
				int childSolutions = dfs(copied);

				if(_findSolution && childSolutions > 0) {
					grid = copied;
					return 1;
				}

				solutions += childSolutions;
				if(solutions > 1)
					return setResultMemo(2);
			}
			return setResultMemo(solutions);
		}
	}
}
