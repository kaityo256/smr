#include "fastrater.hpp"
#include <iostream>
#include <cassert>

void FastRater::setSolution(const char *solution) {
	for(int i = 0; i < 81; ++ i) {
		if(solution[i] < '1' || solution[i] > '9')
			std::abort();
		_originalSolution[i] = solution[i] - '1';
	}
}

FastRater::Score FastRater::rateHintMask(Mask128 mask) {
	RaterState state{};
	while(mask) {
		Mask128 lb = mask.getLowestBit();
		int cell = lb.getBitPos();
		assignDigit(cell, _originalSolution[cell], state);
		mask ^= lb;
	}
	return mainLoop(state);
}

void FastRater::assignDigit(int cell, int digit, RaterState &state) {
	assert(state.grid.digitMasks[digit].get(cell));
	assert(state.solvedDigits[cell] == uint8_t(-1));

	state.grid.assign(cell, digit);

	state.solvedDigits[cell] = digit;
}

FastRater::Score FastRater::mainLoop(RaterState &/*state*/) {
	while(1) {

	}
	return Score();
}
