#pragma once
#include "mask128.hpp"
#include <cstdint>
#include <mutex>

struct FastGrid {
	Mask128 digitMasks[9];

	void init();
	void assign(int cell, int digit);

	Mask128 getUnassignedMask() const;
	Mask128 getSolvedSquares() const;

	uint64_t computeHash() const;

	static void initMasks();
	static Mask128 mask81;
	static Mask128 unitMasks[27];
	static Mask128 nonadjacencyMasks[81];	//そのセルと同じユニットに属*さない*セル
	static Mask128 hashCoeffs[10];
	static uint64_t cellHashCoeffs[81][9];

private:
	static std::mutex _initMasksMutex;
};
