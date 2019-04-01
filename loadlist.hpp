#pragma once
#include "mask81.hpp"
#include <string>
#include <vector>
#include <unordered_set>

void loadHintMasks(const std::string &maskListFilename, const std::string &maskListCacheFilename, std::vector<Mask81> &hintMasks);
void loadSolutions(const std::string &solutionListFilename, const std::string &solutionListCacheFilename, const std::string &knownProblemListCacheFilename, std::vector<std::string> &solutions, std::unordered_set<std::string> &knownProblemSet);
