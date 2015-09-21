#pragma once
#include <BWAPI.h>

class State {
public:
  State();
  State(BWAPI::Player player);

  void add(BWAPI::UnitType type, int count = 1);
  void add(BWAPI::TechType type);
  void add(BWAPI::UpgradeType type, int count = 1);
  void expand();
  void remove(BWAPI::UnitType type, int count = 1);
  void remove(BWAPI::TechType type);
  void remove(BWAPI::UpgradeType type, int count = 1);

  void eachUnit(std::function<bool(BWAPI::UnitType, int)> iterator) const;
  void eachTech(std::function<bool(BWAPI::TechType)> iterator) const;
  void eachUpgrade(std::function<bool(BWAPI::UpgradeType, int)> iterator) const;
  void diffTo(const State *rhs);

  int minerals;
  int gas;
  int supply;
  int savedMinerals;
  int savedGas;

private:
  bool expandStep();

  std::map<BWAPI::UnitType, int> units;
  std::set<BWAPI::TechType> techs;
  std::map<BWAPI::UpgradeType, int> upgrades;
};