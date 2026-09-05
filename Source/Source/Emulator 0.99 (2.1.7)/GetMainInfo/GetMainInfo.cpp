#include "stdafx.h"
#include "CCRC32.H"
#include "CustomEffect.h"
#include "CustomFog.h"
#include "CustomItem.h"
#include "CustomMap.h"
#include "CustomMonster.h"
#include "CustomTooltip.h"
#include "EventEntryLevel.h"
#include "ItemStack.h"
#include "ItemValue.h"
#include "ThemidaSDK.h"

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

int _tmain(int argc, _TCHAR* argv[]) // OK
{
	CLEAR_START

	ENCODE_START

	MAIN_FILE_INFO info;

	memset(&info,0,sizeof(info));

	GetPrivateProfileString("MainInfo","CustomerName","",info.CustomerName,sizeof(info.CustomerName),".\\MainInfo.ini");
	info.LauncherStart = GetPrivateProfileInt("MainInfo","LauncherStart",0,".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","LauncherMutex","",info.LauncherMutex,sizeof(info.LauncherMutex),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","IpAddress","",info.IpAddress,sizeof(info.IpAddress),".\\MainInfo.ini");
	info.IpAddressPort = GetPrivateProfileInt("MainInfo","IpAddressPort",44405,".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","ClientVersion","",info.ClientVersion,sizeof(info.ClientVersion),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","ClientSerial","",info.ClientSerial,sizeof(info.ClientSerial),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","WindowName","",info.WindowName,sizeof(info.WindowName),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","ScreenShotPath","",info.ScreenShotPath,sizeof(info.ScreenShotPath),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","ClientName","",info.ClientName,sizeof(info.ClientName),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","PluginName1","",info.PluginName[0],sizeof(info.PluginName[0]),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","PluginName2","",info.PluginName[1],sizeof(info.PluginName[1]),".\\MainInfo.ini");
	GetPrivateProfileString("MainInfo","PluginName3","",info.PluginName[2],sizeof(info.PluginName[2]),".\\MainInfo.ini");
	info.MaxAttackSpeed[0] = GetPrivateProfileInt("CharacterInfo","DWMaxAttackSpeed",65535,".\\MainInfo.ini");
	info.MaxAttackSpeed[1] = GetPrivateProfileInt("CharacterInfo","DKMaxAttackSpeed",65535,".\\MainInfo.ini");
	info.MaxAttackSpeed[2] = GetPrivateProfileInt("CharacterInfo","FEMaxAttackSpeed",65535,".\\MainInfo.ini");
	info.MaxAttackSpeed[3] = GetPrivateProfileInt("CharacterInfo","MGMaxAttackSpeed",65535,".\\MainInfo.ini");
	info.MaxAttackSpeed[4] = GetPrivateProfileInt("CharacterInfo","DLMaxAttackSpeed",65535,".\\MainInfo.ini");
	info.AttackAnimationValue[0] = GetPrivateProfileInt("AnimationInfo","DWAttackAnimationValue",15,".\\MainInfo.ini");
	info.AttackAnimationValue[1] = GetPrivateProfileInt("AnimationInfo","DKAttackAnimationValue",15,".\\MainInfo.ini");
	info.AttackAnimationValue[2] = GetPrivateProfileInt("AnimationInfo","FEAttackAnimationValue",15,".\\MainInfo.ini");
	info.AttackAnimationValue[3] = GetPrivateProfileInt("AnimationInfo","MGAttackAnimationValue",15,".\\MainInfo.ini");
	info.AttackAnimationValue[4] = GetPrivateProfileInt("AnimationInfo","DLAttackAnimationValue",15,".\\MainInfo.ini");
	info.KeyCodeHealthBarSwitch = GetPrivateProfileInt("KeyInfo","KeyCodeHealthBarSwitch",120,".\\MainInfo.ini");
	info.KeyCodeCamera3DSwitch = GetPrivateProfileInt("KeyInfo","KeyCodeCamera3DSwitch",121,".\\MainInfo.ini");
	info.KeyCodeCamera3DRestore = GetPrivateProfileInt("KeyInfo","KeyCodeCamera3DRestore",122,".\\MainInfo.ini");
	info.KeyCodeTrayModeSwitch = GetPrivateProfileInt("KeyInfo","KeyCodeTrayModeSwitch",123,".\\MainInfo.ini");
	info.ReconnectTime = GetPrivateProfileInt("ReconnectInfo","ReconnectTime",300000,".\\MainInfo.ini");

	if(gCustomEffect.Load(".\\Data\\CustomEffect.txt") == 0) { system("pause"); return 0; }
	if(gCustomFog.Load(".\\Data\\CustomFog.txt") == 0) { system("pause"); return 0; }
	if(gCustomItem.Load(".\\Data\\CustomItem.txt") == 0) { system("pause"); return 0; }
	if(gCustomMap.Load(".\\Data\\CustomMap.txt") == 0) { system("pause"); return 0; }
	if(gCustomMonster.Load(".\\Data\\CustomMonster.txt") == 0) { system("pause"); return 0; }
	if(gCustomTooltip.Load(".\\Data\\CustomTooltip.txt") == 0) { system("pause"); return 0; }
	if(gEventEntryLevel.Load(".\\Data\\EventEntryLevel.txt") == 0) { system("pause"); return 0; }
	if(gItemStack.Load(".\\Data\\ItemStack.txt") == 0) { system("pause"); return 0; }
	if(gItemValue.Load(".\\Data\\ItemValue.txt") == 0) { system("pause"); return 0; }

	memcpy(info.m_BloodCastleEntryLevelCommon,gEventEntryLevel.m_BloodCastleEntryLevelCommon,sizeof(info.m_BloodCastleEntryLevelCommon));
	memcpy(info.m_BloodCastleEntryLevelSpecial,gEventEntryLevel.m_BloodCastleEntryLevelSpecial,sizeof(info.m_BloodCastleEntryLevelSpecial));
	memcpy(info.m_DevilSquareEntryLevelCommon,gEventEntryLevel.m_DevilSquareEntryLevelCommon,sizeof(info.m_DevilSquareEntryLevelCommon));
	memcpy(info.m_DevilSquareEntryLevelSpecial,gEventEntryLevel.m_DevilSquareEntryLevelSpecial,sizeof(info.m_DevilSquareEntryLevelSpecial));
	memcpy(info.m_ChaosCastleEntryLevelCommon,gEventEntryLevel.m_ChaosCastleEntryLevelCommon,sizeof(info.m_ChaosCastleEntryLevelCommon));
	memcpy(info.m_ChaosCastleEntryLevelSpecial,gEventEntryLevel.m_ChaosCastleEntryLevelSpecial,sizeof(info.m_ChaosCastleEntryLevelSpecial));
	memcpy(info.m_KalimaEntryLevelCommon,gEventEntryLevel.m_KalimaEntryLevelCommon,sizeof(info.m_KalimaEntryLevelCommon));
	memcpy(info.m_KalimaEntryLevelSpecial,gEventEntryLevel.m_KalimaEntryLevelSpecial,sizeof(info.m_KalimaEntryLevelSpecial));

	memcpy(info.CustomEffectInfo,gCustomEffect.m_CustomEffectInfo,sizeof(info.CustomEffectInfo));
	memcpy(info.CustomFogInfo,gCustomFog.m_CustomFogInfo,sizeof(info.CustomFogInfo));
	memcpy(info.CustomItemInfo,gCustomItem.m_CustomItemInfo,sizeof(info.CustomItemInfo));
	memcpy(info.CustomMapInfo,gCustomMap.m_CustomMapInfo,sizeof(info.CustomMapInfo));
	memcpy(info.CustomMonsterInfo,gCustomMonster.m_CustomMonsterInfo,sizeof(info.CustomMonsterInfo));
	memcpy(info.CustomMonsterSkinInfo, gCustomMonster.m_CustomMonsterSkinInfo,sizeof(info.CustomMonsterSkinInfo));
	memcpy(info.CustomTooltipInfo,gCustomTooltip.m_CustomTooltipInfo,sizeof(info.CustomTooltipInfo));
	memcpy(info.ItemStackInfo,gItemStack.m_ItemStackInfo,sizeof(info.ItemStackInfo));
	memcpy(info.ItemValueInfo,gItemValue.m_ItemValueInfo,sizeof(info.ItemValueInfo));

	CCRC32 CRC32;

	char buff[256];

	wsprintf(buff,".\\Path\\%s",info.ClientName);

	if(CRC32.FileCRC(buff,&info.ClientNameCRC,1024) == 0)
	{
		info.ClientNameCRC = 0;
	}

	for(int i=0;i < 3;i++)
	{
		wsprintf(buff,".\\Path\\%s",info.PluginName[i]);

		if(CRC32.FileCRC(buff,&info.PluginNameCRC[i],1024) == 0)
		{
			info.PluginNameCRC[i] = 0;
		}
	}

	for (int n=0;n < sizeof(MAIN_FILE_INFO);n++)
	{
		((BYTE*)&info)[n] ^= (BYTE)(0x10^LOBYTE(n));
		((BYTE*)&info)[n] -= (BYTE)(0x20^HIBYTE(n));
		((BYTE*)&info)[n] += (BYTE)(0x30^HIBYTE(n));
	}

	HANDLE file = CreateFile(".\\Path\\ServerInfo.sse",GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,0);

	if(file == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	DWORD OutSize = 0;

	if(WriteFile(file,&info,sizeof(MAIN_FILE_INFO),&OutSize,0) == 0)
	{
		CloseHandle(file);
		return 0;
	}

	CloseHandle(file);

	ENCODE_END

	CLEAR_END

	return 0;
}