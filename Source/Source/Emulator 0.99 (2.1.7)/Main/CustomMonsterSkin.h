// CustomMonsterSkin.h: interface for the CCustomMonsterSkin class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CUSTOM_MONSTER_SKIN 1000

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

class CCustomMonsterSkin
{
public:
	CCustomMonsterSkin();
	virtual ~CCustomMonsterSkin();
	void Init();
	void Load(CUSTOM_MONSTER_SKIN_INFO* info);
	void SetInfo(CUSTOM_MONSTER_SKIN_INFO info);
	CUSTOM_MONSTER_SKIN_INFO m_CustomMonsterSkinInfo[MAX_CUSTOM_MONSTER_SKIN];
};

extern CCustomMonsterSkin gCustomMonsterSkin;