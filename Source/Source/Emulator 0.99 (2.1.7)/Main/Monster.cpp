#include "stdafx.h"
#include "Monster.h"
#include "CustomMonster.h"
#include "CustomMonsterSkin.h"
#include "Offset.h"
#include "Util.h"

void InitMonster() // OK
{
	SetByte(0x004B98C9,0xD0); // Monster Kill

	SetByte(0x004B98CA,0x07); // Monster Kill

	SetCompleteHook(0xE8,0x004CE366,&CreateMonster);

	SetCompleteHook(0xE8,0x0048FECC,&SettingMonster);

	SetCompleteHook(0xE8,0x004904C5,&SettingMonster);

	SetCompleteHook(0xE8,0x00490819,&SettingMonster);

	SetCompleteHook(0xE8,0x004D0D29,&SettingMonster);

	SetCompleteHook(0xE8,0x004FB6D0,&SettingMonster);

	SetCompleteHook(0xE8,0x00548E63,&SettingMonster);
}

DWORD CreateMonster(int index,int x,int y,int key) // OK
{
	CUSTOM_MONSTER_INFO* lpInfo = gCustomMonster.GetInfoByIndex(index);

	if(lpInfo != 0)
	{
		if(lpInfo->Type != 0 && lpInfo->Type != 3)
		{
			index += 206;
		}

		DWORD o = 204 * index + *(DWORD*)0x0568E60C;

		if(*(BYTE*)0x006081FC == 0 || *(WORD*)(o + 38) <= 0)
		{
			char path[MAX_PATH] = {0};

			wsprintf(path,"Data\\%s",lpInfo->FolderPath);

			pLoadItemModel(index,path,lpInfo->ModelPath,-1);

			if(lpInfo->Type == 0 || lpInfo->Type == 3)
			{
				for(int i=0;i < *(WORD*)(o + 38);++i)
				{
					*(float*)(*(DWORD*)(o + 48) + 16 * i + 4) = 0.25f;
				}
			}
			else
			{
				*(float*)(*(DWORD*)(o + 48) + 4) = 0.25f;
				*(float*)(*(DWORD*)(o + 48) + 20) = 0.2f;
				*(float*)(*(DWORD*)(o + 48) + 36) = 0.34f;
				*(float*)(*(DWORD*)(o + 48) + 52) = 0.33f;
				*(float*)(*(DWORD*)(o + 48) + 68) = 0.33f;
				*(float*)(*(DWORD*)(o + 48) + 84) = 0.5f;
				*(float*)(*(DWORD*)(o + 48) + 100) = 0.55f;
				*(BYTE*)(*(DWORD*)(o + 48) + 96) = 1;
			}
		}

		pLoadItemTexture(index,lpInfo->FolderPath,GL_REPEAT,GL_NEAREST,GL_TRUE);

		return pCreateCharacter(key,(lpInfo->Type>2)?433:index,x,y,0);
	}

	return pCreateMonster(index,x,y,key);
}

DWORD SettingMonster(int index,int x,int y,int key) // OK
{
	DWORD o = ((DWORD(*)(int,int,int,int))0x004CE350)(index,x,y,key);

	CUSTOM_MONSTER_INFO* lpInfo = gCustomMonster.GetInfoByIndex(index);

	if(lpInfo != 0)
	{
		*(float*)(o + 12) = lpInfo->Size;

		*(BYTE*)(o + 132) = (lpInfo->Type== 0||lpInfo->Type==3)?4:2;

		memcpy((DWORD*)(o + 449),lpInfo->Name,sizeof(lpInfo->Name));

		*(DWORD*)(o + 862) = (lpInfo->Type==2)?43:index;

		if(lpInfo->Type > 2)
		{
			pSetCharacterScale(o);
		}
	}

	return o;
}