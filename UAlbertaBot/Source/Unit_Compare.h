#include "UnitUtil.h"

class Unit_Compare {

public:

	struct Min
	{
		bool operator()(BWAPI::Unit& t1, BWAPI::Unit& t2)
		{
			BWAPI::Position t1p = t1->getPosition();
			BWAPI::Position t2p = t2->getPosition();
			//based by top left corner of map
			if (t1p.getLength() < t1p.getLength()) return true;
			return false;
		}
	};

	struct Max
	{
		bool operator()(BWAPI::Unit& t1, BWAPI::Unit& t2)
		{
			BWAPI::Position t1p = t1->getPosition();
			BWAPI::Position t2p = t2->getPosition();
			//based by top left corner of map
			if (t1p.getLength() > t1p.getLength()) return true;
			return false;
		}
	};

};