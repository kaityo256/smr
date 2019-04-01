#pragma once
#include "mask128.hpp"
#include "fastgrid.hpp"
#include <cstdint>
#include <cstring>

class FastRater {
public:
	typedef long long Score;
	void setSolution(const char *solution);
	Score rateHintMask(Mask128 mask);

private:
	struct RaterState {
		FastGrid grid;
		uint8_t solvedDigits[81];

		RaterState() {
			grid.init();
			std::memset(solvedDigits, -1, sizeof solvedDigits);
		}
	};

	void assignDigit(int cell, int digit, RaterState &state);
	Score mainLoop(RaterState &state);

	uint8_t _originalSolution[81];
};
