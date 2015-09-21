#include "LevBot.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

void LevBot::onStart() {
  Broodwar->sendText("Welcome to LevBot!");
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;
  Broodwar->setCommandOptimizationLevel(2);

  if (Broodwar->isReplay()) return;
  if (Broodwar->enemy()) {
    Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
  }
}

void LevBot::onEnd(bool isWinner) {}

void LevBot::onFrame() {
  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
  Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS());

  // Return if the game is a replay or is paused
  if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self()) return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0) return;

  // Iterate through all the units that we own
  for (auto &u : Broodwar->self()->getUnits()) {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if (!u->exists())
      continue;

    // Ignore the unit if it has one of the following status ailments
    if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
      continue;

    // Ignore the unit if it is in one of the following states
    if (u->isLoaded() || !u->isPowered() || u->isStuck())
      continue;

    // Ignore the unit if it is incomplete or busy constructing
    if (!u->isCompleted() || u->isConstructing())
      continue;


    if (u->getType() == UnitTypes::Protoss_Probe) {
      this->manageProbe(u);
    } else if (u->getType() == UnitTypes::Protoss_Nexus) {
      this->manageNexus(u);
      this->manageSupply(u);
    }
  }
}

void LevBot::onSendText(std::string text) {}

void LevBot::onReceiveText(BWAPI::Player player, std::string text) {}

void LevBot::onPlayerLeft(BWAPI::Player player) {}

void LevBot::onNukeDetect(BWAPI::Position target) {}

void LevBot::onUnitDiscover(BWAPI::Unit unit) {}

void LevBot::onUnitEvade(BWAPI::Unit unit) {}

void LevBot::onUnitShow(BWAPI::Unit unit) {}

void LevBot::onUnitHide(BWAPI::Unit unit) {}

void LevBot::onUnitCreate(BWAPI::Unit unit) {}

void LevBot::onUnitDestroy(BWAPI::Unit unit) {}

void LevBot::onUnitMorph(BWAPI::Unit unit) {}

void LevBot::onUnitRenegade(BWAPI::Unit unit) {}

void LevBot::onSaveGame(std::string gameName) {}

void LevBot::onUnitComplete(BWAPI::Unit unit) {}

void LevBot::manageProbe(BWAPI::Unit probe) {
  if (probe->isIdle() && !probe->getPowerUp()) {
    if (probe->isCarryingGas() || probe->isCarryingMinerals()) {
      probe->returnCargo();
    } else {
      probe->gather(probe->getClosestUnit(IsMineralField || IsRefinery));
    }
  }
}

void LevBot::manageNexus(BWAPI::Unit nexus) {
  int workers = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Probe);
  int cost = UnitTypes::Protoss_Probe.mineralPrice();
  if (nexus->isIdle() && workers < 16 && cost <= Broodwar->self()->minerals()) {
    nexus->train(UnitTypes::Protoss_Probe);
  }
}

void LevBot::manageSupply(BWAPI::Unit nexus) {
  int left = Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed();
  int cost = UnitTypes::Protoss_Pylon.mineralPrice();
  if (left < 8 && cost <= Broodwar->self()->minerals()) {
    Unit probe = nexus->getClosestUnit(GetType == UnitTypes::Protoss_Probe &&
                                       (IsIdle || IsGatheringMinerals) &&
                                       IsOwned);
    if (probe) {
      TilePosition targetBuildLocation = Broodwar->getBuildLocation(UnitTypes::Protoss_Pylon, probe->getTilePosition());
      if (targetBuildLocation) {
        probe->build(UnitTypes::Protoss_Pylon, targetBuildLocation);
      }
    }
  }
}