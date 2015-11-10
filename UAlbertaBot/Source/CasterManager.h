#pragma once;

#include <Common.h>
#include "MicroManager.h"
#include "Unit_Compare.h"

namespace UAlbertaBot
{
	class CasterManager : public MicroManager
	{
	public:

		CasterManager();
		void executeMicro(const BWAPI::Unitset & targets);

		BWAPI::Unit chooseTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets, std::map<BWAPI::Unit, int> & numTargeting);
		BWAPI::Unit closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign);
		std::pair<BWAPI::Unit, BWAPI::Unit> findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets);

		int getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target);
		BWAPI::Unit getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets);

		void checkTargets(const BWAPI::Unitset & targets);
		int evaluateCastPosition(const BWAPI::Position p);
		void castOnLocation(BWAPI::Unit casterUnit, const BWAPI::Position p);
		std::pair<int, BWAPI::Position> getBestTarget(BWAPI::Unit casterUnit, const BWAPI::Unitset & targets); 
		bool canCast(BWAPI::Unit casterUnit);
	private:
		//only cast if we find a target location who's value exceeds this
		int castThreshold = 0;
	};
}