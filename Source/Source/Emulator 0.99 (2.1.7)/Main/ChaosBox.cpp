#include "stdafx.h"
#include "ChaosBox.h"
#include "Offset.h"
#include "Protocol.h"
#include "Util.h"

DWORD MixRate = 0;
DWORD MixMoney = 0;

void InitChaosBox() // OK
{
	SetCompleteHook(0xE8,0x0042BB3C,&PrintPlayerChaosRate);

	SetCompleteHook(0xE8,0x005963F0,&PrintPlayerChaosRate);

	SetCompleteHook(0xE8,0x0042BB9C,&PrintPlayerPetMixMoney);

	SetCompleteHook(0xE8,0x00596430,&PrintPlayerChaosMoney);
}

void ChaosBoxMixSend() // OK
{
	DWORD MixUpdate = ((pPetMixIndex==1)?CHAOS_MIX_PET1:((pPetMixIndex==2)?CHAOS_MIX_PET2:pChaosMixIndex));

	if(MixUpdate != 0)
	{
		PMSG_CHAOS_MIX_RATE_SEND pMsg;
		
		pMsg.header.set(0x88,sizeof(pMsg));
		
		pMsg.type = MixUpdate;
		
		DataSend((BYTE*)&pMsg,pMsg.header.size);
	}
}

bool ChaosBoxMixCheck() // OK
{
	if(pPetMixIndex != 0)
	{
		return 1;
	}

	switch(pChaosMixIndex)
	{
		case CHAOS_MIX_CHAOS_ITEM:
		case CHAOS_MIX_DEVIL_SQUARE:
		case CHAOS_MIX_PLUS_ITEM_LEVEL1:
		case CHAOS_MIX_PLUS_ITEM_LEVEL2:
		case CHAOS_MIX_DINORANT:
		case CHAOS_MIX_FRUIT:
		case CHAOS_MIX_WING1:
		case CHAOS_MIX_BLOOD_CASTLE:
		case CHAOS_MIX_WING2:
		case CHAOS_MIX_PLUS_ITEM_LEVEL3:
		case CHAOS_MIX_PLUS_ITEM_LEVEL4:
		case CHAOS_MIX_WING3:
			return 1;
	}

	return 0;
}

void ChaosBoxConvertMoney(int money,char* target) // OK
{
	if(money >= 1000000000)
	{
		wsprintf(target,"%d,%03d,%03d,%03d",money/1000000000,money%1000000000/1000000,money%1000000/1000,money%1000);
	}
	else if(money >= 1000000)
	{
		wsprintf(target,"%d,%03d,%03d",money%1000000000/1000000,money%1000000/1000,money%1000);
	}
	else if(money > 1000)
	{
		wsprintf(target,"%d,%03d",money%1000000/1000,money%1000);
	}
	else
	{
		wsprintf(target,"%d",money%1000);
	}
}

void PrintPlayerChaosRate(char* a) // OK
{
	ChaosBoxMixSend();

	if(ChaosBoxMixCheck() != 0)
	{
		wsprintf(a,pGetTextLine(584),MixRate);
	}
	else
	{
		wsprintf(a,pGetTextLine(584),0);
	}
}

void PrintPlayerChaosMoney(char* a,char* b,char* c) // OK
{
	if(ChaosBoxMixCheck() != 0)
	{
		ChaosBoxConvertMoney(MixMoney,c);
	}
	else
	{
		wsprintf(c,"0");
	}

	wsprintf(a,pGetTextLine(585),c);
}

void PrintPlayerPetMixMoney(int a,int b,char* c) // OK
{
	char buff[50];

	if(ChaosBoxMixCheck() != 0)
	{
		char value[32];

		ChaosBoxConvertMoney(MixMoney,value);

		wsprintf(buff,pGetTextLine(585),value);
	}

	DrawInterfaceText(a,b,buff);
}

void GCChaosMixRateRecv(PMSG_CHAOS_MIX_RATE_RECV* lpMsg) // OK
{
	MixRate = lpMsg->rate;

	MixMoney = lpMsg->money;
}