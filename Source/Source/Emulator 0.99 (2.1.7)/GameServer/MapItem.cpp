// MapItem.cpp: implementation of the CMapItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MapItem.h"
#include "BloodCastle.h"
#include "ServerInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMapItem::CMapItem() // OK
{

}

CMapItem::~CMapItem() // OK
{

}

void CMapItem::Init() // OK
{
	this->m_State = OBJECT_EMPTY;
}

void CMapItem::CreateItem(int index,int level,int x,int y,float dur,BYTE Option1,BYTE Option2,BYTE Option3,BYTE NewOption,BYTE SetOption,DWORD serial) // OK
{
	this->Init();

	this->m_Level = level;

	this->m_Durability = dur;

	this->Convert(index,Option1,Option2,Option3,NewOption,SetOption);

	this->m_X = x;

	this->m_Y = y;

	this->m_Live = 1;

	this->m_Give = 0;

	this->m_State = OBJECT_CREATE;

	this->m_Time = GetTickCount()+(gServerInfo.m_ItemDropTime*1000);

	this->m_LootTime = GetTickCount()+(gServerInfo.m_ItemDropTime*500);

	this->m_CreateTime = GetTickCount();

	this->m_Serial = serial;
}

void CMapItem::DropCreateItem(int index,int level,int x,int y,float dur,BYTE Option1,BYTE Option2,BYTE Option3,BYTE NewOption,BYTE SetOption,DWORD serial,int PetLevel,int PetExp) // OK
{
	this->Init();

	this->m_Level = level;

	this->m_Durability = dur;

	this->Convert(index,Option1,Option2,Option3,NewOption,SetOption);

	this->SetPetItemInfo(PetLevel,PetExp);

	this->m_X = x;

	this->m_Y = y;

	this->m_Live = 1;

	this->m_Give = 0;

	this->m_State = OBJECT_CREATE;

	this->m_Time = GetTickCount()+(gServerInfo.m_ItemDropTime*1000);

	this->m_LootTime = GetTickCount()+(gServerInfo.m_ItemDropTime*500);

	this->m_CreateTime = GetTickCount();

	this->m_Serial = serial;
}
