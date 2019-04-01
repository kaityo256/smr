#include "uafinder.hpp"
#include "solvergrid.hpp"
#include "util.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <vector>
#include <array>

using namespace std;

void UAFinder::findAll(const char *solution, int sizeLimit) {
	init(solution);
	_sizeLimit = sizeLimit;
	SolverGrid grid;
	grid.init();
	dfs(grid);
}

void UAFinder::findForSpecificRegion(const Mask81 &region, int sizeLimit) {
	_sizeLimit = sizeLimit;
	SolverGrid grid;
	grid.init();
	rep(cell, 81) if(!region.get(cell))
		grid.applyMask(cell, _solutionMask[cell]);
	dfs(grid);
}

void UAFinder::init(const char *solution) {
	SolverGrid::makeSolutionMask(solution, _solutionMask);
	_uaSets.assign(82, vector<Mask81>());
	_intersectionMask.clear();

	_sizeLimit = -1;
}

int UAFinder::getCount() const {
	int res = 0;
	for(const auto &v : _uaSets)
		res += (int)v.size();
	return res;
}

void UAFinder::saveTo(std::ostream &os) const {
	int maxSize = (int)_uaSets.size() - 1;
	while(maxSize > 0 && _uaSets[maxSize].empty()) -- maxSize;
	os << maxSize << '\n';
	for(int size = 0; size <= maxSize; ++ size) {
		os << _uaSets[size].size() << '\n';
		for(const auto &s : _uaSets[size]) {
			rep(cell, 81) if(s.get(cell))
				os << cell << " ";
			os << '\n';
		}
		os << '\n';
	}
	os.flush();
}

void UAFinder::loadFrom(std::istream &is) {
	_uaSets.assign(82, vector<Mask81>());
	int maxSize;
	is >> maxSize;
	for(int size = 0; size <= maxSize; ++ size) {
		int count;
		is >> count;
		_uaSets[size].resize(count);
		rep(i, count) {
			Mask81 mask; mask.clear();
			rep(j, size) {
				int cell;
				is >> cell;
				mask.set(cell);
			}
			_uaSets[size][i] = mask;
		}
	}
}

void UAFinder::dfs(SolverGrid &grid) {
	while(1) {
		Mask81 uaSet;
		int lowerBound, uaSize;
		int difference = checkDifference(grid, uaSet, uaSize, lowerBound);

		if(difference == 99) {
//			cerr << "too much difference" << endl;
			return;
		}

		if(difference == _sizeLimit) {
			rep(cell, 81) if(!uaSet.get(cell)) {
				grid.applyMask(cell, _solutionMask[cell]);
				if(grid.isInvalid())
					break;
			}
		}

		SolverGrid::StepResult result;
		grid.makeStep(result);

		switch(result.type) {
		case SolverGrid::ResultType::Invalid:
			return;
		case SolverGrid::ResultType::Solved:
			assert(lowerBound == 0);
			if(uaSize > 0) {
				addUASet(uaSize, uaSet);
			}
			return;
		case SolverGrid::ResultType::MadeMoves:
			continue;
		case SolverGrid::ResultType::TimeToGuess:
			int size = result.tupleSize;
			auto tuple = result.smallestTuple;
			int solutionBranches = 0;
			rep(i, size) {
				auto p = tuple[i];
				if((_solutionMask[p.cell] >> p.digit & 1) || _intersectionMask.get(p.cell))
					swap(tuple[solutionBranches ++], tuple[i]);
			}
			rep(i, size) {
				auto p = tuple[i];
				SolverGrid copied = grid;
				copied.assign(p.cell, p.digit);
				dfs(copied);
			}
			return;
		}
	}
}


int UAFinder::checkDifference(const SolverGrid &grid, Mask81 &uaSet, int &uaSize, int &lowerBound) {
	int difference = 0;
	uaSet.clear();
	uaSize = 0;
	char digitCount[9] = {};
	char unitCount[27] = {};
	rep(cell, 81) {
		SolverGrid::Mask mask = grid.getCellMask(cell);
		if((mask & _solutionMask[cell]) == 0) {
			uaSet.set(cell);
			++ uaSize;
			int d = findFirstBitPos(_solutionMask[cell]);
			rep(type, 3) {
				int unit = SolverGrid::cellUnit(cell, type);
				++ unitCount[unit];
			}
			assert(_solutionMask[cell] == 1 << d);
			++ digitCount[d];
			if(_intersectionMask.get(cell))
				continue;
			++ difference;
		}
	}

	lowerBound = 0;
	rep(type, 3) {
		int cnt = 0;
		rep(i, 9) {
			cnt += unitCount[type * 9 + i] == 1;
		}
		if(lowerBound < cnt)
			lowerBound = cnt;
	}
	{
		int cnt = 0;
		rep(i, 9)
			cnt += digitCount[i] == 1;
		if(lowerBound < cnt)
			lowerBound = cnt;
	}

	if(uaSize + lowerBound > _sizeLimit)
		return 99;

	for(int thatSize = 4; thatSize <= uaSize; ++ thatSize) {
		for(const Mask81 &that : _uaSets[thatSize]) {
			if(that.isSubsetOf(uaSet))
				return 99;
		}
	}

	return difference;
}

bool UAFinder::addUASet(int size, const Mask81 &uaSet) {
	_uaSets[size].push_back(uaSet);
	for(int thatSize = size + 1; thatSize < (int)_uaSets.size(); ++ thatSize) {
		vector<Mask81> &list = _uaSets[thatSize];
		list.erase(remove_if(list.begin(), list.end(),
			[uaSet](const Mask81 &that) {
			return uaSet.isSubsetOf(that);
		}), list.end());
	}
	return true;
}
