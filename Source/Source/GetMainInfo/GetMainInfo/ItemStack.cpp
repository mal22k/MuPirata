// ItemStack.cpp: implementation of the CItemStack class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ItemStack.h"
#include "MemScript.h"

CItemStack gItemStack;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CItemStack::Load(char* path) // OK
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

	for(int n=0;n < MAX_ITEM_STACK_INFO;n++)
	{
		this->m_ItemStackInfo[n].Index = -1;
	}

	try
	{
		while(true)
		{
			if(lpMemScript->GetToken() == TOKEN_END)
			{
				break;
			}

			int section = lpMemScript->GetNumber();

			while(true)
			{
				static int ItemStackCount = 0;

				if(section == 0)
				{
					if(strcmp("end",lpMemScript->GetAsString()) == 0)
					{
						break;
					}

					ITEM_STACK_INFO info;

					memset(&info,0,sizeof(info));

					info.Index = ItemStackCount++;

					info.ItemIndex = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

					info.Level = lpMemScript->GetAsNumber();

					info.MaxStack = lpMemScript->GetAsNumber();

					info.CreateItemIndex = -1;

					this->SetInfo(info);
				}
				else if(section == 1)
				{
					if(strcmp("end",lpMemScript->GetAsString()) == 0)
					{
						break;
					}

					ITEM_STACK_INFO info;

					memset(&info,0,sizeof(info));

					info.Index = ItemStackCount++;

					info.Index = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

					info.Level = -1;

					info.MaxStack = lpMemScript->GetAsNumber();

					info.CreateItemIndex = GET_ITEM(lpMemScript->GetAsNumber(),lpMemScript->GetAsNumber());

					this->SetInfo(info);
				}
				else
				{
					break;
				}
			}
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

void CItemStack::SetInfo(ITEM_STACK_INFO info) // OK
{
	if(info.Index < 0 || info.Index >= MAX_ITEM_STACK_INFO)
	{
		return;
	}

	this->m_ItemStackInfo[info.Index] = info;
}