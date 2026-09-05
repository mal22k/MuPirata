// ItemStack.cpp: implementation of the CItemStack class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ItemStack.h"
#include "Util.h"

CItemStack gItemStack;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CItemStack::CItemStack() // OK
{
	this->Init();
}

CItemStack::~CItemStack() // OK
{

}

void CItemStack::Init() // OK
{
	for(int n=0;n<MAX_ITEM_STACK_INFO;n++)
	{
		this->m_ItemStackInfo[n].Index=-1;
	}
}

void CItemStack::Load(ITEM_STACK_INFO* info) // OK
{
	for(int n=0;n<MAX_ITEM_STACK_INFO;n++)
	{
		this->SetInfo(info[n]);
	}
}

void CItemStack::SetInfo(ITEM_STACK_INFO info) // OK
{
	if(info.Index<0 || info.Index >= MAX_ITEM_STACK_INFO)
	{
		return;
	}

	this->m_ItemStackInfo[info.Index]=info;
}

ITEM_STACK_INFO* CItemStack::GetInfo(int index) // OK
{
	if(index<0 || index >= MAX_ITEM_STACK_INFO)
	{
		return 0;
	}

	if(this->m_ItemStackInfo[index].Index != index)
	{
		return 0;
	}

	return &this->m_ItemStackInfo[index];
}

int CItemStack::GetItemMaxStack(int ItemIndex,int Level) // OK
{
	for(int n=0;n<MAX_ITEM_STACK_INFO;n++)
	{
		ITEM_STACK_INFO* lpInfo=this->GetInfo(n);

		if(lpInfo == 0)
		{
			continue;
		}

		if(lpInfo->ItemIndex != ItemIndex)
		{
			continue;
		}

		if(lpInfo->Level != -1 && lpInfo->Level != Level)
		{
			continue;
		}

		return lpInfo->MaxStack;
	}

	return 0;
}
