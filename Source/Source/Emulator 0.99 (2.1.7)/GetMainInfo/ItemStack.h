// ItemStack.h: interface for the CItemStack class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_ITEM_STACK_INFO 2000

struct ITEM_STACK_INFO
{
	int Index;
	int ItemIndex;
	int Level;
	int MaxStack;
	int CreateItemIndex;
};

class CItemStack
{
public:
	bool Load(char* path);
	void SetInfo(ITEM_STACK_INFO info);
	ITEM_STACK_INFO m_ItemStackInfo[MAX_ITEM_STACK_INFO];
};

extern CItemStack gItemStack;