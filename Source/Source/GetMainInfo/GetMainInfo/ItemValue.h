// ItemValue.h: interface for the CItemValue class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_ITEM_VALUE_INFO 2000

struct ITEM_VALUE_INFO
{
	int Index;
	int ItemIndex;
	int Level;
	int Grade;
	int Value;
};

class CItemValue
{
public:
	bool Load(char* path);
	void SetInfo(ITEM_VALUE_INFO info);
	ITEM_VALUE_INFO m_ItemValueInfo[MAX_ITEM_VALUE_INFO];
};

extern CItemValue gItemValue;