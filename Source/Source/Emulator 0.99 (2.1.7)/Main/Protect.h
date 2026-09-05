// Protect.h: interface for the CProtect class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CustomEffect.h"
#include "CustomFog.h"
#include "CustomItem.h"
#include "CustomMap.h"
#include "CustomMonster.h"
#include "CustomMonsterSkin.h"
#include "CustomTooltip.h"
#include "ItemStack.h"
#include "ItemValue.h"

struct MAIN_FILE_INFO
{
	char CustomerName[32];
	BYTE LauncherStart;
	char LauncherMutex[32];
	char IpAddress[32];
	WORD IpAddressPort;
	char ClientVersion[8];
	char ClientSerial[17];
	char WindowName[32];
	char ScreenShotPath[50];
	char ClientName[32];
	DWORD ClientNameCRC;
	char PluginName[3][32];
	DWORD PluginNameCRC[4];
	DWORD MaxAttackSpeed[5];
	BYTE AttackAnimationValue[5];
	BYTE KeyCodeHealthBarSwitch;
	BYTE KeyCodeCamera3DSwitch;
	BYTE KeyCodeCamera3DRestore;
	BYTE KeyCodeTrayModeSwitch;
	DWORD ReconnectTime;
	int m_BloodCastleEntryLevelCommon[7][2];
	int m_BloodCastleEntryLevelSpecial[7][2];
	int m_DevilSquareEntryLevelCommon[4][2];
	int m_DevilSquareEntryLevelSpecial[4][2];
	int m_ChaosCastleEntryLevelCommon[6][2];
	int m_ChaosCastleEntryLevelSpecial[6][2];
	int m_KalimaEntryLevelCommon[6][2];
	int m_KalimaEntryLevelSpecial[6][2];
	CUSTOM_EFFECT_INFO CustomEffectInfo[MAX_CUSTOM_EFFECT];
	CUSTOM_FOG_INFO CustomFogInfo[MAX_CUSTOM_FOG];
	CUSTOM_ITEM_INFO CustomItemInfo[MAX_CUSTOM_ITEM];
	CUSTOM_MAP_INFO CustomMapInfo[MAX_CUSTOM_MAP];
	CUSTOM_MONSTER_INFO CustomMonsterInfo[MAX_CUSTOM_MONSTER];
	CUSTOM_MONSTER_SKIN_INFO CustomMonsterSkinInfo[MAX_CUSTOM_MONSTER_SKIN];
	CUSTOM_TOOLTIP_INFO CustomTooltipInfo[MAX_CUSTOM_TOOLTIP];
	ITEM_STACK_INFO ItemStackInfo[MAX_ITEM_STACK_INFO];
	ITEM_VALUE_INFO ItemValueInfo[MAX_ITEM_VALUE_INFO];
};

class CProtect
{
public:
	CProtect();
	virtual ~CProtect();
	bool ReadMainFile(char* name);
	void CheckLauncher();
	void CheckInstance();
	void CheckClientFile();
	void CheckPluginFile();
public:
	MAIN_FILE_INFO m_MainInfo;
	DWORD m_ClientFileCRC;
};

extern CProtect gProtect;