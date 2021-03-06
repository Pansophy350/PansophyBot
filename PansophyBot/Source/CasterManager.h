#pragma once;

#include <Common.h>
#include "MicroManager.h"

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
		void checkTargets(const BWAPI::Unitset & targets);
		void regroup(const BWAPI::Position & regroupPosition) const override;
		float evaluateCastPosition(const BWAPI::Position p, const BWAPI::Unitset & targets, const BWAPI::Unitset & friendly) const;
		void castOnLocation(BWAPI::Unit casterUnit, const BWAPI::Position p) const;
		std::pair<int, BWAPI::Position> getBestTarget(BWAPI::Unit casterUnit, const BWAPI::Unitset & targets, const BWAPI::Unitset & friendly) const;
		bool canCast(BWAPI::Unit casterUnit) const;
	private:
		//only cast if we find a target location who's value exceeds this
		float castThreshold = 200;
	};
}