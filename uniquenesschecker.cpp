#include "uniquenesschecker.hpp"
#include "gridbase.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <random>
#if defined(NDEBUG) && defined(MY_LOCAL_RUN)
#undef NDEBUG
#endif
#include <cassert>

UniquenessChecker::UniquenessChecker() {
	FastGrid::initMasks();
	_hasOriginalSolution = false;
}

void UniquenessChecker::setSolution(const char *solution, size_t memoSize) {
	for(int i = 0; i < 81; ++ i) {
		if(solution[i] < '1' || solution[i] > '9')
			std::abort();
		_originalSolution[i] = solution[i] - '1';
	}
	_hasOriginalSolution = true;
	_memoTable.init(memoSize);
}

void UniquenessChecker::unsetSolution(size_t memoSize) {
	_hasOriginalSolution = false;
	_memoTable.init(memoSize);
}

bool UniquenessChecker::isSolutionUnique(Mask128 mask) {
	CheckState state{};
	while(mask) {
		Mask128 lb = mask.getLowestBit();
		int cell = lb.getBitPos();
		assignDigit(cell, _originalSolution[cell], state);
		mask ^= lb;
	}
	return checkRec(state) == 1;
}

int UniquenessChecker::countSolutions(const char *problem) {
	CheckState state{};
	for(int cell = 0; cell < 81; ++ cell) {
		if(auto d = parseHint(problem[cell])) {
			assignDigit(cell, d.get(), state);
		}
	}
	return checkRec(state);
}

int UniquenessChecker::checkRec(CheckState &state) {
	while(1) {
		int flag = resolveSolvedSquares(state);
		if(flag == 0)
			break;
	}

	uint64_t hash = state.computeHash();
	if(auto it = _memoTable.find(hash))
		return *it;

	const auto setResultMemo = [&](int result) {
		_memoTable.insert(hash, result);
		return result;
	};
	
	{
		int flag = resolveHiddenSingles(state);
		if(flag != 0) {
			int result = checkRec(state);
			return setResultMemo(result);
		}
	}

	int status = checkStatus(state);
	if(status <= 0) {
		int result;
		if(status < 0) {
			result = 0;
		} else if(_hasOriginalSolution) {
			bool isDifferent = std::memcmp(state.solvedDigits, _originalSolution, sizeof state.solvedDigits) != 0;
			result = isDifferent ? 2 : 1;
		} else {
			result = 1;
		}
		return setResultMemo(result);
	}

	int minSize = 10;
	std::pair<int, int> minTuple[9];

	{
		Mask128 cntx1 = state.grid.digitMasks[0], cntx2(0), cntx4(0);
		for(int d = 1; d < 9; ++ d) {
			Mask128 add = state.grid.digitMasks[d], carry;
			carry = cntx1 & add; cntx1 ^= add; add = carry;
			carry = cntx2 & add; cntx2 ^= add; add = carry;
			carry = cntx4 & add; cntx4 ^= add; add = carry;
		}
		Mask128 cnt2mask = ~cntx1 & cntx2 & ~cntx4;
		Mask128 cnt3mask =  cntx1 & cntx2 & ~cntx4;
		if(cnt2mask | cnt3mask) {
			int cell = (cnt2mask ? cnt2mask : cnt3mask).getBitPos();
			minSize = 0;
			for(int d = 0; d < 9; ++ d) if(state.grid.digitMasks[d].get(cell))
				minTuple[minSize ++] = { cell, d };
			assert(minSize == (cnt2mask ? 2 : 3));
			if(minSize == 2)
				goto endFindingMinTuple;
		}
	}

	for(int d = 0; d < 9; ++ d) {
		Mask128 digitMask = state.grid.digitMasks[d];
		if(!digitMask) continue;
		for(const Mask128 &mask : FastGrid::unitMasks) {
			Mask128 tuple = digitMask & mask;
			if(!tuple) continue;
			int size = tuple.countOneBits();
			if(minSize > size) {
				minSize = size;
				for(int k = 0; k < size; ++ k) {
					Mask128 lb = tuple.getLowestBit();
					minTuple[k] = { lb.getBitPos(), d };
					tuple ^= lb;
				}
				if(size == 2)
					goto endFindingMinTuple;
			}
		}
	}

endFindingMinTuple:
	assert(2 <= minSize && minSize <= 9);

	int differentBranches = 0;
	for(int k = 0; k < minSize; ++ k) {
		const auto &p = minTuple[k];
		if(_originalSolution[p.first] != p.second)
			std::swap(minTuple[differentBranches ++], minTuple[k]);
	}

	int total = 0;
	for(int k = 0; k < minSize; ++ k) {
		int cell = minTuple[k].first, digit = minTuple[k].second;

		CheckState childState = state;
		assignDigit(cell, digit, childState);
		total += checkRec(childState);
		if(total >= 2)
			return setResultMemo(total);
	}

	return setResultMemo(total);
}

int UniquenessChecker::resolveSolvedSquares(CheckState &state) {
	Mask128 mask = state.grid.getSolvedSquares();
	int flag = 0;
	while(mask) {
		Mask128 lb = mask.getLowestBit();
		for(int d = 0; d < 9; ++ d) if(state.grid.digitMasks[d] & lb)
			flag |= assignDigit(lb.getBitPos(), d, state);
		mask ^= lb;
	}
	return flag;
}

int UniquenessChecker::resolveHiddenSingles(CheckState &state) {
	int flag = 0;

	for(int d = 0; d < 9; ++ d) {
		const uint64_t *pdata = state.grid.digitMasks[d].data;
		uint64_t data0 = pdata[0], data1 = pdata[1];

		auto resolve64 = [&](uint64_t mask) {
			if(mask != 0 && (mask & (mask - 1)) == 0) {
				int cell = Mask128::bsf(mask);
				flag |= assignDigit(cell, d, state);
				data0 = pdata[0], data1 = pdata[1];
			}
		};

		auto resolve128 = [&](uint64_t mask0, uint64_t mask1) {
			if(mask0 != 0) {
				if(mask1 == 0 && (mask0 & (mask0 - 1)) == 0) {
					int cell = Mask128::bsf(mask0);
					flag |= assignDigit(cell, d, state);
					data0 = pdata[0], data1 = pdata[1];
				}
			} else {
				if(mask1 != 0 && (mask1 & (mask1 - 1)) == 0) {
					int cell = 64 + Mask128::bsf(mask1);
					flag |= assignDigit(cell, d, state);
					data0 = pdata[0], data1 = pdata[1];
				}
			}
		};

		//row
		{
			if((data0 | data1) == 0) continue;
			const uint64_t X = 0x1ff;

			resolve64(data0 & (X << 0));
			resolve64(data0 & (X << 9));
			resolve64(data0 & (X << 18));
			resolve64(data0 & (X << 27));
			resolve64(data0 & (X << 36));
			resolve64(data0 & (X << 45));
			resolve64(data0 & (X << 54));
			resolve128(data0 & (X << 63), data1 & (X >> 1));
			resolve128(0, data1 & (X << 8));
		}
		//col
		{
			if((data0 | data1) == 0) continue;
			const uint64_t X = 0x8040201008040201ULL, Y = 0x201;

			resolve128(data0 & (X << 0), data1 & (Y >> 1));
			resolve128(data0 & (X << 1), data1 & (Y << 0));
			resolve128(data0 & (X << 2), data1 & (Y << 1));
			resolve128(data0 & (X << 3), data1 & (Y << 2));
			resolve128(data0 & (X << 4), data1 & (Y << 3));
			resolve128(data0 & (X << 5), data1 & (Y << 4));
			resolve128(data0 & (X << 6), data1 & (Y << 5));
			resolve128(data0 & (X << 7), data1 & (Y << 6));
			resolve128(data0 & (X << 8), data1 & (Y << 7));
		}
		//block
		{
			if((data0 | data1) == 0) continue;
			const uint64_t X = 0x1c0e07;

			resolve64(data0 & (X << 0));
			resolve64(data0 & (X << 3));
			resolve64(data0 & (X << 6));
			resolve64(data0 & (X << 27));
			resolve64(data0 & (X << 30));
			resolve64(data0 & (X << 33));
			resolve128(data0 & (X << 54), data1 & (X >> 10));
			resolve128(data0 & (X << 57), data1 & (X >> 7));
			resolve128(data0 & (X << 60), data1 & (X >> 4));
		}
	}
	return flag;
}

int UniquenessChecker::checkStatus(CheckState &state) {
	Mask128 unassigned = state.grid.getUnassignedMask();
	int count = 0;
	for(int cell = 0; cell < 81; ++ cell) {
		if(state.solvedDigits[cell] == uint8_t(-1)) {
			if(!unassigned.get(cell))
				return -1;
			++ count;
		}
	}
	return count;
}

int UniquenessChecker::assignDigit(int cell, int digit, CheckState &state) {
	assert(state.grid.digitMasks[digit].get(cell));
	assert(state.solvedDigits[cell] == uint8_t(-1));

	state.grid.assign(cell, digit);

	state.solvedDigits[cell] = digit;

	state.solvedDigitsHash ^= FastGrid::cellHashCoeffs[cell][digit];

	return 1;
}
