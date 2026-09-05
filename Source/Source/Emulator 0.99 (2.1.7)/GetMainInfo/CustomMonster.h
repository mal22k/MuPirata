// CustomMonster.h: interface for the CCustomMonster class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CUSTOM_MONSTER 100
#define MAX_CUSTOM_MONSTER_SKIN 1000

struct CUSTOM_MONSTER_INFO
{
	int Index;
	int MonsterIndex;
	int Type;
	float Size;
	char FolderPath[128];
	char ModelPath[32];
	char Name[25];
};

struct CUSTOM_MONSTER_SKIN_INFO
{
	int Index;
	int MonsterIndex;
	int slot;
	int ItemIndex;
	int ItemLevel;
	int Option1;
	int Excellent;
};

class CCustomMonster
{
public:
	bool Load(char* path);
	void SetInfo(CUSTOM_MONSTER_INFO info);
	void SetInfoSkin(CUSTOM_MONSTER_SKIN_INFO info);
	CUSTOM_MONSTER_INFO m_CustomMonsterInfo[MAX_CUSTOM_MONSTER];
	CUSTOM_MONSTER_SKIN_INFO m_CustomMonsterSkinInfo[MAX_CUSTOM_MONSTER_SKIN];
};

extern CCustomMonster gCustomMonster;
