// ItemValue.cpp: implementation of the CItemValue class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ItemValue.h"
#include "ItemStack.h"
#include "Offset.h"
#include "Util.h"

CItemValue gItemValue;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CItemValue::CItemValue() // OK
{
	this->Init();
}

CItemValue::~CItemValue() // OK
{

}

void CItemValue::Init() // OK
{
	for(int n=0;n<MAX_ITEM_VALUE_INFO;n++)
	{
		this->m_ItemValueInfo[n].Index=-1;
	}
}

void CItemValue::Load(ITEM_VALUE_INFO* info) // OK
{
	for(int n=0;n<MAX_ITEM_VALUE_INFO;n++)
	{
		this->SetInfo(info[n]);
	}
}

void CItemValue::SetInfo(ITEM_VALUE_INFO info) // OK
{
	if(info.Index<0 || info.Index >= MAX_ITEM_VALUE_INFO)
	{
		return;
	}

	this->m_ItemValueInfo[info.Index]=info;
}

ITEM_VALUE_INFO* CItemValue::GetInfo(int index) // OK
{
	if(index<0 || index >= MAX_ITEM_VALUE_INFO)
	{
		return 0;
	}

	if(this->m_ItemValueInfo[index].Index != index)
	{
		return 0;
	}

	return &this->m_ItemValueInfo[index];
}