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
	const BWAPI::Unitset & ourUnits = BWAPI::Broodwar->self()->getUnits();

	BWAPI::Bulletset bullets = BWAPI::Broodwar->getBullets();
	BWAPI::Bulletset psistorms;
	std::copy_if(bullets.begin(), bullets.end(), std::inserter(psistorms, psistorms.end()), [](BWAPI::Bullet b){return b->getType() == BWAPI::BulletTypes::Psionic_Storm; });

	//keep a list of friendly units to try to avoid targeting them
	BWAPI::Unitset friendlyUnits;
	std::copy_if(ourUnits.begin(), ourUnits.end(), std::inserter(friendlyUnits, friendlyUnits.end()), [](BWAPI::Unit u){return u->isVisible() && !u->getType().isBuilding();});

	// figure out targets omit buildings since psi-storm does not work on them
	BWAPI::Unitset casterUnitTargets;
	BWAPI::Unitset casterUnitThreats;
	std::copy_if(targets.begin(), targets.end(), std::inserter(casterUnitTargets, casterUnitTargets.end()), [](BWAPI::Unit u){return u->isVisible() && !u->getType().isBuilding();});
	std::copy_if(targets.begin(), targets.end(), std::inserter(casterUnitThreats, casterUnitThreats.end()), [](BWAPI::Unit u){return u->isVisible() && u->getType().isBuilding() && u->canAttack(); });
	//removed targets effected by psi-storm
	for (auto & psistorm : psistorms){
		removeEffected(psistorm->getPosition(), casterUnitTargets);
	}

	for (auto & casterUnit : casterUnits)
	{
		//become selfless in the face of death
		if(casterUnit->getHitPoints() <= (casterUnit->getType().maxHitPoints() / 1.5)) friendlyUnits.erase(casterUnit);
		
		BWAPI::Broodwar->drawCircleMap(casterUnit->getPosition(), 2, BWAPI::Colors::Green, true);

		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			// if there are targets and we have enough energy to cast
			if (!casterUnitTargets.empty() && canCast(casterUnit))
			{
				// find the best target location for this templar's psi-storm
				std::pair<int, BWAPI::Position> valueAndPosition = getBestTarget(casterUnit, casterUnitTargets, friendlyUnits);
				int value = valueAndPosition.first;
				BWAPI::Position target = valueAndPosition.second;

				//if this target meets our castThreshold, or we are significantly damaged cast on that target
				if (target && casterUnit->getDistance(target) < 300 && (value > castThreshold || (value >= 0 && casterUnit->getHitPoints() <= (casterUnit->getInitialHitPoints() / 1.5)))){
					//if not being casted on, cast on target
					removeEffected(target, casterUnitTargets);
					castOnLocation(casterUnit, target);
				}
				//if we are too far away move towards best target even if we don't plan to cast
				else if (casterUnit->getDistance(target) >= 300) {
					Micro::SmartMove(casterUnit, order.getPosition());
				}
				//back off if we get too close and aren't going to cast right away
				else if ((casterUnit->getDistance(target) < 300)){
					BWAPI::Position fleeTo(BWAPI::Broodwar->self()->getStartLocation());
					Micro::SmartMove(casterUnit, fleeTo);
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
				//if there are no targets but we are near a threat flee
				for (auto threat : casterUnitThreats){
					if (casterUnit->getDistance(threat) < 500){
						BWAPI::Position fleeTo(BWAPI::Broodwar->self()->getStartLocation());
						return;
					}
				}
				// if we're not near the order position
				if (casterUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartMove(casterUnit, order.getPosition());
				}
			}
		}
	}
	//merge
	//for (auto & casterUnit : casterUnits)
}



//We may want our casters to take a retreating shot even while fleeing so we create an alternative regroup function
void CasterManager::regroup(const BWAPI::Position & regroupPosition) const
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	int regroupDistanceFromBase = MapTools::Instance().getGroundDistance(regroupPosition, ourBasePosition);

	const BWAPI::Unitset & casterUnits = getUnits();
	const BWAPI::Unitset & ourUnits = BWAPI::Broodwar->self()->getUnits();

	//keep a list of friendly units to try to avoid targeting them
	BWAPI::Unitset friendlyUnits;
	std::copy_if(ourUnits.begin(), ourUnits.end(), std::inserter(friendlyUnits, friendlyUnits.end()), [](BWAPI::Unit u){return u->isVisible() && !u->getType().isBuilding(); });

	// figure out targets omit buildings since psi-storm does not work on them
	BWAPI::Unitset targets;
	BWAPI::Unitset casterUnitTargets;
	MapGrid::Instance().GetUnits(targets, calcCenter(), 800, false, true);
	std::copy_if(targets.begin(), targets.end(), std::inserter(casterUnitTargets, casterUnitTargets.end()), [](BWAPI::Unit u){return u->isVisible() && !u->getType().isBuilding(); });

	// for each of the units we have
	for (auto & casterUnit : casterUnits)
	{
		// if there are targets and we have enough energy to cast
		if (!casterUnitTargets.empty() && canCast(casterUnit))
		{
			// find the best target location for this templar's psi-storm
			std::pair<int, BWAPI::Position> valueAndPosition = getBestTarget(casterUnit, casterUnitTargets, friendlyUnits);
			int value = valueAndPosition.first;
			BWAPI::Position target = valueAndPosition.second;

			//if this target meets our castThreshold, or we are significantly damaged cast on that target
			if (target && casterUnit->getDistance(target) < 300 && (value > castThreshold || (value >= 0 && casterUnit->getHitPoints() <= (casterUnit->getType().maxHitPoints()) / 1.5))){
				//if not being casted on, cast on target
				removeEffected(target, casterUnitTargets);
				castOnLocation(casterUnit, target);
				return;
			}
		}
		int unitDistanceFromBase = MapTools::Instance().getGroundDistance(casterUnit->getPosition(), ourBasePosition);

		// if the unit is outside the regroup area
		if (unitDistanceFromBase > regroupDistanceFromBase)
		{
			Micro::SmartMove(casterUnit, ourBasePosition);
		}
		else if (casterUnit->getDistance(regroupPosition) > 100)
		{
			// regroup it
			Micro::SmartMove(casterUnit, regroupPosition);
		}
		else
		{
			Micro::SmartAttackMove(casterUnit, casterUnit->getPosition());
		}
	}
}

bool CasterManager::canCast(BWAPI::Unit casterUnit) const{
	//psi-storm costs 75 energy so we can cast it if we have this
	return casterUnit->getEnergy() >= BWAPI::TechTypes::Psionic_Storm.energyCost();
}

//return the bestTarget location we can find for casting psi-storm and the value of doing so
std::pair<int, BWAPI::Position> CasterManager::getBestTarget(BWAPI::Unit casterUnit, const BWAPI::Unitset & targets, const BWAPI::Unitset & friendly) const
{
	//if we can't do better than 0 value just return arbitrary position and 0 value
	float maxvalue = 0;
	BWAPI::Unit besttarget = NULL;
	for (auto target : targets){
		float value = evaluateCastPosition(target->getPosition(), targets, friendly);
		if (value > maxvalue){
			maxvalue = value;
			besttarget = target;
		}
	}
	if (besttarget == NULL) return std::pair<int, BWAPI::Position>(0,BWAPI::Position(0,0));
	return std::pair<int, BWAPI::Position>((int) maxvalue, besttarget->getPosition());
}

float unitValue(const BWAPI::Unit u){
	float value;
	if (u->getType() == BWAPI::UnitTypes::Protoss_Archon) value = (float) 2 * (BWAPI::UnitTypes::Protoss_High_Templar.gasPrice() + BWAPI::UnitTypes::Protoss_High_Templar.mineralPrice());
	else if (u->getType() == BWAPI::UnitTypes::Protoss_Dark_Archon) value = (float) 2 * (BWAPI::UnitTypes::Protoss_Dark_Templar.gasPrice() + BWAPI::UnitTypes::Protoss_Dark_Templar.mineralPrice());
	else value = (float) (u->getType().gasPrice() + u->getType().mineralPrice());
	if (u->isCloaked() || u->isBurrowed()) value *= 1.5;
	//value should now be value of unit hit points
	//value = value * 100 / (float)u->getType().maxHitPoints();
	return value;
}

//return a measure of value (sum of gasPrice+minearlPrice of units in range) for casting psi-storm at the given location
float CasterManager::evaluateCastPosition(const BWAPI::Position p, const BWAPI::Unitset & targets, const BWAPI::Unitset & friendly) const
{
	float value = 0;
	for (auto &effected : targets){
		//slightly generous approximation of units in effected area
		if (effected->getDistance(p) <= 55) value += unitValue(effected);
	}
	for (auto &effected : friendly){
		if (effected->getDistance(p) <= 55) value -= unitValue(effected);
	}
	return value;
}

//tell the given unit to cast psi-storm on the given location
void CasterManager::castOnLocation(BWAPI::Unit casterUnit, const BWAPI::Position p) const
{
	//ToDo: generalize for non high-templar units
	BWAPI::TechType tech = BWAPI::TechTypes::Psionic_Storm;
	casterUnit->useTech(tech, p);
}
