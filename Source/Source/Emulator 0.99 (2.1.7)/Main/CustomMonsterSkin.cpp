// CustomMonsterSkin.cpp: implementation of the CCustomMonsterSkin class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomMonsterSkin.h"

CCustomMonsterSkin gCustomMonsterSkin;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomMonsterSkin::CCustomMonsterSkin() // OK
{
	this->Init();
}

CCustomMonsterSkin::~CCustomMonsterSkin() // OK
{

}

void CCustomMonsterSkin::Init() // OK
{
	for (int n=0; n< MAX_CUSTOM_MONSTER_SKIN;n++)
	{
		this->m_CustomMonsterSkinInfo[n].Index = -1;
	}
}

void CCustomMonsterSkin::Load(CUSTOM_MONSTER_SKIN_INFO* info) // OK
{
	for (int n=0; n< MAX_CUSTOM_MONSTER_SKIN;n++)
	{
		this->SetInfo(info[n]);
	}
}

void CCustomMonsterSkin::SetInfo(CUSTOM_MONSTER_SKIN_INFO info) // OK
{
	if (info.Index < 0 || info.Index >= MAX_CUSTOM_MONSTER_SKIN)
	{
		return;
	}

	this->m_CustomMonsterSkinInfo[info.Index] = info;
}