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


void CasterManager::checkTargets(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & casterUnits = getUnits();

	// figure out targets
	BWAPI::Unitset casterUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(casterUnitTargets, casterUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

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
				if (target && (value > castThreshold)){
					castOnLocation(casterUnit, target);
				}

			}
			// if there are no targets
			else if (casterUnitTargets.empty())
			{
				// if we're not near the order position
				if (casterUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartAttackMove(casterUnit, order.getPosition());
				}
			}
			else if (!canCast(casterUnit)){
				//ToDo: tell unit to stay out of enemy range (or prehaps merge into archon) if it doesn't have the energy to cast
			}
		}
	}
}




// get the attack priority of a type in relation to a zergling
int CasterManager::getAttackPriority(BWAPI::Unit casterUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = casterUnit->getType();
	BWAPI::UnitType targetType = target->getType();


	if (casterUnit->getType() == BWAPI::UnitTypes::Zerg_Scourge)
	{
		if (target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
		{

			return 100;
		}

		if (target->getType() == BWAPI::UnitTypes::Protoss_Corsair)
		{
			return 90;
		}
	}

	bool isThreat = rangedType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;

	if (target->getType().isWorker())
	{
		isThreat = false;
	}

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}

	if (casterUnit->isFlying() && target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		return 101;
	}

	// if the target is building something near our base something is fishy
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	if (target->getType().isWorker() && (target->isConstructing() || target->isRepairing()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 100;
	}

	if (target->getType().isBuilding() && (target->isCompleted() || target->isBeingConstructed()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 90;
	}

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		if (casterUnit->getType() == BWAPI::UnitTypes::Terran_Vulture)
		{
			return 11;
		}

		return 11;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 4;
	}
	else if (targetType.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}

bool CasterManager::canCast(BWAPI::Unit casterUnit){
	//psi-storm costs 75 energy so we can cast it if we have this
	return casterUnit->getEnergy() >= BWAPI::TechTypes::Psionic_Storm.energyCost();
}

//return the bestTarget location we can find for casting psi-storm and the value of doing so
/*
getBestTarget
|-> evaluateCastPosition
|-> castOnLocation
*/
std::pair<int, BWAPI::Position> CasterManager::getBestTarget(BWAPI::Unit casterUnit, const BWAPI::Unitset & targets)
{
	
	//max...min
	std::priority_queue < BWAPI::Unit, std::vector<BWAPI::Unit>, Unit_Compare::Min > min;
	//min...max
	std::priority_queue < BWAPI::Unit, std::vector<BWAPI::Unit>, Unit_Compare::Max > max;
	min.push( *(targets.begin()) );

	//data use to know where to place unit 
	int minSize = 1;
	int maxSize = 0;

	//find center unit of a group relative to the caster
	for (auto target : targets)
	{
		//if found new min
		if ( casterUnit->getDistance(target) < min.top()->getDistance(target) )
		{
			if (minSize > maxSize)
			{
				maxSize++;
				max.push( min.top() );
				min.pop();
				min.push(target);
			}
			else
			{
				minSize++;
				min.push(target);
			}
		}
		else
		{
			if (minSize < maxSize)
			{
				minSize++;
				min.push(max.top());
				max.pop();
				max.push(target);
			}
			else
			{
				maxSize++;
				max.push(target);
			}
		}
	}
	return std::pair<int, BWAPI::Position>(1, min.top()->getPosition());
}

//return a measure of value (sum of attack priority?) for casting psi-storm at the given location
int CasterManager::evaluateCastPosition(const BWAPI::Position p)
{
	//ToDo: implement
	return 0;
}

//tell the given unit to cast psi-storm on the given location
void CasterManager::castOnLocation(BWAPI::Unit casterUnit, const BWAPI::Position p)
{
	//ToDo: generalize for non high-templar units
	BWAPI::TechType tech = BWAPI::TechTypes::Psionic_Storm;
	casterUnit->useTech(tech, p);
}
