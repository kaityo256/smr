#pragma once

#include <string>

namespace utilforcommands {

void help(std::string message = "");
void checkArgument(bool b, std::string message = "");

void checkInput(bool b, const std::string &input);

int parseInt(const std::string &s, int L, int U);

void checkProblem(const std::string &problem);

void checkSolution(const std::string &solution);
}
