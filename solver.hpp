#pragma once
#include "rater.hpp"
#include "solvergrid.hpp"
#include "memotable.hpp"
#include <random>

class Solver {
	bool _findSolution;
	bool _randomize;
	MemoTable<int> _resultMemo;
	std::default_random_engine _randomEngine;

public:
	Solver(size_t resultMemoSize = 10000) { _resultMemo.init(resultMemoSize); }

	int solve(const char *problem);
	int solveGrid(SolverGrid grid);
	bool findSolution(const char *problem, char *solution, bool randomize = false);

	void setRandomEngine(std::default_random_engine re) { _randomEngine = re; }
	const std::default_random_engine &getRandomEngine() const { return _randomEngine; }

private:
	int dfs(SolverGrid &grid);
};
