#include "stdafx.h"
#include "Shop.h"
#include "ItemStack.h"
#include "ItemValue.h"
#include "Offset.h"
#include "Protocol.h"
#include "Util.h"

void InitShop() // OK
{
	SetCompleteHook(0xE8,0x00556C0A,&NpcTalkClose);
}

void NpcTalkClose() // OK
{
	if(*(DWORD*)0x6482F8 == 0)
	{
		PBMSG_HEAD pMsg;

		pMsg.set(0x31,sizeof(pMsg));

		DataSend((BYTE*)&pMsg,pMsg.size);
	}
	
	pPlayBuffer(25,0,0);
}