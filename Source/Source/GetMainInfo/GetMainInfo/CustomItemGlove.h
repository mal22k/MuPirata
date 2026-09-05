// CustomItemGlove.h: interface for the CCustomItemGlove class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CUSTOM_ITEM_GLOVE 10

struct CUSTOM_ITEM_GLOVE_INFO
{
	int Index;
	int ItemIndex;
	int ItemL;
	int ItemR;
	int ColorR;
	int ColorG;
	int ColorB;
	char ModelName[32];
	char ModelNameL[32];
	char ModelNameR[32];
};

class CCustomItemGlove
{
public:
	bool Load(char* path);
	void SetInfo(CUSTOM_ITEM_GLOVE_INFO info);
	CUSTOM_ITEM_GLOVE_INFO m_CustomItemGloveInfo[MAX_CUSTOM_ITEM_GLOVE];
};

extern CCustomItemGlove gCustomItemGlove;