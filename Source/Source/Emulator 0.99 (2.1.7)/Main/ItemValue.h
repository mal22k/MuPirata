// ItemValue.h: interface for the CItemValue class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Item.h"

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
	CItemValue();
	virtual ~CItemValue();
	void Init();
	void Load(ITEM_VALUE_INFO* info);
	void SetInfo(ITEM_VALUE_INFO info);
	ITEM_VALUE_INFO* GetInfo(int index);
private:
	ITEM_VALUE_INFO m_ItemValueInfo[MAX_ITEM_VALUE_INFO];
};

extern CItemValue gItemValue;
