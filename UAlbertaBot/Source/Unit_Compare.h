#include "UnitUtil.h"

class Unit_Compare {
public:
	bool operator()(BWAPI::Unit& t1, BWAPI::Unit& t2)
	{
		//if (t1->getPosition.getPosition() < t2->getPosition()) return true;
		return false;
	}
};