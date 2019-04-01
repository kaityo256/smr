#include "canonicalizer.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <cassert>
using namespace std;

string Canonicalizer::GridPermutation::applyTo(const char *grid) const {
	string res(81, '?');
	rep(i, 81) {
		char c = grid[i];
		res[_cellPerm.get(i)] =
			'1' <= c && c <= '9' ? '1' + _digitPerm[c - '1'] : c;
	}
	return res;
}

Canonicalizer::Canonicalizer() {
	_columnPermutations.resize(Symmetry::COLUMN_PERMUTATIONS_ORDER);
	auto generator = Symmetry::allColumnPermutations();
	copy(generator.begin(), generator.end(), _columnPermutations.begin());
}

Mask81 Canonicalizer::canonicalizeMask(const Mask81 &mask, Symmetry::CellPermutation *resPerm) const {
	vector<int> hintPositions;
	for(int pos : mask)
		hintPositions.push_back(pos);
	array<unsigned, 9> lexMin;
	fill(lexMin.begin(), lexMin.end(), numeric_limits<unsigned short>::max());
	for(const auto &perm : _columnPermutations) {
		array<unsigned, 9> rows = { {} };
		for(int pos : hintPositions) {
			int newPos = perm.get(pos);
			rows[newPos / 9] |= 1 << (8 - newPos % 9);
		}
		int rowSwaps = findLexMinRowPermutation(rows.data());
		if(rows < lexMin) {
			lexMin = rows;
			if(resPerm) {
				*resPerm = perm.inverse();
				applyRowSwaps(*resPerm, rowSwaps);
				*resPerm = resPerm->inverse();
			}
		}
	}
	Mask81 lexMinMask;
	lexMinMask.clear();
	rep(row, 9) for(int col : EachBit(lexMin[row]))
		lexMinMask.set(row * 9 + (8 - col));
	return lexMinMask;
}

string Canonicalizer::canonicalizeSolution(const char *solution, GridPermutation *resPerm) const {
	rep(cell, 81)
		if(!('1' <= solution[cell] && solution[cell] <= '9'))
			abort();
	unsigned bases[10];
	bases[0] = 1;
	rep(i, 9) bases[i + 1] = bases[i] * 10;
	array<unsigned, 9> lexMin;
	fill(lexMin.begin(), lexMin.end(), numeric_limits<unsigned>::max());
	for(const auto &perm : _columnPermutations) {
		char digitPerms[9][9] = {};
		rep(i, 81) {
			int newPos = perm.get(i);
			int row = newPos / 9, col = newPos % 9;
			digitPerms[row][solution[i] - '1'] = col + 1;
		}
		for(const char *digitPerm : digitPerms) {
			assert(count(digitPerm, digitPerm + 9, 0) == 0);
			array<unsigned, 9> rows = { {} };
			rep(i, 81) {
				int newPos = perm.get(i);
				int row = newPos / 9, col = newPos % 9;
				int d = digitPerm[solution[i] - '1'];
				rows[row] += d * bases[8 - col];
			}
			int rowSwaps = findLexMinRowPermutation(rows.data());
			if(rows < lexMin) {
				lexMin = rows;
				if(resPerm) {
					resPerm->_cellPerm = perm.inverse();
					applyRowSwaps(resPerm->_cellPerm, rowSwaps);
					resPerm->_cellPerm = resPerm->_cellPerm.inverse();
					rep(d, 9)
						resPerm->_digitPerm[d] = digitPerm[d] - 1;
				}
			}
		}
	}
	string lexMinSolution(81, '?');
	rep(row, 9) rep(col, 9) {
		int d = lexMin[row] / bases[8 - col] % 10 - 1;
		lexMinSolution[row * 9 + col] = '1' + d;
	}

	return lexMinSolution;
}

string Canonicalizer::canonicalizeProblem(const char *problem, GridPermutation *resPerm) const {
	struct State {
		unsigned short usedRows;
		array<char, 9> digitPerm;
		int determinedDigits;
		array<char, 9> rowOrder;
	};

	rep(cell, 81)
		if(!('0' <= problem[cell] && problem[cell] <= '9'))
			abort();
	array<unsigned, 9> lexMin;
	fill(lexMin.begin(), lexMin.end(), numeric_limits<unsigned>::max());
	vector<State> q, nq;
	for(const auto &perm : _columnPermutations) {
		char columnPermuted[9][9];
		rep(i, 81) {
			int nextPos = perm.get(i);
			columnPermuted[nextPos / 9][nextPos % 9] = problem[i];
		}
		nq.clear();
		nq.push_back(State{ 0, {{}}, 0, {{}} });
		rep(depth, 9) {
			q.swap(nq);
			nq.clear();
			for(const State &s : q) {
				rep(row, 9) if(~s.usedRows >> row & 1) {
					int band = row / 3, bandUsed = s.usedRows >> (band * 3) & 7;
					if(depth % 3 == 0 ? (bandUsed != 0) : (bandUsed == 0 || bandUsed == 7))
						continue;
					State ns = s;
					ns.usedRows |= 1 << row;
					ns.rowOrder[depth] = row;
					unsigned curRow = 0;
					rep(col, 9) {
						int d = columnPermuted[row][col] - '1';
						if(d != -1 && ns.digitPerm[d] == 0)
							ns.digitPerm[d] = 1 + ns.determinedDigits ++;
						curRow = curRow * 10 + (d == -1 ? 0 : ns.digitPerm[d]);
					}
					if(curRow < lexMin[depth]) {
						lexMin[depth] = curRow;
						for(int i = depth + 1; i < 9; ++ i)
							lexMin[i] = numeric_limits<unsigned>::max();
						nq.clear();
						nq.push_back(ns);
					} else if(lexMin[depth] == curRow) {
						nq.push_back(ns);
					}
				}
			}
		}
		if(!nq.empty()) {
			const State &s = nq.front();
			if(resPerm) {
				resPerm->_cellPerm = perm.inverse();
				char rowPos[9];
				rep(i, 9) rowPos[i] = i;
				rep(i, 9) {
					int row = s.rowOrder[i];
					if(rowPos[row] != i) {
						Symmetry::applyRowSwap(i, rowPos[row], resPerm->_cellPerm);
						*find(rowPos, rowPos + 9, i) = rowPos[row];
						rowPos[row] = i;
					}
				}
				resPerm->_cellPerm = resPerm->_cellPerm.inverse();
				int digits = s.determinedDigits;
				rep(d, 9) {
					int t = s.digitPerm[d];
					if(t == 0) t = 1 + digits ++;
					resPerm->_digitPerm[d] = t - 1;
				}
			}
		}
	}
	string lexMinProblem(81, '?');
	rep(row, 9) {
		unsigned x = lexMin[row];
		rep(j, 9) {
			lexMinProblem[row * 9 + (8 - j)] = '0' + x % 10;
			x /= 10;
		}
	}
	return lexMinProblem;
}

int Canonicalizer::findLexMinRowPermutation(unsigned rows[9]) {
	int res = 0;
	res |= sortBand(rows + 0) << 0;
	res |= sortBand(rows + 3) << 3;
	res |= sortBand(rows + 6) << 6;
	res |= sortTwoBands(rows + 0, rows + 3) << 9;
	res |= sortTwoBands(rows + 3, rows + 6) << 10;
	res |= sortTwoBands(rows + 0, rows + 3) << 11;
	return res;
}

int Canonicalizer::sortBand(unsigned x[3]) {
	int res = 0;
	if(x[1] < x[0]) swap(x[0], x[1]), res |= 1;
	if(x[2] < x[1]) swap(x[1], x[2]), res |= 2;
	if(x[1] < x[0]) swap(x[0], x[1]), res |= 4;
	return res;
}

int Canonicalizer::sortTwoBands(unsigned x[3], unsigned y[3]) {
	if(lexicographical_compare(y, y + 3, x, x + 3)) {
		rep(i, 3) swap(x[i], y[i]);
		return 1;
	} else {
		return 0;
	}
}

void Canonicalizer::applyRowSwaps(Symmetry::CellPermutation &perm, int rowSwaps) {
	rep(band, 3) rep(i, 3)
		if(rowSwaps >> (band * 3 + i) & 1)
			Symmetry::applyRowSwap(band * 3 + i % 2, band * 3 + i % 2 + 1, perm);
	rep(i, 3)
		if(rowSwaps >> (9 + i) & 1)
			Symmetry::applyBandSwap(i % 2, i % 2 + 1, perm);
}

ostream &operator<<(ostream & os, const Canonicalizer::GridPermutation &perm) {
	os << perm._cellPerm;
	rep(i, 9) os << ' ' << (int)perm._digitPerm[i];
	return os;
}

istream &operator>>(istream &is, Canonicalizer::GridPermutation &perm) {
	is >> perm._cellPerm;
	rep(i, 9) {
		int x;
		is >> x;
		perm._digitPerm[i] = x;
	}
	return is;
}
