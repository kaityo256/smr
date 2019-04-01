#pragma once
#include "gridbase.hpp"

class SolverGrid : public GridBase {
	Mask _cellMasks[81];
	Mask _unitMasks[27][9];
	bool _invalid;
	Hash _hash;

public:
	SolverGrid() {}

	void init();
	void load(const char *problem);

	bool isInvalid() const { return _invalid; }

	void assign(int cell, int d);
	void eliminate(int cell, int d);
	void applyMask(int cell, Mask mask);

	bool isDetermined(int cell) const;
	Mask getCellMask(int cell) const { return _cellMasks[cell]; }
	Mask getUnitMask(int unit, int d) const { return _unitMasks[unit][d]; }
	int getSolutionDigit(int cell) const;

	Hash getHash() const { return _hash; }

	void debugCheckValidity() const;
	void debugCheckUnitMasks() const;

	void debugOut() const;

	enum class ResultType {
		TimeToGuess,
		MadeMoves,
		Invalid,
		Solved,
	};

	struct TupleElement {
		unsigned char cell, digit;
		TupleElement() {}
		TupleElement(int cell, int digit) : cell(cell), digit(digit) {}
	};

	struct StepResult {
		ResultType type;
		int tupleSize;
		TupleElement smallestTuple[9];
	};

	void makeStep(StepResult &result);
};
