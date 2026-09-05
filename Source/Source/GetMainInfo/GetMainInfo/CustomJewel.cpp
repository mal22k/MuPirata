// CustomJewel.cpp: implementation of the CCustomJewel class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomJewel.h"
#include "MemScript.h"

CCustomJewel gCustomJewel;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CCustomJewel::Load(char* path) // OK
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

	for(int n=0;n < MAX_CUSTOM_JEWEL;n++)
	{
		this->m_CustomJewelInfo[n].Index = -1;
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

			CUSTOM_JEWEL_INFO info;

			memset(&info,0,sizeof(info));

			static int CustomJewelIndexCount = 0;

			info.Index = CustomJewelIndexCount++;

			info.ItemIndex = GET_ITEM(lpMemScript->GetNumber(),lpMemScript->GetAsNumber());

			info.EnableSlotWeapon = lpMemScript->GetAsNumber();

			info.EnableSlotArmor = lpMemScript->GetAsNumber();

			info.EnableSlotWing = lpMemScript->GetAsNumber();

			info.SalePrice = lpMemScript->GetAsNumber();

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

void CCustomJewel::SetInfo(CUSTOM_JEWEL_INFO info) // OK
{
	if(info.Index < 0 || info.Index >= MAX_CUSTOM_JEWEL)
	{
		return;
	}

	this->m_CustomJewelInfo[info.Index] = info;
}
