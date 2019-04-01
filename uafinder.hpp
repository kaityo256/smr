#pragma once
#include "mask81.hpp"
#include "solver.hpp"
#include "solvergrid.hpp"
#include <iostream>
#include <vector>

class UAFinder {
public:
  void findAll(const char *solution, int sizeLimit);
  void findForSpecificRegion(const Mask81 &region, int sizeLimit);

  void init(const char *solution);

  const std::vector<Mask81> &getList(int size) const {
    return _uaSets[size];
  }
  int getMaxSize() {
    return (int)_uaSets.size() - 1;
  }
  int getCount() const;

  void saveTo(std::ostream &os) const;
  void loadFrom(std::istream &is);

private:
  SolverGrid::Mask _solutionMask[81];
  Mask81 _intersectionMask;
  std::vector<std::vector<Mask81>> _uaSets;

  int _sizeLimit;

  void dfs(SolverGrid &grid);

  int checkDifference(const SolverGrid &grid, Mask81 &uaSet, int &uaSize, int &lowerBound);

  bool addUASet(int size, const Mask81 &uaSet);
};
