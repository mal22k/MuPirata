#include "stdafx.h"
#include "Map.h"
#include "CustomMap.h"
#include "Offset.h"
#include "Util.h"

void InitMap()  // OK
{
	SetByte(0x005ACE7A,0xEB); // Fix Check .map files

	SetByte(0x005ACE7F,0xEB); // Fix Check .map files

	SetByte(0x005ACEEF,0xEB); // Fix Check .att files

	SetByte(0x005ACEF4,0xEB); // Fix Check .att files

	SetByte(0x005ACF61,0xEB); // Fix Check .obj files

	SetByte(0x005ACF66,0xEB); // Fix Check .obj files

	SetCompleteHook(0xE8,0x0048EC8A,&LoadMapName);

	SetCompleteHook(0xE9,0x005881A2,&LoadMapNameParty);
}

char* LoadMapName(int index) //OK
{
	CUSTOM_MAP_INFO* lpInfo = gCustomMap.GetInfoByNumber(index);

	if(lpInfo != 0)
	{
		return lpInfo->MapName;
	}

	return ((char*(*)(int))0x00587E20)(index);
}

__declspec(naked) void LoadMapNameParty() // OK
{
	static DWORD LoadMapNamePartyAddress1 = 0x005881AF;
	static DWORD index;
	static char* MapName;

	_asm 
	{
		PushAd
		Movsx Ecx,Byte Ptr Ds:[Esi+0x0C]
		Mov index,Ecx
		PopAd
	}
		
	MapName = gCustomMap.GetMapName(index);

	_asm
	{
		Push MapName
		Lea Eax,[Ebp-0x124]
		Push 0x0060810C
		Push Eax
		Jmp[LoadMapNamePartyAddress1]
	}
}