#include "stdafx.h"
#include "Item.h"
#include "CustomEffect.h"
#include "CustomItem.h"
#include "CustomTooltip.h"
#include "Offset.h"
#include "Util.h"	

void InitItem() // OK
{
	SetCompleteHook(0xE8,0x005B056D,&ItemModelLoad);

	SetCompleteHook(0xE8,0x005B0572,&ItemTextureLoad);

	SetCompleteHook(0xFF,0x004CB0C6,&GetItemColor);

	SetCompleteHook(0xFF,0x005A0295,&GetItemColor);

	SetCompleteHook(0xE8,0x004C5B82,&GetItemEffect);

	SetCompleteHook(0xE8,0x005A2162,&GetItemEffect);

	SetCompleteHook(0xE9,0x00550E24,&GetItemToolTip);

	SetCompleteHook(0xE8,0x00561387,&MoveItem);
}

void ItemModelLoad() // OK
{
	((void(*)())0x005A4860)();

	for(int n=0;n<MAX_CUSTOM_ITEM;n++)
	{
		if(gCustomItem.m_CustomItemInfo[n].Index != -1)
		{
			LoadItemModel((gCustomItem.m_CustomItemInfo[n].ItemIndex+ITEM_BASE_MODEL),((gCustomItem.m_CustomItemInfo[n].ItemIndex >= GET_ITEM(7,0) && gCustomItem.m_CustomItemInfo[n].ItemIndex<GET_ITEM(12,0)) ? "Player\\" : "Item\\"),gCustomItem.m_CustomItemInfo[n].ModelName);
		}
	}
}

void ItemTextureLoad() // OK
{
	for(int n=0;n<MAX_CUSTOM_ITEM;n++)
	{
		if(gCustomItem.m_CustomItemInfo[n].Index != -1)
		{
			LoadItemTexture((gCustomItem.m_CustomItemInfo[n].ItemIndex+ITEM_BASE_MODEL),((gCustomItem.m_CustomItemInfo[n].ItemIndex >= GET_ITEM(7,0) && gCustomItem.m_CustomItemInfo[n].ItemIndex<GET_ITEM(12,0)) ? "Player\\" : "Item\\"));
		}
	}

	((void(*)())0x005A5F30)();
}

void LoadItemModel(int index,char* folder,char* name)
{
	if(name[0] == 0)
	{
		return;
	}

	char path[MAX_PATH]={ 0 };

	wsprintf(path,"Data\\%s",folder);

	pLoadItemModel(index,path,name,-1);
}

void LoadItemTexture(int index,char* folder)
{
	pLoadItemTexture(index,folder,GL_REPEAT,GL_NEAREST,GL_TRUE);
}

void GetItemColor(DWORD a,DWORD b,DWORD c,DWORD d,DWORD e) // OK
{
	if(gCustomItem.GetCustomItemColor((a-ITEM_BASE_MODEL),(float*)d) == 0)
	{
		((void(*)(DWORD,DWORD,DWORD,DWORD,DWORD))0x0059E730)(a,b,c,d,e);
	}
}

void GetItemEffect(DWORD a,int b,float* c,float d,int e,int f,int g,int h,int i) // OK
{
	for(int n=0;n < MAX_CUSTOM_EFFECT;n++)
	{
		if(gCustomEffect.m_CustomEffectInfo[n].Index != -1)
		{
			if(gCustomEffect.m_CustomEffectInfo[n].ItemIndex != (b-ITEM_BASE_MODEL))
			{
				continue;
			}

			if(gCustomEffect.m_CustomEffectInfo[n].MinItemLevel != -1 && gCustomEffect.m_CustomEffectInfo[n].MinItemLevel > GET_ITEM_OPT_LEVEL(e))
			{
				continue;
			}

			if(gCustomEffect.m_CustomEffectInfo[n].MaxItemLevel != -1 && gCustomEffect.m_CustomEffectInfo[n].MaxItemLevel < GET_ITEM_OPT_LEVEL(e))
			{
				continue;
			}

			if(gCustomEffect.m_CustomEffectInfo[n].MinNewOption != -1 && gCustomEffect.m_CustomEffectInfo[n].MinNewOption > GET_ITEM_OPT_EXC(f))
			{
				continue;
			}

			if(gCustomEffect.m_CustomEffectInfo[n].MaxNewOption != -1 && gCustomEffect.m_CustomEffectInfo[n].MaxNewOption < GET_ITEM_OPT_EXC(f))
			{
				continue;
			}

			DWORD o = 204 * b + *(DWORD*)0x0568E60C; // OK

			float ItemColor[3];
			
			float Position[3] = {0.0f,0.0f,0.0f};

			float WorldPosition[3] = {0.0f,0.0f,0.0f};

			ItemColor[0] = (float)(gCustomEffect.m_CustomEffectInfo[n].ColorR/255.0f);
			
			ItemColor[1] = (float)(gCustomEffect.m_CustomEffectInfo[n].ColorG/255.0f);
			
			ItemColor[2] = (float)(gCustomEffect.m_CustomEffectInfo[n].ColorB/255.0f);

			pTransformPosition(o,0x00689E7FC+(48*gCustomEffect.m_CustomEffectInfo[n].EffectValue),Position,WorldPosition,true);

			switch(gCustomEffect.m_CustomEffectInfo[n].EffectType)
			{
				case 0:
					pCreateSprite(gCustomEffect.m_CustomEffectInfo[n].EffectIndex,WorldPosition,gCustomEffect.m_CustomEffectInfo[n].Scale,ItemColor,1,0,gCustomEffect.m_CustomEffectInfo[n].EffectLevel);
					break;
				case 1:
					pCreateParticle(gCustomEffect.m_CustomEffectInfo[n].EffectIndex,WorldPosition,a+28,ItemColor,gCustomEffect.m_CustomEffectInfo[n].EffectLevel,gCustomEffect.m_CustomEffectInfo[n].Scale,1);
					break;
				case 2:
					pCreateEffect(gCustomEffect.m_CustomEffectInfo[n].EffectIndex,WorldPosition,a+28,ItemColor,gCustomEffect.m_CustomEffectInfo[n].EffectLevel,a,0,0,0,0,0);
					break;
			}
		}
	}

	pRenderPartObjectEffect(a,b,c,d,e,f,g,h,i);
}

void DrawItemToolTip(DWORD address) // OK
{
	for(int i=0;i<MAX_CUSTOM_TOOLTIP;i++)
	{
		if(gCustomTooltip.m_CustomTooltipInfo[i].Index != -1 && gCustomTooltip.m_CustomTooltipInfo[i].ItemIndex == *(WORD*)(address+0x00))
		{
			if (gCustomTooltip.m_CustomTooltipInfo[i].ItemLevel == -1 || gCustomTooltip.m_CustomTooltipInfo[i].ItemLevel == GET_ITEM_OPT_LEVEL(*(DWORD*)(address+0x04)))
			{
				*(&*(DWORD*)0x07D24D10 + *(DWORD*)0x07D3EB3C) = gCustomTooltip.m_CustomTooltipInfo[i].FontValue;
				*(&*(DWORD*)0x07D3C260 + *(DWORD*)0x07D3EB3C) = gCustomTooltip.m_CustomTooltipInfo[i].FontColor;

				wsprintf((char*)(0x64 * *(DWORD*)0x07D3EB3C + 0x07D23C20), gCustomTooltip.m_CustomTooltipInfo[i].Text);

				*(DWORD*)0x07D3EB3C += 1;
			}
		}
	}
}

bool MoveItem(DWORD a,DWORD b,int c,int d,int e) // OK
{
	((UINT(__thiscall*)(DWORD,DWORD)) 0x004066A0)(0x05687D00,0x07D3EADE);

	int trade_var = *(BYTE*)0x07D3EADE;

	((UINT(__thiscall*)(DWORD,DWORD)) 0x004061D0)(0x05687D00,0x07D3EADE);

	if(trade_var)
	{
		return ((bool(*)(DWORD,DWORD,int,int,int))0x00562390)(*(DWORD*)0x07D38A7C+0xF, *(DWORD*)0x07D38A78+0x10E,0x07D3C2D8,d,4);
	}
	else if(*(BYTE*)0x081B0F78 && *(DWORD*)0x081B0F7C != 1)
	{
		return ((bool(*)(DWORD,DWORD,int,int,int))0x00562390)(*(DWORD*)0x07D3EA30+0xF,*(DWORD*)0x07D3EA34+0x6E,0x081B0678,d,4);
	}
	else if(*(BYTE*)0x7D3EADD)
	{
		return ((bool(*)(DWORD,DWORD,int,int,int))0x00562390)(a,b,c,d,e);
	}
	
	return 0;
}

__declspec(naked) void GetItemToolTip() // OK
{
	static DWORD ItemApplyToolTipAddress1 = 0x00550E29;

	_asm
	{
		PushAd
		Push Esi
		Call [DrawItemToolTip]
		Add Esp,0x04
		PopAd
		Cmp Word Ptr Ss:[Esi],0x1CD
		Jmp [ItemApplyToolTipAddress1]
	}
}