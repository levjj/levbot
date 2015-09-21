#include "State.h"

using namespace BWAPI;

State::State() : minerals(0), gas(0), supply(0), savedMinerals(0), savedGas(0) {}

State::State(Player player) : State() {
  this->savedMinerals = player->minerals();
  this->savedGas = player->gas();
  for (auto &u : player->getUnits()) {
    auto type = u->getType();
    this->add(type);
    if (type == UnitTypes::Protoss_Probe) {
      if (u->isGatheringMinerals()) {
        this->add(UnitTypes::Resource_Mineral_Field);
      }
      if (u->isGatheringGas()) {
        this->add(UnitTypes::Resource_Vespene_Geyser);
      }
      if (u->isConstructing()) {
        this->add(u->getBuildType());
        this->savedMinerals -= u->getBuildType().mineralPrice();
        this->savedGas -= u->getBuildType().gasPrice();
      }
    }
  }
  for (auto &t : TechTypes::allTechTypes()) {
    if (player->hasResearched(t) || player->isResearching(t)) {
      this->add(t);
    }
  }
  for (auto &g : UpgradeTypes::allUpgradeTypes()) {
    if (player->getUpgradeLevel(g) > 0) {
      this->add(g, player->getUpgradeLevel(g));
    }
  }
}

void State::add(UnitType type, int count) {
  this->units[type] += count;
  if (type == UnitTypes::Resource_Mineral_Field || type == UnitTypes::Resource_Vespene_Geyser) {
    type = UnitTypes::Protoss_Probe;
  }
  this->minerals += type.mineralPrice() * count;
  this->gas += type.gasPrice() * count;
  this->supply += type.supplyRequired() * count;
}

void State::add(TechType type) {
  this->techs.insert(type);
  this->minerals += type.mineralPrice();
  this->gas += type.gasPrice();
}

void State::add(UpgradeType type, int count) {
  if (this->upgrades[type] == 0) {
    this->upgrades[type] = 1;
    this->minerals += type.mineralPrice();
    this->gas += type.gasPrice();
  }
  if (this->upgrades[type] == 1 && count > 1) {
    this->upgrades[type] = 2;
    this->minerals += type.mineralPrice(2);
    this->gas += type.gasPrice(2);
  }
  if (this->upgrades[type] == 2 && count > 2) {
    this->upgrades[type] = 3;
    this->minerals += type.mineralPrice(3);
    this->gas += type.gasPrice(3);
  }
}

bool State::expandStep() {
  for (auto it = this->units.begin(); it != this->units.end(); ++it) {
    // Trainer
    UnitType trainer;
    if (it->first == UnitTypes::Resource_Mineral_Field || it->first == UnitTypes::Resource_Vespene_Geyser) {
      trainer = NULL; // UnitTypes::Protoss_Nexus;
    } else {
      trainer = it->first.whatBuilds().first;
    }
    const int rate = (trainer == UnitTypes::Protoss_Carrier) ? 8 : 4;
    const int trainersRequired = (rate + it->second - 1) / rate;
    if (trainer && trainer != UnitTypes::None && this->units[trainer] < trainersRequired) {
      this->add(trainer, trainersRequired - this->units[trainer]);
      return true;
    }
    // Tech
    auto req = it->first.requiredUnits();
    for (auto it2 = req.begin(); it2 != req.end(); ++it2) {
      if (this->units[it2->first] < 1 /*it2->second*/) {
        this->add(it2->first); /* , it2->second - this->units[it2->first]);*/
        return true;
      }
    }

    // Resources
    const UnitType MINERAL = UnitTypes::Resource_Mineral_Field;
    const int mWorkersRequired = this->minerals / 600;
    if (this->units[MINERAL] < mWorkersRequired) {
      this->add(MINERAL, mWorkersRequired - this->units[MINERAL]);
      return true;
    }
    const UnitType GAS = UnitTypes::Resource_Vespene_Geyser;
    const int gWorkersRequired = this->gas / 900;
    if (this->units[GAS] < gWorkersRequired) {
      this->add(GAS, gWorkersRequired - this->units[GAS]);
      return true;
    }
    const UnitType AS = UnitTypes::Protoss_Assimilator;
    const int asRequired = (this->gas + 1499) / 2700;
    if (this->units[AS] < asRequired) {
      this->add(AS, asRequired - this->units[AS]);
      return true;
    }

    // Supply
    const UnitType PYLON = UnitTypes::Protoss_Pylon;
    const int pylonsRequired = this->supply / PYLON.supplyProvided();
    if (this->units[PYLON] < pylonsRequired) {
      this->add(PYLON, pylonsRequired - this->units[PYLON]);
      return true;
    }
  }
  // Tech
  for (auto &t : this->techs) {
    auto req = t.whatResearches();
    if (this->units[req] < 1) {
      this->add(req);
      return true;
    }
  }
  // Upgrades
  for (auto it = this->upgrades.begin(); it != this->upgrades.end(); ++it) {
    auto req = it->first.whatUpgrades();
    if (this->units[req] < 1) {
      this->add(req);
      return true;
    }
  }
  return false;
}

void State::expand() {
  while (this->expandStep()) {
  }
  // Workers
  int workersRequired = 0;
  for (auto it = this->units.begin(); it != this->units.end(); ++it) {
    if (it->first == UnitTypes::Resource_Mineral_Field || it->first == UnitTypes::Resource_Vespene_Geyser) {
      workersRequired += it->second;
    }
  }
  this->add(UnitTypes::Protoss_Probe, workersRequired - this->units[UnitTypes::Protoss_Probe]);
}

void State::diffTo(const State *goal) {
  goal->eachUnit([this](UnitType type, int count) {
    const int d = count - 2 * this->units[type];
    if (d > 0) {
      this->add(type, d);
    } else if (d < 0) {
      this->remove(type, -d);
    }
    return true;
  });
  goal->eachTech([this](TechType type) {
    if (this->techs.count(type) > 0) {
      this->add(type);
    } else {
      this->remove(type);
    }
    return true;
  });
  goal->eachUpgrade([this](UpgradeType type, int count) {
    const int d = count - 2 * this->upgrades[type];
    if (d > 0) {
      this->add(type, d);
    } else if (d < 0) {
      this->remove(type, -d);
    }
    return true;
  });

  this->minerals = goal->minerals - this->minerals;
  this->gas = goal->gas - this->gas;
}

void State::eachUnit(std::function<bool(BWAPI::UnitType, int)> iterator) const {
  for (auto it = this->units.rbegin(); it != this->units.rend(); ++it) {
    if (it->second > 0) {
      if (!iterator(it->first, it->second)) return;
    }
  }
}

void State::eachTech(std::function<bool(BWAPI::TechType)> iterator) const {
  for (auto &t : this->techs) {
    if (!iterator(t)) return;
  }
}

void State::eachUpgrade(std::function<bool(BWAPI::UpgradeType, int)> iterator) const {
  for (auto it = this->upgrades.rbegin(); it != this->upgrades.rend(); ++it) {
    if (it->second > 0) {
      if (!iterator(it->first, it->second)) return;
    }
  }
}

void State::remove(UnitType type, int count) {
  this->units[type] -= count;
  if (this->units[type] <= 0) {
    this->units.erase(type);
  }
}

void State::remove(TechType type) {
  this->techs.erase(type);
}

void State::remove(UpgradeType type, int count) {
  this->upgrades[type] -= count;
  if (this->upgrades[type] <= 0) {
    this->upgrades.erase(type);
  }
}