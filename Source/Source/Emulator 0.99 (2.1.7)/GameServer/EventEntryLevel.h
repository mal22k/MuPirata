// EventEntryLevel.h: interface for the CEventEntryLevel class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "User.h"

class CEventEntryLevel
{
public:
	void Load(char* path);
	int GetBCLevel(LPOBJ lpObj);
	int GetDSLevel(LPOBJ lpObj);
	int GetCCLevel(LPOBJ lpObj);
	int GetKALevel(LPOBJ lpObj);
private:
	int m_BloodCastleEntryLevelCommon[7][2];
	int m_BloodCastleEntryLevelSpecial[7][2];
	int m_DevilSquareEntryLevelCommon[4][2];
	int m_DevilSquareEntryLevelSpecial[4][2];
	int m_ChaosCastleEntryLevelCommon[6][2];
	int m_ChaosCastleEntryLevelSpecial[6][2];
	int m_KalimaEntryLevelCommon[6][2];
	int m_KalimaEntryLevelSpecial[6][2];
};

extern CEventEntryLevel gEventEntryLevel;