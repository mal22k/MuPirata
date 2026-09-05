#include "stdafx.h"
#include "ServerList.h"
#include "Offset.h"
#include "Util.h"

int ServerCode = -1;
char ServerName[32][400];

void InitServerList() // OK
{
	SetByte(0x005C0138,0xEB);

	SetByte(0x005C01DD,0x50);

	MemorySet(0x005C01B2,0x90,0x26);

	SetCompleteHook(0xE8,0x005C0259,&PrintServerName1);

	SetCompleteHook(0xE8,0x00585DB9,&PrintServerName2);

	SetCompleteHook(0xE8,0x0043D325,&PrintServerName3);
}

void PrintServerName1(char* a,char* b,char* c,DWORD d) // OK
{
	wsprintf(a,"%s",ServerName[d]);
}

void PrintServerName2(char* a,char* b,char* c,DWORD d) // OK
{
	wsprintf(a,"%s",ServerName[ServerCode]);
}

void PrintServerName3(char* a,char* b,DWORD c) // OK
{
	wsprintf(a,"%s",ServerName[c-1]);
}