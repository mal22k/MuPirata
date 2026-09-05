// CustomItemGlove.cpp: implementation of the CCustomItemGlove class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomItemGlove.h"
#include "MemScript.h"

CCustomItemGlove gCustomItemGlove;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CCustomItemGlove::Load(char* path) // OK
{
	CMemScript* lpMemScript = new CMemScript;

	if(lpMemScript == 0)
	{
		printf(MEM_SCRIPT_ALLOC_ERROR, path);
		return 0;
	}

	if(lpMemScript->SetBuffer(path) == 0)
	{
		printf(lpMemScript->GetLastError());
		delete lpMemScript;
		return 0;
	}

	for(int n=0;n<MAX_CUSTOM_ITEM_GLOVE;n++)
	{
		this->m_CustomItemGloveInfo[n].Index = -1;
	}

	try
	{
		while(true)
		{
			if(lpMemScript->GetToken() == TOKEN_END)
			{
				break;
			}

			if(strcmp("end",lpMemScript->GetString()) == 0)
			{
				break;
			}

			CUSTOM_ITEM_GLOVE_INFO info;

			memset(&info,0,sizeof(info));

			static int CustomItemGloveCount = 0;

			info.Index = CustomItemGloveCount++;

			info.ItemIndex = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

			info.ItemL = lpMemScript->GetAsNumber();

			info.ItemR = lpMemScript->GetAsNumber();

			info.ColorR = lpMemScript->GetAsNumber();

			info.ColorG = lpMemScript->GetAsNumber();

			info.ColorB = lpMemScript->GetAsNumber();

			strcpy_s(info.ModelName,lpMemScript->GetAsString());

			strcpy_s(info.ModelNameL,lpMemScript->GetAsString());

			strcpy_s(info.ModelNameR,lpMemScript->GetAsString());

			this->SetInfo(info);
		}
	}
	catch(...)
	{
		printf(lpMemScript->GetLastError());
		return 0;
	}

	delete lpMemScript;

	return 1;
}

void CCustomItemGlove::SetInfo(CUSTOM_ITEM_GLOVE_INFO info) // OK
{
	if(info.Index < 0 || info.Index >= MAX_CUSTOM_ITEM_GLOVE)
	{
		return;
	}

	this->m_CustomItemGloveInfo[info.Index] = info;
}
