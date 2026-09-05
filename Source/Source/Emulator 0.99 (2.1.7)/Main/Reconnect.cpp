#include "stdafx.h"
#include "Reconnect.h"
#include "HackCheck.h"
#include "Offset.h"
#include "Protect.h"
#include "Protocol.h"
#include "Shop.h"
#include "Util.h"

char GameServerAddress[16];
WORD GameServerPort;
char ReconnectAccount[11];
char ReconnectPassword[11];
char ReconnectName[11];
char ReconnectMapServerAddress[16];
WORD ReconnectMapServerPort;
DWORD ReconnectStatus = RECONNECT_STATUS_NONE;
DWORD ReconnectProgress = RECONNECT_PROGRESS_NONE;
DWORD ReconnectCurTime = 0;
DWORD ReconnectMaxTime = 0;
DWORD ReconnectCurWait = 0;
DWORD ReconnectMaxWait = 0;
DWORD ReconnectAuthSend = 0;

void InitReconnect() // OK
{
	if(gProtect.m_MainInfo.ReconnectTime == 0)
	{
		return;
	}

	SetCompleteHook(0xE9,0x005C1435,&ReconnectGetAccountInfo);

	SetCompleteHook(0xE9,0x005C8A93,&ReconnectCheckConnection);

	SetCompleteHook(0xE9,0x004A7D0A,&ReconnectCloseSocket);

	SetCompleteHook(0xE9,0x005B4B8B,&ReconnectMenuExitGame);

	SetCompleteHook(0xE8,0x00489BBD,&ReconnectCreateConnection);

	SetCompleteHook(0xE8,0x004A453F,&ReconnectCreateConnection);

	SetCompleteHook(0xE8,0x005C089B,&ReconnectCreateConnection);

	SetCompleteHook(0xE8,0x005C119E,&ReconnectCreateConnection);

	SetCompleteHook(0xE8,0x005444E5,&ReconnectMainProc);
}

void ReconnectMainProc() // OK
{
	((void(*)())0x005454C0)();

	if(*(DWORD*)(MAIN_SCREEN_STATE) != 5)
	{
		return;
	}

	if(ReconnectStatus != RECONNECT_STATUS_RECONNECT)
	{
		return;
	}

	ReconnectDrawInterface();

	if((GetTickCount()-ReconnectMaxTime) > ReconnectMaxWait)
	{
		ReconnectSetInfo(RECONNECT_STATUS_DISCONNECT,RECONNECT_PROGRESS_NONE,0,0);
		((void(__thiscall*)(void*))0x004A7D00)((void*)MAIN_ACTIVE_SOCKET);
		return;
	}

	if((GetTickCount()-ReconnectCurTime) < ReconnectCurWait)
	{
		return;
	}

	switch(ReconnectProgress)
	{
		case RECONNECT_PROGRESS_NONE:
			ReconnecGameServerLoad();
			break;
		case RECONNECT_PROGRESS_CONNECTED:
			ReconnecGameServerAuth();
			break;
	}

	ReconnectCurTime = GetTickCount();
}

void ReconnectDrawInterface() // OK
{
	float progress = ((ReconnectMaxWait==0)?0:(((GetTickCount()-ReconnectMaxTime)*160.0f)/(float)ReconnectMaxWait));

	EnableAlphaTest(true);
	glColor4f(0.f,0.f,0.f,0.4f);
	pDrawBarForm(240,88,160,20.0f,0,0);

	glColor3f(1.f,0.f,0.f);

	pDrawBarForm(241,101,((progress>158)?158:progress),6.0f,0,0);
	EnableAlphaBlend();

	glColor3f(1.0,1.0,1.0);

	DrawInterfaceText(320,91,pGetTextLine((801+ReconnectProgress)));

	DisableAlphaBlend();

	glColor3f(1.0,1.0,1.0);
}

void ReconnectSetInfo(DWORD status,DWORD progress,DWORD CurWait,DWORD MaxWait) // OK
{
	ReconnectStatus = status;

	ReconnectProgress = progress;

	ReconnectCurTime = GetTickCount();

	ReconnectMaxTime = GetTickCount();

	ReconnectCurWait = CurWait;

	ReconnectMaxWait = MaxWait;

	ReconnectAuthSend = ((status==RECONNECT_STATUS_NONE)?0:ReconnectAuthSend);
}

void ReconnecGameServerLoad() // OK
{
	if(ReconnectCreateConnection(GameServerAddress,GameServerPort) != 0)
	{
		*(DWORD*)(MAIN_CONNECTION_STATUS) = 1;

		ReconnectSetInfo(RECONNECT_STATUS_RECONNECT,RECONNECT_PROGRESS_CONNECTED,10000,30000);
	}
}

void ReconnecGameServerAuth() // OK
{
	if(((ReconnectAuthSend==0)?(ReconnectAuthSend++):ReconnectAuthSend) != 0)
	{
		return;
	}

	PMSG_CONNECT_ACCOUNT_SEND pMsg;

	pMsg.header.setE(0xF1,0x01,sizeof(pMsg));

	PacketArgumentEncrypt(pMsg.account,ReconnectAccount,(sizeof(ReconnectAccount)-1));

	PacketArgumentEncrypt(pMsg.password,ReconnectPassword,(sizeof(ReconnectPassword)-1));

	pMsg.TickCount = GetTickCount();

	pMsg.ClientVersion[0] = (*(BYTE*)(0x006069EC))-1;

	pMsg.ClientVersion[1] = (*(BYTE*)(0x006069ED))-2;

	pMsg.ClientVersion[2] = (*(BYTE*)(0x006069EE))-3;

	pMsg.ClientVersion[3] = (*(BYTE*)(0x006069EF))-4;

	pMsg.ClientVersion[4] = (*(BYTE*)(0x006069F0))-5;

	memcpy(pMsg.ClientSerial,(void*)0x006069F4,sizeof(pMsg.ClientSerial));

	DataSend((BYTE*)&pMsg,pMsg.header.size);
}

void ReconnectOnCloseSocket() // OK
{
	if(*(DWORD*)(MAIN_SCREEN_STATE) == 5 && ReconnectStatus != RECONNECT_STATUS_DISCONNECT)
	{
		ReconnectSetInfo(RECONNECT_STATUS_RECONNECT,RECONNECT_PROGRESS_NONE,30000,gProtect.m_MainInfo.ReconnectTime);

		ReconnectAuthSend = 0;

		ReconnectViewportDestroy();

		*(DWORD*)(MAIN_PARTY_MEMBER_COUNT) = 0;

		STRUCT_DECRYPT

		memcpy(ReconnectName,(void*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x00),sizeof(ReconnectName));

		STRUCT_ENCRYPT
	}
}

void ReconnectOnConnectAccount(BYTE result) // OK
{
	if(ReconnectProgress == RECONNECT_PROGRESS_CONNECTED)
	{
		if(ReconnectAuthSend != 0)
		{
			if(result == 1)
			{
				PMSG_CHARACTER_LIST_SEND pMsg;

				pMsg.header.set(0xF3,0x00,sizeof(pMsg));

				DataSend((BYTE*)&pMsg,pMsg.header.size);

				ReconnectSetInfo(RECONNECT_STATUS_RECONNECT,RECONNECT_PROGRESS_JOINED,30000,30000);
			}
			else
			{
				if(result == 3)
				{
					ReconnectSetInfo(RECONNECT_STATUS_RECONNECT,RECONNECT_PROGRESS_CONNECTED,10000,30000);
					ReconnectAuthSend = 0;
				}
				else
				{
					ReconnectSetInfo(RECONNECT_STATUS_DISCONNECT,RECONNECT_PROGRESS_NONE,0,0);
					((void(__thiscall*)(void*))0x004A7D00)((void*)MAIN_ACTIVE_SOCKET);
				}
			}
		}
	}
}

void ReconnectOnCloseClient(BYTE result) // OK
{
	if(ReconnectStatus != RECONNECT_STATUS_RECONNECT)
	{
		if(result == 0 || result == 2)
		{
			ReconnectSetInfo(RECONNECT_STATUS_DISCONNECT,RECONNECT_PROGRESS_NONE,0,0);
		}
	}
}

void ReconnectOnCharacterList() // OK
{
	if(ReconnectProgress == RECONNECT_PROGRESS_JOINED)
	{
		PMSG_CHARACTER_INFO_SEND pMsg;

		pMsg.header.set(0xF3,0x03,sizeof(pMsg));

		memcpy(pMsg.name,ReconnectName,sizeof(pMsg.name));

		DataSend((BYTE*)&pMsg,pMsg.header.size);

		ReconnectSetInfo(RECONNECT_STATUS_RECONNECT,RECONNECT_PROGRESS_CHAR_LIST,30000,30000);
	}
}

void ReconnectOnCharacterInfo() // OK
{
	ReconnectSetInfo(RECONNECT_STATUS_NONE,RECONNECT_PROGRESS_NONE,0,0);
}

void ReconnectViewportDestroy() // OK
{
	DWORD count = 0;

	DWORD ViewportAddress = 0;

	do
	{
		BYTE send[256];

		PMSG_VIEWPORT_DESTROY_RECV pMsg;

		pMsg.header.set(0x14,0);

		int size = sizeof(pMsg);

		pMsg.count = 0;

		PMSG_VIEWPORT_DESTROY info;

		for(;count < MAX_MAIN_VIEWPORT;count++)
		{
			if((ViewportAddress=(pViewportAddress+(count*0x40C))) == 0)
			{
				continue;
			}

			if(*(BYTE*)(ViewportAddress) == 0)
			{
				continue;
			}

			info.index[0] = SET_NUMBERHB((*(WORD*)(ViewportAddress+0x1E4)));
			info.index[1] = SET_NUMBERLB((*(WORD*)(ViewportAddress+0x1E4)));

			if((size+sizeof(info)) > sizeof(send))
			{
				break;
			}
			else
			{
				memcpy(&send[size],&info,sizeof(info));
				size += sizeof(info);

				pMsg.count++;
			}
		}

		pMsg.header.size = size;

		memcpy(send,&pMsg,sizeof(pMsg));

		ProtocolCoreEx(pMsg.header.head,send,size,-1);
	}
	while(count < MAX_MAIN_VIEWPORT);
}

BOOL ReconnectCreateConnection(char* address,WORD port) // OK
{
	if(PORT_RANGE(port) != 0 && GameServerAddress != address)
	{
		if(strcmp(ReconnectMapServerAddress,address) != 0 || ReconnectMapServerPort != port)
		{
			wsprintf(GameServerAddress,"%s",address);

			GameServerPort = port;

			memset(ReconnectMapServerAddress,0,sizeof(ReconnectMapServerAddress));

			ReconnectMapServerPort = 0;
		}
	}

	return ((BOOL(*)(char*,WORD))0x00489400)(address,port);
}

__declspec(naked) void ReconnectGetAccountInfo() // OK
{
	static DWORD ReconnectGetAccountInfoAddress1 = 0x00415150;
	static DWORD ReconnectGetAccountInfoAddress2 = 0x005C1458;

	_asm
	{
		Push 0x07C4A1E8;
		Push 0x00610AFC;
		Push 0x05687D28;
		Mov Dword Ptr Ds:[0x081BCDB0],Eax;
		Mov Dword Ptr DS:[0x081B9608],0x00;
		Call [ReconnectGetAccountInfoAddress1];
		Push 0x0A
		Lea Eax,Dword Ptr Ss:[0x07C4A1E8]
		Push Eax
		Lea Ecx,ReconnectAccount
		Push Ecx
		Call [MemoryCpy]
		Add Esp,0x0C
		Push 0x0A
		Lea Edx,Dword Ptr Ss:[0x07C4A2E8]
		Push Edx
		Lea Eax,ReconnectPassword
		Push Eax
		Call [MemoryCpy]
		Add Esp,0x0C
		Jmp [ReconnectGetAccountInfoAddress2];
	}
}

__declspec(naked) void ReconnectCheckConnection() // OK
{
	static DWORD ReconnectCheckConnectionAddress1 = 0x005C8A98;
	static DWORD ReconnectCheckConnectionAddress2 = 0x005C8AD3;

	_asm
	{
		Cmp Eax,-1
		Jnz EXIT
		Mov Ecx,ReconnectStatus
		Cmp Ecx,RECONNECT_STATUS_RECONNECT
		Je EXIT
		Jmp [ReconnectCheckConnectionAddress1]
		EXIT:
		Jmp [ReconnectCheckConnectionAddress2]
	}
}

__declspec(naked) void ReconnectCloseSocket() // OK
{
	static DWORD ReconnectCloseSocketAddress1 = 0x004A7D14;

	_asm
	{
		Mov Eax,Dword Ptr Ds:[MAIN_CONNECTION_STATUS]
		Cmp Eax,0x00
		Je EXIT
		Mov Eax,Dword Ptr Ds:[Esi+0x0C]
		Push Eax
		Call [CheckSocketPort]
		Add Esp,0x04
		Je EXIT
		Call [ReconnectOnCloseSocket]
		EXIT:
		Mov Dword Ptr Ds:[MAIN_CONNECTION_STATUS],Eax
		Jmp [ReconnectCloseSocketAddress1]
	}
}

__declspec(naked) void ReconnectMenuExitGame() // OK
{
	static DWORD ReconnectMenuExitGameAddress1 = 0x005B4B90;

	_asm
	{
		Mov Eax,ReconnectStatus
		Cmp Eax,RECONNECT_STATUS_RECONNECT
		Jnz EXIT
		Push 0
		Call [ExitProcess]
		EXIT:
		Mov ReconnectStatus,RECONNECT_STATUS_DISCONNECT
		Push 0x00610900
		Jmp [ReconnectMenuExitGameAddress1]
	}
}