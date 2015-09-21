#include "LevBot.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

LevBot::LevBot() : goal(new State()) {}

void LevBot::onStart() {
  Broodwar->sendText("Welcome to LevBot!");
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;
  Broodwar->setCommandOptimizationLevel(2);

  if (Broodwar->isReplay()) return;
  if (Broodwar->self()->getRace() != Races::Protoss) {
    Broodwar << "Wrong Race!" << std::endl;
    return;
  }
  if (Broodwar->enemy()) {
    Broodwar << "The matchup is " << Broodwar->self()->getName() << " vs " << Broodwar->enemy()->getName() << std::endl;
  }
  // Mixed
  this->goal->add(UnitTypes::Protoss_Zealot, 3);
  this->goal->add(UnitTypes::Protoss_Dragoon, 12);
  this->goal->add(UnitTypes::Protoss_Dark_Templar, 6);
  this->goal->add(UnitTypes::Protoss_Observer, 1);
  this->goal->add(UnitTypes::Protoss_Scarab, 8);
  this->goal->add(UnitTypes::Protoss_Photon_Cannon, 1);
  this->goal->add(UpgradeTypes::Singularity_Charge);
  this->goal->add(UpgradeTypes::Protoss_Ground_Weapons, 3);
  this->goal->add(UpgradeTypes::Protoss_Ground_Armor, 3);
  this->goal->add(UpgradeTypes::Protoss_Plasma_Shields, 2);
  // Zealot Rush
  /*this->goal->add(UnitTypes::Protoss_Zealot, 22);
  this->goal->add(UpgradeTypes::Protoss_Ground_Weapons, 3);
  this->goal->add(UpgradeTypes::Protoss_Ground_Armor, 3);
  this->goal->add(UpgradeTypes::Protoss_Plasma_Shields, 3);*/
  // DT Rush
  /* this->goal->add(UnitTypes::Protoss_Dark_Templar, 16);
  this->goal->add(UpgradeTypes::Protoss_Ground_Weapons, 3);
  this->goal->add(UpgradeTypes::Protoss_Ground_Armor, 3);
  this->goal->add(UpgradeTypes::Protoss_Plasma_Shields, 3); */
  // Carrier Rush
  /*this->goal->add(UnitTypes::Protoss_Interceptor, 32);
  this->goal->add(UpgradeTypes::Protoss_Air_Weapons, 3);
  this->goal->add(UpgradeTypes::Protoss_Air_Armor, 3);
  this->goal->add(UpgradeTypes::Protoss_Plasma_Shields, 3);
  this->goal->add(UpgradeTypes::Carrier_Capacity);*/
  this->goal->expand();
}

void LevBot::onEnd(bool isWinner) {}

static int i = 0;

void LevBot::onFrame() {
  //if (Broodwar->getFrameCount() % 100 != 0) return;
  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
  Broodwar->drawTextScreen(200, 15, "Average FPS: %f", Broodwar->getAverageFPS());

  // Return if the game is a replay or is paused
  if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self()) return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0) return;

  State* current = new State(Broodwar->self());

  // Tactics
  TilePosition tp1 = Broodwar->getStartLocations().at(0);
  TilePosition tp2 = Broodwar->getStartLocations().at(1);
  Position my(Broodwar->self()->getStartLocation());
  Position enemy(tp1 == Broodwar->self()->getStartLocation() ? tp2 : tp1);
  if (Broodwar->getFrameCount() - i > 24) {
    i = Broodwar->getFrameCount();
    for (auto &u : Broodwar->self()->getUnits()) {
      if (u->getType() == UnitTypes::Protoss_Probe && !u->isAttacking()) {
        for (auto &e : u->getUnitsInRadius(600, IsEnemy)) {
          u->attack(e);
        }
      } else if (u->canAttackMove()) {
        // Only support 2 player maps right now
        if (this->attackUnits() > 84) {
          //if (u->getOrder() != Orders::AttackMove && !u->isAttacking() && !u->isStartingAttack() && !u->isUnderAttack()) {
          auto target = u->getClosestUnit(IsEnemy);
          if (target != NULL && u->canAttack(target)) {
            u->attack(target);
            Broodwar->drawTextScreen(300, 40, "Attacking Unit");
          } else {
            u->attack(enemy);
            Broodwar->drawTextScreen(300, 0, "Attacking");
          }
        } else if (this->attackUnits() > 29) {
          //if (u->getOrder() != Orders::AttackMove && !u->isAttacking() && !u->isStartingAttack() && !u->isUnderAttack()) {
          auto target = u->getClosestUnit(IsEnemy);
          if (target != NULL && u->canAttack(target)) {
            u->attack(target);
            Broodwar->drawTextScreen(300, 40, "Attacking Unit");
          } else {
            u->attack((my + enemy) / 2);
            Broodwar->drawTextScreen(300, 0, "Attacking");
          }
        } else {
          double fullDist = (enemy - my).getLength();
          double myDist = (u->getPosition() - my).getLength();
          if (myDist * 2 < fullDist) {
            //if (!u->isPatrolling() && !u->isAttacking() && !u->isStartingAttack() && !u->isMoving()
            //    && !u->isUnderAttack()) {
            auto target = u->getClosestUnit(IsEnemy && CanAttack);
            if (target != NULL && u->canAttack(target)) {
              u->attack(target);
              Broodwar->drawTextScreen(300, 40, "Attacking Inner Unit");
            } else {
              u->patrol(my);
              Broodwar->drawTextScreen(300, 0, "Patrol Inner");
            }
          } else {
            u->move(my);
            Broodwar->drawTextScreen(300, 40, "Retrieving");
          }
        }
      }
      if (u->getType() == UnitTypes::Protoss_Observer) {
        if (u->isUnderAttack()) {
          u->move(my);
        } else {
          auto close = u->getClosestUnit(IsUnderAttack);
          if (close) {
            u->move(close->getPosition());
          } else if (u->isIdle()) {
            /*auto it = Broodwar->getStaticGeysers().begin();
            std::advance(it, rand() % Broodwar->getStaticGeysers().size());*/
            Position p(rand() % (32 * Broodwar->mapWidth()), rand() % (32 * Broodwar->mapHeight()));
            u->move(p);
          }
        }
      }
    }
  }
  Broodwar->drawTextScreen(360, 0, "Attack: %d", this->attackUnits());
  
  // Strategy
  current->diffTo(this->goal);
  int i = 0;
  int* j = &i;
  current->eachUnit([j](UnitType type, int count) {
    Broodwar->drawTextScreen(0, 12 * ((*j)++), "%s: %d", type.getName().c_str(), count);
    return true;
  });
  current->eachUnit([this, current](UnitType type, int count) {
    if (this->dispatch(type, current)) {
      Broodwar << "Dispatched " << type << std::endl;
      return false;
    }
    return true;
  });
  current->eachTech([this, current](TechType type) {
    for (auto &u : Broodwar->self()->getUnits()) {
      if (u->canResearch(type) && u->research(type)) {
        Broodwar << "Researching " << type << std::endl;
        return false;
      }
    }
    return true;
  });
  current->eachUpgrade([this, current](UpgradeType type, int count) {
    for (auto &u : Broodwar->self()->getUnits()) {
      if (u->canUpgrade(type) && u->upgrade(type)) {
        Broodwar << "Upgrading " << type << std::endl;
        return false;
      }
    }
    return true;
  });
  delete current;
}

void LevBot::onSendText(std::string text) {}

void LevBot::onReceiveText(Player player, std::string text) {}

void LevBot::onPlayerLeft(Player player) {}

void LevBot::onNukeDetect(Position target) {}

void LevBot::onUnitDiscover(Unit unit) {}

void LevBot::onUnitEvade(Unit unit) {}

void LevBot::onUnitShow(Unit unit) {}

void LevBot::onUnitHide(Unit unit) {}

void LevBot::onUnitCreate(Unit unit) {}

void LevBot::onUnitDestroy(Unit unit) {}

void LevBot::onUnitMorph(BWAPI::Unit unit) {}

void LevBot::onUnitRenegade(Unit unit) {}

void LevBot::onSaveGame(std::string gameName) {}

void LevBot::onUnitComplete(Unit unit) {}

Unit LevBot::getIdleProbe() {
  for (auto &u : Broodwar->self()->getUnits()) {
    if (u->getType() == UnitTypes::Protoss_Probe && u->isIdle()) {
      return u;
    }
  }
  return NULL;
}

Unit LevBot::getReadyProbe() {
  for (auto &u : Broodwar->self()->getUnits()) {
    if (u->getType() == UnitTypes::Protoss_Probe &&
        u->isIdle() || u->isGatheringMinerals() || u->isGatheringGas()) {
      return u;
    }
  }
  return NULL;
}

Unit LevBot::getIdleTrainer(UnitType type) {
  for (auto &u : Broodwar->self()->getUnits()) {
    if (!u->isTraining() && u->canTrain(type)) {
      return u;
    }
  }
  return NULL;
}

bool LevBot::dispatch(UnitType type, State *current) {
  // Resource gathering
  if (type == UnitTypes::Resource_Vespene_Geyser) {
    auto probe = this->getIdleProbe();
    if (probe) {
      return probe->gather(probe->getClosestUnit(IsRefinery));
    }
  }
  if (type == UnitTypes::Resource_Mineral_Field) {
    auto probe = this->getIdleProbe();
    if (probe) {
      return probe->gather(probe->getClosestUnit(IsMineralField));
    }
  }
  // Training / Building
  int mineralCost = type.mineralPrice();
  int gasCost = type.gasPrice();
  if (current->savedMinerals < mineralCost || current->savedGas < gasCost) return false;
  if (type.isBuilding()) {
    auto probe = this->getReadyProbe();
    if (probe && probe->canBuild(type)) {
      TilePosition targetBuildLocation = Broodwar->getBuildLocation(type, probe->getTilePosition());
      if (targetBuildLocation) {
        return probe->build(type, targetBuildLocation);
      }
    }
  }
  auto trainer = this->getIdleTrainer(type);
  if (trainer) {
    return trainer->train(type);
  }
  return false;
}

int LevBot::attackUnits() {
  int res = 0;
  for (auto &u : Broodwar->self()->getUnits()) {
    if (u->canAttack() && u->canMove() && u->exists() &&
     u->getType() != UnitTypes::Protoss_Probe) {
      res += u->getType().supplyRequired();
    }
  }
  return res;
}