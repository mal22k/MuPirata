#include "stdafx.h"
#include "EventEntryLevel.h"
#include "Offset.h"
#include "Protect.h"
#include "Util.h"

void InitEventEntryLevel() // OK
{
	MemoryCpy(0x005FBB44,gProtect.m_MainInfo.m_BloodCastleEntryLevelCommon,sizeof(gProtect.m_MainInfo.m_BloodCastleEntryLevelCommon));

	MemoryCpy(0x005FBB7C,gProtect.m_MainInfo.m_BloodCastleEntryLevelSpecial,sizeof(gProtect.m_MainInfo.m_BloodCastleEntryLevelSpecial));

	MemoryCpy(0x005FBB04,gProtect.m_MainInfo.m_DevilSquareEntryLevelCommon,sizeof(gProtect.m_MainInfo.m_DevilSquareEntryLevelCommon));

	MemoryCpy(0x005FBB24,gProtect.m_MainInfo.m_DevilSquareEntryLevelSpecial,sizeof(gProtect.m_MainInfo.m_DevilSquareEntryLevelSpecial));

	MemoryCpy(0x005FBBB4,gProtect.m_MainInfo.m_ChaosCastleEntryLevelCommon,sizeof(gProtect.m_MainInfo.m_ChaosCastleEntryLevelCommon));

	MemoryCpy(0x005FBBE4,gProtect.m_MainInfo.m_ChaosCastleEntryLevelSpecial,sizeof(gProtect.m_MainInfo.m_ChaosCastleEntryLevelSpecial));

	MemoryCpy(0x005FA7A8,gProtect.m_MainInfo.m_KalimaEntryLevelCommon,sizeof(gProtect.m_MainInfo.m_KalimaEntryLevelCommon));

	MemoryCpy(0x005FA7D8,gProtect.m_MainInfo.m_KalimaEntryLevelSpecial,sizeof(gProtect.m_MainInfo.m_KalimaEntryLevelSpecial));

	SetDword(0x00551AB1,0x270F); // Chaos Castle MaxLevel

	SetDword(0x0041B8DC,0x270F); // Kalima MaxLevel
}