#include "stdafx.h"
#include "HealthBar.h"
#include "Offset.h"
#include "PrintPlayer.h"
#include "Protect.h"
#include "Util.h"

NEW_HEALTH_BAR gNewHealthBar[MAX_MAIN_VIEWPORT];

bool MonsterHPBarMode = 0;

void HealthBarToggle() // OK
{
	if(*(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		MonsterHPBarMode ^= 1;
		pDrawMessage(pGetTextLine((808+MonsterHPBarMode)),1);
	}
}

void ClearNewHealthBar() // OK
{
	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		gNewHealthBar[n].index = 0xFFFF;
		gNewHealthBar[n].type = 0;
		gNewHealthBar[n].rate = 0;
	}
}

void InsertNewHealthBar(WORD index,BYTE type,BYTE rate) // OK
{
	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		if (gNewHealthBar[n].index == 0xFFFF)
		{
			gNewHealthBar[n].index = index;
			gNewHealthBar[n].type = type;
			gNewHealthBar[n].rate = rate;
			return;
		}
	}
}

NEW_HEALTH_BAR* GetNewHealthBar(WORD index,BYTE type) // OK
{
	for(int n=0;n < MAX_MAIN_VIEWPORT;n++)
	{
		if(gNewHealthBar[n].index != 0xFFFF)
		{
			if(gNewHealthBar[n].index == index && gNewHealthBar[n].type == type)
			{
				return &gNewHealthBar[n];
			}
		}
	}

	return 0;
}

void DrawNewHealthBar() // OK
{
	((void(*)())0x00544E40)(); // OK

	if(MonsterHPBarMode != 0)
	{
		return;
	}

	int PosX,PosY,LifeProgress;
	float LifeBarWidth = 38.0f;
	char LifeDisplay[20];
	float Angle[3];
	DWORD ViewportAddress = 0;

	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		if ((ViewportAddress = pViewportAddress+(n*0x40C)) == 0) // OK
		{
			continue;
		}

		if (*(BYTE*)(ViewportAddress) == 0)
		{
			continue;
		}

		NEW_HEALTH_BAR* lpNewHealthBar = GetNewHealthBar(*(WORD*)(ViewportAddress+0x1E4),*(BYTE*)(ViewportAddress+0x84));

		if (lpNewHealthBar == 0)
		{
			continue;
		}

		int LifePercent = lpNewHealthBar->rate / 10;

		Angle[0] = *(float*)(ViewportAddress+0x10);

		Angle[1] = *(float*)(ViewportAddress+0x14);

		Angle[2] = *(float*)(ViewportAddress+0x12C) + *(float*)(ViewportAddress+0x18) + 100.0f;

		pGetPosFromAngle(Angle,&PosX,&PosY);

		PosX -= (int)floor(LifeBarWidth / (double)2.0);

		if ((pCursorX >= PosX) && ((float)pCursorX <= (float)PosX + LifeBarWidth) && (pCursorY >= PosY - 2) && (pCursorY < PosY + 6))
		{
			wsprintf(LifeDisplay,"HP : %d0%%",LifePercent);
			pDrawText((PosX+4),(PosY-8),LifeDisplay,0,0,0);
		}

		EnableAlphaTest(true);

		glColor4f(0.0,0.0,0.0,0.5);
		pDrawBarForm((float)(PosX + 1),(float)(PosY + 1),LifeBarWidth + 4.0f,5.0f,0.0f,0);
		EnableAlphaBlend();

		glColor3f(0.2f,0.0,0.0);
		pDrawBarForm((float)PosX,(float)PosY,LifeBarWidth + 4.0f,5.0f,0.0,0);

		glColor3f(0.19607843f,0.039215688f,0.0);
		pDrawBarForm((float)(PosX + 2),(float)(PosY + 2),LifeBarWidth,1.0f,0.0,0);

		if (LifePercent > 10)
		{
			LifeProgress = 10;
		}
		else
		{
			LifeProgress = LifePercent;
		}

		glColor3f(0.98039216f,0.039215688f,0.0);

		for (int i = 0; i < LifeProgress; i++)
		{
			pDrawBarForm((float)(i * 4 + PosX + 2),(float)(PosY + 2),4.0,2.0,0.0,0);
		}

		DisableAlphaBlend();
	}

	DisableAlphaBlend();

	glColor3f(1.0,1.0,1.0);
}