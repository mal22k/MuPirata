// ItemValue.cpp: implementation of the CItemValue class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ItemValue.h"
#include "MemScript.h"

CItemValue gItemValue;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CItemValue::Load(char* path) // OK
{
	CMemScript* lpMemScript = new CMemScript;

	if(lpMemScript == 0)
	{
		printf(MEM_SCRIPT_ALLOC_ERROR,path);
		return 0;
	}

	if(lpMemScript->SetBuffer(path) == 0)
	{
		printf(lpMemScript->GetLastError());
		delete lpMemScript;
		return 0;
	}

	for(int n=0;n < MAX_ITEM_VALUE_INFO;n++)
	{
		this->m_ItemValueInfo[n].Index = -1;
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

			ITEM_VALUE_INFO info;

			memset(&info,0,sizeof(info));

			static int ItemValueCount = 0;

			info.Index = ItemValueCount++;

			info.ItemIndex = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

			info.Level = lpMemScript->GetAsNumber();

			info.Grade = lpMemScript->GetAsNumber();

			info.Value = lpMemScript->GetAsNumber();

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

void CItemValue::SetInfo(ITEM_VALUE_INFO info) // OK
{
	if(info.Index < 0 || info.Index >= MAX_ITEM_VALUE_INFO)
	{
		return;
	}

	this->m_ItemValueInfo[info.Index] = info;
}