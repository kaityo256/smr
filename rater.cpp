#include "util.hpp"
#include "rater.hpp"
#include <iostream>
#include <vector>
#include <tuple>
#include <memory>
#include <algorithm>
#include <random>
#include <cassert>

using namespace std;

void Rater::GsfGrid::init() {
	std::fill(_cells, _cells + 81, FullMask());
	std::fill(_units, _units + 27, FullMask());
	rep(unit, 27) rep(d, 9)
		_unitMasks[unit][d] = FullMask();
	_moves.clear();
	initData();
	_hash = getInitHashCellsAndUnits();
}

void Rater::GsfGrid::load(const char * problem) {
	init();
	rep(i, 81) {
		char c = problem[i];
		if(c == '0') {
		}else if('1' <= c && c <= '9') {
			int d = c - '1';
			assert(0 <= d && d < 9);
			assignInstantly(i, d);
		} else {
			cerr << "error while loding: " << problem << endl;
			abort();
		}
	}
}

Rater::GsfGrid Rater::GsfGrid::clone() const {
	GsfGrid g = *this;
	g._moves.clear();
	return g;
}

void Rater::GsfGrid::assign(int cell, int digit) {
	assert(!isDetermined(cell));
	_moves.push_back({ Move::ASSIGN, cell, digit });
}

void Rater::GsfGrid::eliminate(int cell, int digit) {
	assert(!isDetermined(cell));
	_moves.push_back({ Move::ELIMINATE, cell, digit });
}

//sudoku.cのものと返り値の意味が違うので注意
int Rater::GsfGrid::commit() {
	int moves = 0;
	rep(i, _moves.size()) {
		const Move &move = _moves[i];
		bool unique = true;
		rep(j, i)
			unique &= _moves[j] != move;
		if(!unique) continue;
		int cell = move.cell, digit = move.digit;
		++moves;
		if(isDetermined(cell))
			continue;
		Mask mask = 0;
		switch(move.type) {
		case Move::ASSIGN:
			mask = 1 << digit;
			break;
		case Move::ELIMINATE:
			mask = ~(1 << digit);
			break;
		}
		Mask changed = _cells[cell] & ~mask;
		_cells[cell] &= mask;
		updateUnitMask(cell, changed);
		changeCellHash(cell, changed);
		Mask t = _cells[cell];
		if(t != 0 && (t & (t - 1)) == 0)
			propagateMask(cell, t);
	}
	_moves.clear();
#if 0
	rep(unit, 27) rep(d, 9) rep(index, 9) {
		int cell = unitCell(unit, index);
		bool a = getUnitMask(unit, d) >> index & 1;
		bool b = getUnderminedMask(cell) >> d & 1;
		b &= (_cells[cell] & (_cells[cell] - 1)) != 0;
		assert(a == b);
	}
#endif
#if 0
	Hash h = 0;
	rep(cell, 81) for(int d : EachBit(_cells[cell]))
		h ^= _hashCoeffs[cell][d];
	rep(unit, 27) for(int d : EachBit(_units[unit]))
		h ^= _hashCoeffs[81 + unit][d];
	if(h != _hash) cerr << "err" << endl;
#endif
	return moves;
}

void Rater::GsfGrid::assignInstantly(int cell, int digit) {
	assert(!isDetermined(cell));
	setMask(cell, 1 << digit);
}

void Rater::GsfGrid::updateUnitMaskSub(int unit, int index, Mask undeterminedMask, Mask changedMask) {
	for(int d : EachBit(changedMask)) {
		bool b = undeterminedMask >> d & 1;
		_unitMasks[unit][d] = (_unitMasks[unit][d] & ~(1 << index)) | (b << index);
	}
}

void Rater::GsfGrid::updateUnitMask(int cell, Mask changedMask) {
	int row = cell / 9, col = cell % 9, block = row / 3 * 3 + col / 3;
	int blockindex = (row % 3) * 3 + (col % 3);
	Mask cellMask = _cells[cell];
	Mask undeterminedMask;
	if(isSingleBitOrZero(cellMask)) {
		undeterminedMask = 0;
	} else {
		undeterminedMask = cellMask;
		undeterminedMask &= _units[row];
		undeterminedMask &= _units[9 + col];
		undeterminedMask &= _units[18 + block];
	}
	updateUnitMaskSub(row, col, undeterminedMask, changedMask);
	updateUnitMaskSub(9 + col, row, undeterminedMask, changedMask);
	updateUnitMaskSub(18 + block, blockindex, undeterminedMask, changedMask);
}

void Rater::GsfGrid::changeCellHash(int cell, Mask changed) {
	for(int d : EachBit(changed))
		_hash ^= getCellHashCoeff(cell, d);
}

void Rater::GsfGrid::changeUnitHash(int unit, Mask changed) {
	for(int d : EachBit(changed))
		_hash ^= getUnitHashCoeff(unit, d);
}

void Rater::GsfGrid::setMask(int cell, Mask mask) {
	Mask changed = _cells[cell] ^ mask;
	_cells[cell] = mask;
	updateUnitMask(cell, changed);
	changeCellHash(cell, changed);
	propagateMask(cell, mask);
}

void Rater::GsfGrid::propagateMask(int cell, Mask mask) {
	//xorなので候補が復活することがある。
	//バグ的な挙動だが…
	rep(type, 3) {
		int unit = cellUnit(cell, type);
		_units[unit] ^= mask;
		changeUnitHash(unit, mask);
	}
	rep(type, 3) {
		int unit = cellUnit(cell, type);
		rep(index, 9) {
			int cell2 = unitCell(unit, index);
			updateUnitMask(cell2, mask);
		}
	}
}

Rater::GsfGrid::Mask Rater::GsfGrid::getUnderminedMask(int cell) const {
	Mask mask = _cells[cell];
	rep(type, 3)
		mask &= _units[cellUnit(cell, type)];
	return mask;
}

bool Rater::GsfGrid::isEmpty(int cell) const {
	return getCellMask(cell) == 0;
}

bool Rater::GsfGrid::isDetermined(int cell) const {
	Mask mask = _cells[cell];
	return isSingleBitOrZero(mask);
}

bool Rater::GsfGrid::isSingleton(int cell) {
	Mask t = getCellMask(cell);
	return t != 0 && isSingleBitOrZero(t);
}

int Rater::GsfGrid::getDigit(int cell) const {
	Mask t = getCellMask(cell);
	assert(t != 0);
	return findFirstBitPos(t);
}

Rater::GsfGrid::Mask Rater::GsfGrid::getCellMask(int cell) const {
	if(!isDetermined(cell))
		return getUnderminedMask(cell);
	else
		return _cells[cell];
}

Rater::GsfGrid::Mask Rater::GsfGrid::getUnitMask(int unit, int d) const {
	return _unitMasks[unit][d];
}

bool Rater::GsfGrid::isPossible(int cell, int d) const {
	return getCellMask(cell) >> d & 1;
}

int Rater::GsfGrid::getNumMoves() const {
	return (int)_moves.size();
}

void Rater::GsfGrid::setNumMoves(int size) {
	assert(size <= getNumMoves());
	_moves.resize(size);
}

void Rater::GsfGrid::copyMoves(const GsfGrid & that) {
	_moves.insert(_moves.end(), that._moves.begin(), that._moves.end());
}

Rater::GsfGrid::Hash Rater::GsfGrid::getHash() const {
	return _hash;
}

//hidden singles
int Rater::constraint_N(GsfGrid &grid) {
	int moves = 0;
	rep(unit, 27) rep(d, 9) {
		GsfGrid::Mask mask = grid.getUnitMask(unit, d);
		if(mask == 0 || (mask & (mask - 1)) != 0) continue;
		int index = findFirstBitPos(mask);
		int cell = GsfGrid::unitCell(unit, index);
		grid.assign(cell, d);
		++moves;
	}
	return moves;
}

//Box claim (box-line / line-box reduction)
//あるユニット内と数字に対し、その候補が他の1つのユニットに収まる場合は、そっちのユニット内のその数字をeliminateできる
int Rater::constraint_B(GsfGrid &grid) {
	return GsfGrid::processBoxLineReduction(
		[&grid](int unit, int d) { return grid.getUnitMask(unit, d); },
		[&grid](int cell, int d) { grid.eliminate(cell, d); }
	);
}

//「セル内にn個の候補が残っている」または
//「ユニット内にある数字の候補場所がn個ある」
//をまとめてn-tupleと呼ぶ。
//つまり、guessするときに n択 となるもの。
void Rater::makeTupleList(const GsfGrid &grid, TupleList &tupleList) {
	bool claimPattern[512] = {};
	for(int i = 0; i < 9; i += 3) {
		claimPattern[1 << (i + 0) | 1 << (i + 1)] = true;
		claimPattern[1 << (i + 0) | 1 << (i + 2)] = true;
		claimPattern[1 << (i + 1) | 1 << (i + 2)] = true;
		claimPattern[1 << (i + 0) | 1 << (i + 1) | 1 << (i + 2)] = true;
	}

	tupleList.init();
	//cell tuples
	rep(cell, 81) if(!grid.isDetermined(cell)) {
		int count = 0;
		rep(d, 9) if(grid.isPossible(cell, d))
			++count;
		if(count == 1) continue;
		rep(d, 9) if(grid.isPossible(cell, d))
			tupleList.tuples[count].push_back(TupleEntry{ cell, d });
	}
	//unit tuples
	rep(d, 9) rep(unit, 27) {
		int count = 0;
		unsigned pattern = 0;
		rep(i, 9) if(grid.isPossible(GsfGrid::unitCell(unit, i), d)) {
			++count;
			pattern |= 1U << i;
		}
		//constraint_N と constraint_B の対象になるものは除かれる
		if(count <= 1) continue;
		if(unit < 18 && claimPattern[pattern]) continue;
		rep(i, 9) {
			int cell = GsfGrid::unitCell(unit, i);
			if(grid.isPossible(cell, d))
				tupleList.tuples[count].push_back(TupleEntry{ cell, d });
		}
	}
}

//各w-tupleに対し、guessした結果を基本テクニックのみで解いてみる。
//
int Rater::propstep(GsfGrid &grid, TupleList &tupleList, TupleList::SeenMemo &seenMemo, Counters &counters, int w, bool nested) {
	assert(grid.getNumMoves() == 0);

	int num = tupleList.getNumEntries(w) / w;
	bool error = false;
	rep(i, num) {
		int remCandidates = w;
		rep(j, w) {
			TupleEntry &entry = tupleList.tuples[w][i * w + j];
			int cell = entry.cell, digit = entry.digit;
			if(grid.isDetermined(cell)) {
				if(grid.getDigit(cell) == digit) {
					remCandidates = -1;
					break;
				} else {
					--remCandidates;
				}
			} else if(!grid.isPossible(cell, digit)) {
				--remCandidates;
			}
		}
		if(remCandidates == 0) {
			error = true;
			continue;
		} else if(remCandidates == -1) {
			continue;
		}
		rep(j, w) {
			const TupleEntry &entry = tupleList.tuples[w][i * w + j];
			int cell = entry.cell, digit = entry.digit;
			if(grid.isDetermined(cell) || !grid.isPossible(cell, digit))
				continue;
			switch(seenMemo.get(cell, digit)) {
			case TupleList::Solution:
				break;
			case TupleList::Unknown:
				break;
			case TupleList::Error:
				--remCandidates;
				break;
			case TupleList::NoInfo:
				++counters.propositions;
				GsfGrid cloned = grid.clone();
				cloned.assignInstantly(cell, digit);
				unsigned iterations;
				Result result = propagate(cloned, iterations, true);
				counters.iterations += iterations;
				if(result == Result::Solved) {
					++counters.solutions;
					grid.assign(cell, digit);
					seenMemo.set(cell, digit, TupleList::Solution);
					j = w;	//ループを抜ける(このw-tupleを終了する)
				} else if(result == Result::Invalid) {
					--remCandidates;
					if(!nested) {
						++counters.contradictions;
						grid.eliminate(cell, digit);
					}
					seenMemo.set(cell, digit, TupleList::Error);
				} else {
					seenMemo.set(cell, digit, TupleList::Unknown);
				}
				break;
			}
		}
		if(remCandidates == 0)
			error = true;
	}
	if(error) {
		//moveを全て消す
		grid.setNumMoves(0);
		return -1;
	} else {
		return grid.getNumMoves();
	}
}

int Rater::constraint_P(GsfGrid &grid) {
	assert(grid.getNumMoves() == 0);

	TupleList tupleList;
	makeTupleList(grid, tupleList);

//	int w1 = -1, w2 = -1;

	Counters counters;
	int firstMoves = 0;
	TupleList::SeenMemo firstSeenMemo;
	for(int w = 2; w <= 9; ++w) {
		firstMoves = propstep(grid, tupleList, firstSeenMemo, counters, w, false);
		if(firstMoves == -2)
			return -2;
		if(firstMoves != 0) {
//			w1 = w;
			goto done;
		}
	}

	//2段階のguess
	counters = Counters();
	for(int m = 2; m <= 9; ++m) {
		int num = tupleList.getNumEntries(m) / m;
		vector<TupleList::SeenMemo> seenMemos(num * m);
		for(int w = 2; w <= m; ++w) {
			rep(i, num) rep(j, m) {
				const TupleEntry &entry = tupleList.tuples[m][i * m + j];
				TupleList::SeenMemo &seenMemo = seenMemos[i * m + j];
				int cell = entry.cell, digit = entry.digit;
				switch(seenMemo.get(cell, digit)) {
				case TupleList::Solution:
					break;
				case TupleList::Error:
					break;
				default:
					GsfGrid cloned = grid.clone();
					cloned.assignInstantly(cell, digit);
					//propstepの先で正解に到達したかの判定用
					int saved_solutions = counters.solutions;
					int k = propstep(cloned, tupleList, seenMemo, counters, w, true);
					if(k == -2)
						return -2;
					//propstepでcloned gridに対してしたmoveをコピーする
					grid.copyMoves(cloned);
					if(k < 0) {
						++counters.contradictions;
						grid.eliminate(cell, digit);
						seenMemo.set(cell, digit, TupleList::Error);
					} else if(k == 0) {
						seenMemo.set(cell, digit, TupleList::Unknown);
					} else if(saved_solutions < counters.solutions) {
						grid.assign(cell, digit);
						seenMemo.set(cell, digit, TupleList::Solution);
						j = m;	//ループを抜ける(このw-tupleを終了する)
					}
					break;
				}
			}
//			w1 = m, w2 = w;
			if(grid.getNumMoves() > 0)
				goto done;
		}
	}
done:
	long long work = counters.calculateWork();
//	cerr << "work = " << work << ", iterations = " << counters.iterations << ", propositions = " << counters.propositions << ", solutions = " << counters.solutions << ", contradictions = " << counters.contradictions << "\n";
//	cerr << "w = " << w1 << ", " << w2 << '\n';
	_totalWork += work;
	return grid.getNumMoves();
}

Rater::Result Rater::propagate(GsfGrid &grid, unsigned &iterations, bool nested) {
	vector<tuple<GsfGrid::Hash, unsigned, long long> > hashHistory;

	auto setResultMemo = [this, &grid, nested, &iterations, &hashHistory](Result result) -> Result {
		for(auto p : hashHistory) {
			PropagateResult r;
			r.result = result;
			r.iterations = iterations - std::get<1>(p);
			r.totalWork = _totalWork - std::get<2>(p);
			_resultMemo.insert(std::get<0>(p), r);
		}
		hashHistory.clear();
		return result;
	};

	iterations = 0;
	do {
		{
			GsfGrid::Hash gridHash = grid.getHash();
			if(!nested) gridHash ^= 1;
			PropagateResult *r = _resultMemo.find(gridHash);
			if(r != nullptr) {
				iterations += r->iterations;
				_totalWork += r->totalWork;
				setResultMemo(r->result);
				grid.init();
				return r->result;
			}
			hashHistory.emplace_back(gridHash, iterations, _totalWork);
		}

		int undeterminedCells = 0;
		int moves = 0;
		rep(cell, 81) {
			if(grid.isDetermined(cell))
				continue;
			GsfGrid::Mask cellMask = grid.getCellMask(cell);
			if(cellMask == 0) {
				return setResultMemo(Result::Invalid);
			} else if((cellMask & (cellMask - 1)) == 0) {
				grid.assign(cell, findFirstBitPos(cellMask));
				++moves;
			} else {
				++undeterminedCells;
			}
		}

		if(iterations >= 0xffffffffU) {
			cerr << "iterations >= 0xffffffffU" << endl;
			break;
		}
		++iterations;

		if(undeterminedCells == 0 && moves == 0) {
			//verify
			GsfGrid g; g.init();
			rep(cell, 81) {
				int digit = grid.getDigit(cell);
				if(!g.isPossible(cell, digit)) {
					return setResultMemo(Result::Invalid);
				}
				g.assignInstantly(cell, digit);
			}
			return setResultMemo(Result::Solved);
		}

		moves += constraint_N(grid);
		if(moves == 0) {
			moves += constraint_B(grid);
			if(moves == 0 && !nested) {
				moves += constraint_P(grid);
			}
		}
	} while(grid.commit() > 0);

	rep(unit, 27) {
		GsfGrid::Mask mask = 0;
		rep(index, 9) {
			int cell = GsfGrid::unitCell(unit, index);
			mask |= grid.getCellMask(cell);
		}
		if(!GsfGrid::isFullMask(mask)) {
			return setResultMemo(Result::Invalid);
		}
	}

	return setResultMemo(Result::Unknown);
}

Rater::Score Rater::rate(const char *problem) {
	GsfGrid grid;
	grid.load(problem);
	unsigned iterations;
	_totalWork = 0;
	Result result = propagate(grid, iterations, false);
	if(result == Result::Invalid)
		return -1;
	if(_totalWork != 0)
		return _totalWork;
	else
		return iterations - 1;
}

int Rater::squash(long long n) {
	if(n < 0)
		return 0;
	else if(n < 90000)
		return (int)n;
	else if(n > 99999999)
		return (int)(99375 + 624.0 / ((2147483647.0 - 99999999.0) / (n - 99999999.0)));
	else if(n > 9999999)
		return (int)(98750 + 624.0 / ((99999999.0 - 9999999.0) / (n - 9999999.0)));
	else if(n > 999999)
		return (int)(97500 + 1249.0 / ((9999999.0 - 999999.0) / (n - 999999.0)));
	else if(n > 99999)
		return (int)(95000 + 2499.0 / ((999999.0 - 99999.0) / (n - 99999.0)));
	else
		return (int)(90000 + 4999.0 / ((99999.0 - 90000.0) / (n - 90000.0)));
}

static long long cdiv(long long x, int y) { return (x - 1) / y + 1; }

long long Rater::unsquash(int n) {
	if(n < 0)
		return -1;
	else if(n < 90000)
		return n;
	else if(n < 95000)
		return 90000 + cdiv((n - 90000LL) * (99999 - 90000), 4999);
	else if(n < 97500)
		return 99999 + cdiv((n - 95000LL) * (999999 - 99999), 2499);
	else if(n < 98750)
		return 999999 + cdiv((n - 97500LL) * (9999999 - 999999), 1249);
	else if(n < 99375)
		return 9999999 + cdiv((n - 98750LL) * (99999999 - 9999999), 624);
	else 
		return 99999999 + cdiv((n - 99375LL) * (2147483647 - 99999999), 624);
}
