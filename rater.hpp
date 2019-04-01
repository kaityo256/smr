#pragma once
#include "gridbase.hpp"
#include "memotable.hpp"
#include <vector>

class Rater {
public:
	class GsfGrid : public GridBase {
		struct Move {
			enum MoveType {
				ASSIGN,
				ELIMINATE
			} type;
			int cell;
			int digit;

			bool operator!=(const Move &that) const { return type != that.type || cell != that.cell || digit != that.digit; }
		};

		//ÉZÉãÇÃ mask ÇÕ getMask Çí ÇµÇƒéÊìæÇ∑ÇÈÅB
		//ä˘Ç…åàÇ‹Ç¡ÇΩÇ‚Ç¬ÇÕ _cells Ç™ 2^d Ç…Ç»ÇËÅAgetUndeterminedMask Ç≈ÇÕ 0 Ç™ï‘ÇÈ
		Mask _cells[81];
		Mask _units[27];
		Mask _unitMasks[27][9];
		std::vector<Move> _moves;
		Hash _hash;

	public:
		void init();
		void load(const char *problem);
		GsfGrid clone() const;

		void assign(int cell, int digit);
		void eliminate(int cell, int digit);
		int commit();
		void assignInstantly(int cell, int digit);

	private:
		void updateUnitMaskSub(int unit, int index, Mask undeterminedMask, Mask changedMask);
		void updateUnitMask(int cell, Mask changedMask);

		void changeCellHash(int cell, Mask changed);
		void changeUnitHash(int unit, Mask changed);

		void setMask(int cell, Mask mask);

		void propagateMask(int cell, Mask mask);

		Mask getUnderminedMask(int cell) const;

	public:
		bool isEmpty(int cell) const;
		bool isDetermined(int cell) const;
		bool isSingleton(int cell);
		int getDigit(int cell) const;
		bool isPossible(int cell, int d) const;

		Mask getCellMask(int cell) const;
		Mask getUnitMask(int unit, int d) const;

		int getNumMoves() const;
		void setNumMoves(int size);
		void copyMoves(const GsfGrid &that);

	public:
		Hash getHash() const;
	};

private:
	enum class Result {
		Unknown,
		Invalid,
		Solved,
	};

	struct PropagateResult {
		Result result;
		unsigned iterations;
		long long totalWork;
	};

public:
	static int constraint_N(GsfGrid &grid);
	static int constraint_B(GsfGrid &grid);

private:
	struct TupleEntry {
		int cell;
		int digit;
	};
	struct TupleList {
		enum Seen {
			NoInfo,
			Solution,
			Error,
			Unknown
		};

		struct SeenMemo {
			std::vector<Seen> memo;

			SeenMemo() : memo(81 * 9, NoInfo) {}

			Seen get(int cell, int digit) const {
				return memo[cell * 9 + digit];
			}

			void set(int cell, int digit, Seen seen) {
				memo[cell * 9 + digit] = seen;
			}
		};

		std::vector<std::vector<TupleEntry> > tuples;

		void init() {
			tuples.assign(10, std::vector<TupleEntry>());
		}

		int getNumEntries(int n) const {
			return (int)tuples[n].size();
		}
	};

	void makeTupleList(const GsfGrid &grid, TupleList &tupleList);

	struct Counters {
		int iterations;
		int propositions;
		int solutions;
		int contradictions;

		Counters() : iterations(0), propositions(0), solutions(0), contradictions(0) {}

		long long calculateWork() const {
			if(propositions == 0)
				return 0;
			long long work = (long long)(iterations / 100) * propositions / (solutions + contradictions + 1);
			if(work < 1000)
				work = 1000LL * iterations / propositions / (solutions + contradictions + 1);
			return work;
		}
	};

	int propstep(GsfGrid &grid, TupleList &tupleList, TupleList::SeenMemo &seenMemo, Counters &counters, int w, bool nested);
	int constraint_P(GsfGrid &grid);

	Result propagate(GsfGrid &grid, unsigned &iterations, bool nested);

public:
	Rater(size_t resultMemoSize = 10000) {
		_resultMemo.init(resultMemoSize);
	}

	static int squash(long long n);
	static long long unsquash(int n);

	typedef long long Score;
	Score rate(const char *problem);

private:
	long long _totalWork;
	MemoTable<PropagateResult> _resultMemo;
};

