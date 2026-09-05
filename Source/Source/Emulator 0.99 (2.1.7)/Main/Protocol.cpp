#include "stdafx.h"
#include "Protocol.h"
#include "CCRC32.H"
#include "ChaosBox.h"
#include "Common.h"
#include "EventEntryLevel.h"
#include "HackCheck.h"
#include "HealthBar.h"
#include "Language.h"
#include "Offset.h"
#include "PacketManager.h"
#include "PrintPlayer.h"
#include "Protect.h"
#include "Reconnect.h"
#include "ServerList.h"
#include "Util.h"

BOOL ProtocolCoreEx(BYTE head,BYTE* lpMsg,int size,int key) // OK
{
	switch(head)
	{
		case 0xD9:
			GCDamageRecv((PMSG_DAMAGE_RECV*)lpMsg);
			break;
		case 0xDE:
			GCCharacterCreationEnableRecv((PMSG_CHARACTER_CREATION_ENABLE_RECV*)lpMsg);
			break;
		case 0x17:
			GCUserDieRecv((PMSG_USER_DIE_RECV*)lpMsg);
			break;
		case 0x22:
			GCItemGetRecv((PMSG_ITEM_GET_RECV*)lpMsg);
			break;
		case 0x26:
			GCLifeRecv((PMSG_LIFE_RECV*)lpMsg);
			break;
		case 0x27:
			GCManaRecv((PMSG_MANA_RECV*)lpMsg);
			break;
		case 0x88:
			GCChaosMixRateRecv((PMSG_CHAOS_MIX_RATE_RECV*)lpMsg);
			break;
		case 0x9C:
			GCRewardExperienceRecv((PMSG_REWARD_EXPERIENCE_RECV*)lpMsg);
			break;
		case 0xA3:
			GCQuestRewardRecv((PMSG_QUEST_REWARD_RECV*)lpMsg);
			break;
		case 0xF1:
			switch(((lpMsg[0]==0xC1)?lpMsg[3]:lpMsg[4]))
			{
				case 0x00:
					GCConnectClientRecv((PMSG_CONNECT_CLIENT_RECV*)lpMsg);
					break;
				case 0x01:
					GCConnectAccountRecv((PMSG_CONNECT_ACCOUNT_RECV*)lpMsg);
					break;
				case 0x02:
					GCCloseClientRecv((PMSG_CLOSE_CLIENT_RECV*)lpMsg);
					break;
			}
			break;
		case 0xF3:
			switch(((lpMsg[0]==0xC1)?lpMsg[3]:lpMsg[4]))
			{
				case 0x00:
					GCCharacterListRecv((PMSG_CHARACTER_LIST_RECV*)lpMsg);
					break;
				case 0x03:
					GCCharacterInfoRecv((PMSG_CHARACTER_INFO_RECV*)lpMsg);
					break;
				case 0x04:
					GCCharacterRegenRecv((PMSG_CHARACTER_REGEN_RECV*)lpMsg);
					break;
				case 0x05:
					GCLevelUpRecv((PMSG_LEVEL_UP_RECV*)lpMsg);
					break;
				case 0x06:
					GCLevelUpPointRecv((PMSG_LEVEL_UP_POINT_RECV*)lpMsg);
					break;
				case 0x07:
					GCMonsterDamageRecv((PMSG_MONSTER_DAMAGE_RECV*)lpMsg);
					break;
				case 0x22:
					GCTimeViewRecv((PMSG_TIME_VIEW_RECV*)lpMsg);
					return 1;
				case 0xE0:
					GCNewCharacterInfoRecv((PMSG_NEW_CHARACTER_INFO_RECV*)lpMsg);
					return 1;
				case 0xE1:
					GCNewCharacterCalcRecv((PMSG_NEW_CHARACTER_CALC_RECV*)lpMsg);
					return 1;
				case 0xE2:
					GCNewHealthBarRecv((PMSG_NEW_HEALTH_BAR_RECV*)lpMsg);
					return 1;
				case 0xE4:
					GCNewMessageRecv((PMSG_NEW_MESSAGE_RECV*)lpMsg);
					return 1;
				case 0xE8:
					GCWindowNameRecv((PMSG_WINDOW_NAME_RECV*)lpMsg);
					return 1;
				case 0xEA:
					CCServerNameRecv((PMSG_SERVER_NAME_RECV*)lpMsg);
					return 1;
				case 0xEB:
					GCCustomAttackRecv((PMSG_CUSTOM_ATTACK_RECV*)lpMsg);
					return 1;
			}
			break;
	}

	return ProtocolCore(head,lpMsg,size,key);
}

void GCWindowNameRecv(PMSG_WINDOW_NAME_RECV* lpMsg) // OK
{
	strcpy_s(WindowName,lpMsg->title);
}

void CCServerNameRecv(PMSG_SERVER_NAME_RECV* lpMsg) // OK
{
	memset(ServerName,0,sizeof(ServerName));

	for (int n = 0; n<lpMsg->count; n++)
	{
		PMSG_SERVER_NAME* lpInfo = (PMSG_SERVER_NAME*)(((BYTE*)lpMsg) + sizeof(PMSG_SERVER_NAME_RECV) + (sizeof(PMSG_SERVER_NAME)*n));

		memcpy(ServerName[lpInfo->index],lpInfo->Name,sizeof(ServerName[lpInfo->index]));
	}
}

void GCCustomAttackRecv(PMSG_CUSTOM_ATTACK_RECV* lpMsg) // OK
{
	CustomAttack = lpMsg->Started;
}

void GCDamageRecv(PMSG_DAMAGE_RECV* lpMsg) // OK
{
	int aIndex = MAKE_NUMBERW(lpMsg->index[0],lpMsg->index[1]) & 0x7FFF;

	if(ViewIndex == aIndex)
	{
		ViewCurHP = lpMsg->ViewCurHP;
	}

	ViewDamageHP = lpMsg->ViewDamageHP;
}

void GCCharacterCreationEnableRecv(PMSG_CHARACTER_CREATION_ENABLE_RECV* lpMsg) // OK
{
	SetExperienceTable(lpMsg->CharacterMaxLevel,lpMsg->ExperienceMultiplierConstA,lpMsg->ExperienceMultiplierConstB,lpMsg->CharacterDeleteMaxLevel);
}

void GCUserDieRecv(PMSG_USER_DIE_RECV* lpMsg) // OK
{
	int aIndex = MAKE_NUMBERW(lpMsg->index[0],lpMsg->index[1]) & 0x7FFF;

	if(ViewIndex == aIndex)
	{
		ViewCurHP = 0;
	}
}

void GCItemGetRecv(PMSG_ITEM_GET_RECV* lpMsg)
{
	if (lpMsg->result != 0xFF && lpMsg->result != 0xFE)
	{
		*(DWORD*)0x07CA2E00 = lpMsg->ViewIndex;
	}
}

void GCLifeRecv(PMSG_LIFE_RECV* lpMsg) // OK
{
	if(lpMsg->type == 0xFE)
	{
		ViewMaxHP = lpMsg->ViewHP;
	}

	if(lpMsg->type == 0xFF)
	{
		ViewCurHP = ((ViewCurHP==0)?ViewCurHP:lpMsg->ViewHP);
	}
}

void GCManaRecv(PMSG_MANA_RECV* lpMsg) // OK
{
	if(lpMsg->type == 0xFE)
	{
		ViewMaxMP = lpMsg->ViewMP;
		ViewMaxBP = lpMsg->ViewBP;
	}

	if(lpMsg->type == 0xFF)
	{
		ViewCurMP = lpMsg->ViewMP;
		ViewCurBP = lpMsg->ViewBP;
	}
}

void GCRewardExperienceRecv(PMSG_REWARD_EXPERIENCE_RECV* lpMsg) // OK
{
	ViewDamageHP = lpMsg->ViewDamageHP;
	ViewExperience = lpMsg->ViewExperience;
	ViewNextExperience = lpMsg->ViewNextExperience;
}

void GCQuestRewardRecv(PMSG_QUEST_REWARD_RECV* lpMsg) // OK
{
	int aIndex = MAKE_NUMBERW(lpMsg->index[0],lpMsg->index[1]) & 0x7FFF;

	if(ViewIndex == aIndex)
	{
		ViewPoint = lpMsg->ViewPoint;
	}
}

void GCConnectClientRecv(PMSG_CONNECT_CLIENT_RECV* lpMsg) // OK
{
	ServerCode = lpMsg->ServerCode;

	ViewIndex = MAKE_NUMBERW(lpMsg->index[0],lpMsg->index[1]);

	PMSG_HARDWAREID_INFO_SEND pMsg;

	pMsg.header.set(0xF3,0x09,sizeof(pMsg));

	memcpy(pMsg.HardwareId,GetHardwareId(),sizeof(pMsg.HardwareId));

	DataSend((BYTE*)&pMsg,pMsg.header.size);
}

void GCConnectAccountRecv(PMSG_CONNECT_ACCOUNT_RECV* lpMsg) // OK
{
	ReconnectOnConnectAccount(lpMsg->result);
}

void GCCloseClientRecv(PMSG_CLOSE_CLIENT_RECV* lpMsg) // OK
{
	ReconnectOnCloseClient(lpMsg->result);
}

void GCCharacterListRecv(PMSG_CHARACTER_LIST_RECV* lpMsg) // OK
{
	ReconnectOnCharacterList();
}

void GCCharacterInfoRecv(PMSG_CHARACTER_INFO_RECV* lpMsg) // OK
{
	ReconnectOnCharacterInfo();

	ViewReset = lpMsg->ViewReset;
	ViewPoint = lpMsg->ViewPoint;
	ViewCurHP = lpMsg->ViewCurHP;
	ViewMaxHP = lpMsg->ViewMaxHP;
	ViewCurMP = lpMsg->ViewCurMP;
	ViewMaxMP = lpMsg->ViewMaxMP;
	ViewCurBP = lpMsg->ViewCurBP;
	ViewMaxBP = lpMsg->ViewMaxBP;
	ViewStrength = lpMsg->ViewStrength;
	ViewDexterity = lpMsg->ViewDexterity;
	ViewVitality = lpMsg->ViewVitality;
	ViewEnergy = lpMsg->ViewEnergy;
	ViewLeadership = lpMsg->ViewLeadership;

	*(WORD*)(*(DWORD*)(MAIN_VIEWPORT_STRUCT)+0x1E4) = 0; // OK

	*(BYTE*)(*(DWORD*)(MAIN_VIEWPORT_STRUCT)) = 0; // OK

	STRUCT_DECRYPT

	SetByte(0x004B6F66,gProtect.m_MainInfo.AttackAnimationValue[((*(BYTE*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x0B)) & 7)]); // OK

	STRUCT_ENCRYPT
}

void GCCharacterRegenRecv(PMSG_CHARACTER_REGEN_RECV* lpMsg) // OK
{
	ViewCurHP = lpMsg->ViewCurHP;
	ViewCurMP = lpMsg->ViewCurMP;
	ViewCurBP = lpMsg->ViewCurBP;
}

void GCLevelUpRecv(PMSG_LEVEL_UP_RECV* lpMsg) // OK
{
	ViewLevel = lpMsg->Level;
	ViewPoint = lpMsg->ViewPoint;
	ViewMaxHP = lpMsg->ViewMaxHP;
	ViewMaxMP = lpMsg->ViewMaxMP;
	ViewMaxBP = lpMsg->ViewMaxBP;
	ViewExperience = lpMsg->ViewExperience;
	ViewNextExperience = lpMsg->ViewNextExperience;
}

void GCLevelUpPointRecv(PMSG_LEVEL_UP_POINT_RECV* lpMsg) // OK
{
	if(lpMsg->result >= 16 && lpMsg->result <= 20)
	{
		ViewPoint = lpMsg->ViewPoint;
		ViewMaxHP = lpMsg->ViewMaxHP;
		ViewMaxMP = lpMsg->ViewMaxMP;
		ViewMaxBP = lpMsg->ViewMaxBP;
		ViewStrength = lpMsg->ViewStrength;
		ViewDexterity = lpMsg->ViewDexterity;
		ViewVitality = lpMsg->ViewVitality;
		ViewEnergy = lpMsg->ViewEnergy;
		ViewLeadership = lpMsg->ViewLeadership;
	}
}

void GCMonsterDamageRecv(PMSG_MONSTER_DAMAGE_RECV* lpMsg) // OK
{
	ViewCurHP = lpMsg->ViewCurHP;
	ViewDamageHP = lpMsg->ViewDamageHP;
}

void GCTimeViewRecv(PMSG_TIME_VIEW_RECV* lpMsg) // OK
{
	*(WORD*)(0x0568C460) = lpMsg->time;
}

void GCNewCharacterInfoRecv(PMSG_NEW_CHARACTER_INFO_RECV* lpMsg) // OK
{
	STRUCT_DECRYPT

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x0E) = lpMsg->Level;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x62) = lpMsg->LevelUpPoint;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x18) = lpMsg->Strength;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1A) = lpMsg->Dexterity;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1C) = lpMsg->Vitality;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1E) = lpMsg->Energy;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x20) = lpMsg->Leadership;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x22) = lpMsg->Life;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x26) = lpMsg->MaxLife;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x24) = lpMsg->Mana;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x28) = lpMsg->MaxMana;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x38) = lpMsg->BP;

	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x3A) = lpMsg->MaxBP;

	//*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x44) = lpMsg->FruitAddPoint;

	//*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x46) = lpMsg->MaxFruitAddPoint;

	//*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x48) = lpMsg->FruitSubPoint;

	//*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x4A) = lpMsg->MaxFruitSubPoint;

	STRUCT_ENCRYPT

	ViewLevel = lpMsg->Level;
	ViewReset = lpMsg->ViewReset;
	ViewPoint = lpMsg->ViewPoint;
	ViewCurHP = lpMsg->ViewCurHP;
	ViewMaxHP = lpMsg->ViewMaxHP;
	ViewCurMP = lpMsg->ViewCurMP;
	ViewMaxMP = lpMsg->ViewMaxMP;
	ViewCurBP = lpMsg->ViewCurBP;
	ViewMaxBP = lpMsg->ViewMaxBP;
	ViewStrength = lpMsg->ViewStrength;
	ViewDexterity = lpMsg->ViewDexterity;
	ViewVitality = lpMsg->ViewVitality;
	ViewEnergy = lpMsg->ViewEnergy;
	ViewLeadership = lpMsg->ViewLeadership;
	ViewExperience = lpMsg->Experience;
	ViewNextExperience = lpMsg->NextExperience;
}

void GCNewCharacterCalcRecv(PMSG_NEW_CHARACTER_CALC_RECV* lpMsg) // OK
{
	ViewCurHP = lpMsg->ViewCurHP;
	ViewMaxHP = lpMsg->ViewMaxHP;
	ViewCurMP = lpMsg->ViewCurMP;
	ViewMaxMP = lpMsg->ViewMaxMP;
	ViewCurBP = lpMsg->ViewCurBP;
	ViewMaxBP = lpMsg->ViewMaxBP;
	ViewAddStrength = lpMsg->ViewAddStrength;
	ViewAddDexterity = lpMsg->ViewAddDexterity;
	ViewAddVitality = lpMsg->ViewAddVitality;
	ViewAddEnergy = lpMsg->ViewAddEnergy;
	ViewAddLeadership = lpMsg->ViewAddLeadership;
	ViewPhysiSpeed = lpMsg->ViewPhysiSpeed;
	ViewMagicSpeed = lpMsg->ViewMagicSpeed;
}

void GCNewHealthBarRecv(PMSG_NEW_HEALTH_BAR_RECV* lpMsg) // OK
{
	ClearNewHealthBar();

	for(int n=0;n < lpMsg->count;n++)
	{
		PMSG_NEW_HEALTH_RECV* lpInfo = (PMSG_NEW_HEALTH_RECV*)(((BYTE*)lpMsg)+sizeof(PMSG_NEW_HEALTH_BAR_RECV)+(sizeof(PMSG_NEW_HEALTH_RECV)*n));

		InsertNewHealthBar(lpInfo->index,lpInfo->type,lpInfo->rate);
	}
}

void GCNewMessageRecv(PMSG_NEW_MESSAGE_RECV* lpMsg) // OK
{
	if(lpMsg->type == 0)
	{
		*(DWORD*)0x081BCE04 = 138;

		((int(*)(char*))0x005BEE40)(lpMsg->message);
	}
	else
	{
		((void*(*)(char*,char*,int))0x005010A0)("",lpMsg->message,lpMsg->color);
	}
}

void DataSend(BYTE* lpMsg,DWORD size) // OK
{
	BYTE EncBuff[2048];

	if (gPacketManager.AddData(lpMsg,size) != 0 && gPacketManager.ExtractPacket(EncBuff) != 0)
	{
		BYTE send[2048];

		memcpy(send,EncBuff,size);

		if (EncBuff[0] == 0xC3 || EncBuff[0] == 0xC4)
		{
			if (EncBuff[0] == 0xC3)
			{
				BYTE save = EncBuff[1];

				((void(__thiscall*)(void*,int))0x00405F90)((void*)0x05687D00,MAIN_PACKET_SERIAL);

				EncBuff[1] = (*(BYTE*)(MAIN_PACKET_SERIAL))++;

				((void(__thiscall*)(void*,int))0x004061D0)((void*)0x05687D00,MAIN_PACKET_SERIAL);

				size = gPacketManager.Encrypt(&send[2],&EncBuff[1],(size-1))+2;

				EncBuff[1] = save;

				send[0] = 0xC3;
				send[1] = LOBYTE(size);
			}
			else
			{
				BYTE save = EncBuff[2];

				EncBuff[2] = (*(BYTE*)(MAIN_PACKET_SERIAL))++;

				size = gPacketManager.Encrypt(&send[3],&EncBuff[2],(size-2))+3;

				EncBuff[2] = save;

				send[0] = 0xC4;
				send[1] = HIBYTE(size);
				send[2] = LOBYTE(size);
			}
		}

		MySend(*(SOCKET*)(MAIN_ACTIVE_SOCKET+0x0C),(char*)send,size,0);
	}
}