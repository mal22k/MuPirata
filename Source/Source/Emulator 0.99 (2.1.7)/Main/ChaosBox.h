#pragma once

#include "Protocol.h"

enum eChaosMixNumber
{
	CHAOS_MIX_CHAOS_ITEM = 1,
	CHAOS_MIX_DEVIL_SQUARE = 2,
	CHAOS_MIX_PLUS_ITEM_LEVEL1 = 3,
	CHAOS_MIX_PLUS_ITEM_LEVEL2 = 4,
	CHAOS_MIX_DINORANT = 5,
	CHAOS_MIX_FRUIT = 6,
	CHAOS_MIX_WING1 = 7,
	CHAOS_MIX_BLOOD_CASTLE = 8,
	CHAOS_MIX_WING2 = 11,
	CHAOS_MIX_PET1 = 13,
	CHAOS_MIX_PET2 = 14,
	CHAOS_MIX_PLUS_ITEM_LEVEL3 = 22,
	CHAOS_MIX_PLUS_ITEM_LEVEL4 = 23,
	CHAOS_MIX_WING3 = 24,
};

//**********************************************//
//************ Client -> GameServer ************//
//**********************************************//

struct PMSG_CHAOS_MIX_RATE_SEND
{
	PBMSG_HEAD header; // C1:88
	DWORD type;
};

//**********************************************//
//************ GameServer -> Client ************//
//**********************************************//

struct PMSG_CHAOS_MIX_RATE_RECV
{
	PBMSG_HEAD header; // C1:88
	int rate;
	int money;
};

//**********************************************//
//**********************************************//
//**********************************************//

void InitChaosBox();
void ChaosBoxMixSend();
bool ChaosBoxMixCheck();
void ChaosBoxConvertMoney(int money,char* target);
void PrintPlayerChaosRate(char* a);
void PrintPlayerChaosMoney(char* a,char* b,char* c);
void PrintPlayerPetMixMoney(int a,int b,char* c);
void GCChaosMixRateRecv(PMSG_CHAOS_MIX_RATE_RECV* lpMsg);;