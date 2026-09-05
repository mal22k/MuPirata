// CustomItemBow.h: interface for the CCustomItemBow class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CUSTOM_ITEM_BOW 200

struct CUSTOM_ITEM_BOW_INFO
{
	int Index;
	int ItemIndex;
	int Number;
	int Effect;
	int ColorR;
	int ColorG;
	int ColorB;
	char ModelName[32];
};

class CCustomItemBow
{
public:
	bool Load(char* path);
	void SetInfo(CUSTOM_ITEM_BOW_INFO info);
	CUSTOM_ITEM_BOW_INFO m_CustomItemBowInfo[MAX_CUSTOM_ITEM_BOW];
};

extern CCustomItemBow gCustomItemBow;