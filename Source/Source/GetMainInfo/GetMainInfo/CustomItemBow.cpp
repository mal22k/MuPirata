// CustomItemBow.cpp: implementation of the CCustomItemBow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomItemBow.h"
#include "MemScript.h"

CCustomItemBow gCustomItemBow;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CCustomItemBow::Load(char* path) // OK
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

	for(int n=0;n<MAX_CUSTOM_ITEM_BOW;n++)
	{
		this->m_CustomItemBowInfo[n].Index = -1;
	}

	try
	{
		while(true)
		{
			if(lpMemScript->GetToken() == TOKEN_END)
			{
				break;
			}

			if(strcmp("end", lpMemScript->GetString()) == 0)
			{
				break;
			}

			CUSTOM_ITEM_BOW_INFO info;

			memset(&info,0,sizeof(info));

			static int CustomItemBowCount = 0;

			info.Index = CustomItemBowCount++;

			info.ItemIndex = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

			info.Number = lpMemScript->GetAsNumber();

			info.Effect = lpMemScript->GetAsNumber();

			info.ColorR = lpMemScript->GetAsNumber();

			info.ColorG = lpMemScript->GetAsNumber();

			info.ColorB = lpMemScript->GetAsNumber();

			strcpy_s(info.ModelName,lpMemScript->GetAsString());

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

void CCustomItemBow::SetInfo(CUSTOM_ITEM_BOW_INFO info) // OK
{
	if(info.Index < 0 || info.Index >= MAX_CUSTOM_ITEM_BOW)
	{
		return;
	}

	this->m_CustomItemBowInfo[info.Index] = info;
}
