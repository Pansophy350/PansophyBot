#include "CasterManager.h"
#include "UnitUtil.h"
#include <queue>
#include <vector>

using namespace UAlbertaBot;

CasterManager::CasterManager()
{
}

void CasterManager::executeMicro(const BWAPI::Unitset & targets)
{
	checkTargets(targets);
}

//helper function to remove targets once they are targeted by a psi-storm
void removeEffected(BWAPI::Position castLocation, BWAPI::Unitset & targets){
	for (auto effected : targets){
		//approximation of units in effected area
		if (effected->getDistance(castLocation) <= 48){
			targets.erase(effected);
		}
	}
}

void CasterManager::checkTargets(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & casterUnits = getUnits();

	// figure out targets omit buildings since psi-storm does not work on them
	BWAPI::Unitset casterUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(casterUnitTargets, casterUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible() && !u->getType().isBuilding();});

	//a collection of units already effected by a psi-storm
	BWAPI::Unitset covered;

	for (auto & casterUnit : casterUnits)
	{
		BWAPI::Broodwar->drawCircleMap(casterUnit->getPosition(), 2, BWAPI::Colors::Green, true);

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			// if there are targets and we have enough energy to cast
			if (!casterUnitTargets.empty() && canCast(casterUnit))
			{
				// find the best target location for this templar's psi-storm
				std::pair<int, BWAPI::Position> valueAndPosition = getBestTarget(casterUnit, casterUnitTargets);
				int value = valueAndPosition.first;
				BWAPI::Position target = valueAndPosition.second;
				//if this target meets our castThreshold, cast on that target
				if (target && (value > castThreshold) && casterUnit->getDistance(target) < 100){
					//if not being casted on, cast on target
					removeEffected(target, casterUnitTargets);
					castOnLocation(casterUnit, target);
				}
				else if(casterUnit->getDistance(target) > 200){
					//even if we're not going to cast move toward the best target to prepare
					Micro::SmartAttackMove(casterUnit, order.getPosition());
				}

			}
			//if we don't have the energy to cast just retreat
			else if (!canCast(casterUnit)){
				BWAPI::Position fleeTo(BWAPI::Broodwar->self()->getStartLocation());
				Micro::SmartMove(casterUnit, fleeTo);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (casterUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartAttackMove(casterUnit, order.getPosition());
				}
			}
		}
	}
}

bool CasterManager::canCast(BWAPI::Unit casterUnit){
	//psi-storm costs 75 energy so we can cast it if we have this
	return casterUnit->getEnergy() >= BWAPI::TechTypes::Psionic_Storm.energyCost();
}

//return the bestTarget location we can find for casting psi-storm and the value of doing so
std::pair<int, BWAPI::Position> CasterManager::getBestTarget(BWAPI::Unit casterUnit, const BWAPI::Unitset & targets)
{
	int maxvalue = 0;
	BWAPI::Unit besttarget = NULL;
	BWAPI::Unitset targetCoverage;
	for (auto target : targets){
		int value = evaluateCastPosition(target->getPosition(), targets);
		if (value > maxvalue){
			maxvalue = value;
			besttarget = target;
		}
	}
	return std::pair<int, BWAPI::Position>(maxvalue, besttarget->getPosition());
}

//return a measure of value (sum of gasPrice+minearlPrice of units in range) for casting psi-storm at the given location
int CasterManager::evaluateCastPosition(const BWAPI::Position p, const BWAPI::Unitset & targets)
{
	int value = 0;
	for (auto effected : targets){
		//approximation of units in effected area
		if (effected->getDistance(p) <= 48){
			value += effected->getType().gasPrice() + effected->getType().mineralPrice();
		}
	}
	return value;
}

//tell the given unit to cast psi-storm on the given location
void CasterManager::castOnLocation(BWAPI::Unit casterUnit, const BWAPI::Position p)
{
	//ToDo: generalize for non high-templar units
	BWAPI::TechType tech = BWAPI::TechTypes::Psionic_Storm;
	casterUnit->useTech(tech, p);
}
