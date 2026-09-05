#include "stdafx.h"
#include "Common.h"
#include "Offset.h"
#include "Protect.h"
#include "Protocol.h"
#include "Util.h"

int CustomAttack;
char WindowName[64];

void InitCommon() // OK
{
	SetCompleteHook(0xE8,0x005C7C3C,&CalcFPS);

	SetCompleteHook(0xE9,0x005C8A4E,&CheckTickCount);

	SetCompleteHook(0xE9,0x00524665,&SkillIndex1);

	SetCompleteHook(0xE9,0x0052C83A,&SkillIndex2);

	SetCompleteHook(0xE9,0x0052D59D,&SkillIndex3);

	SetCompleteHook(0xE9,0x0052F413,&SkillIndex4);

	SetCompleteHook(0xE9,0x00530F2B,&SkillIndex5);
}

void CalcFPS() // OK
{
	((void(*)())0x004AA910)();

	if(*(DWORD*)(MAIN_SCREEN_STATE) == 5)
	{
		if(*(BYTE*)0x81B0F78)
		{
			*(BYTE*)(*(DWORD*)(MAIN_VIEWPORT_STRUCT)+864) = 0;

			pSetPlayerStop(*(DWORD*)(MAIN_VIEWPORT_STRUCT));
		}

		if(WindowName[0] == 0)
		{
			SetWindowText(*(HWND*)(MAIN_WINDOW),gProtect.m_MainInfo.WindowName);
		}
		else
		{
			SetWindowText(*(HWND*)(MAIN_WINDOW),WindowName);
		}

		if(*(BYTE*)(pMouseRButton))
		{
			if(CustomAttack != 0)
			{
				CustomAttack = 0;

				PBMSG_HEAD pMsg;

				pMsg.set(0x04,sizeof(pMsg));

				DataSend((BYTE*)&pMsg,pMsg.size);
			}
		}
	}
	else
	{
		CustomAttack = 0;
		WindowName[0] = 0;
		SetWindowText(*(HWND*)(MAIN_WINDOW),gProtect.m_MainInfo.WindowName);
	}
}

void __declspec(naked) CheckTickCount() // OK
{
	static DWORD CheckTickCountAddress1 = 0x005C8A54;

	_asm
	{
		Push 0x01
		Call Dword Ptr Ds:[Sleep]
		Call Dword Ptr Ds:[GetTickCount]
		Jmp[CheckTickCountAddress1]
	}
}

void __declspec(naked) SkillIndex1() // OK
{
	static DWORD SkillIndexAddress1 = 0x0052466A;

	_asm
	{
		Cmp Byte Ptr Ds:[pMouseLButton],0x00
		Je EXIT
		Mov Byte Ptr Ds:[pMouseRButton],0x00
		Jmp EXIT
		EXIT:
		Mov Eax,Dword Ptr Ss:[MAIN_VIEWPORT_STRUCT]
		Jmp[SkillIndexAddress1]
	}
}

void __declspec(naked) SkillIndex2() // OK
{
	static DWORD SkillIndexAddress1 = 0x0052C843;

	_asm
	{
		Cmp Byte Ptr Ds:[pMouseLButton],0x00
		Je EXIT
		Mov Byte Ptr Ds:[pMouseRButton],0x00
		Jmp EXIT
		EXIT:
		Mov Eax,Dword Ptr Ss:[Ebp+0x08]
		Cmp Word Ptr Ds:[Eax+0x02],0x1B1
		Jmp[SkillIndexAddress1]
	}
}

void __declspec(naked) SkillIndex3() // OK
{
	static DWORD SkillIndexAddress1 = 0x0052D5A6;

	_asm
	{
		Cmp Byte Ptr Ds:[pMouseLButton],0x00
		Je EXIT
		Mov Byte Ptr Ds:[pMouseRButton],0x00
		Jmp EXIT
		EXIT:
		Mov Ecx,Dword Ptr Ss:[Ebp+0x08]
		Cmp Word Ptr Ds:[Ecx+0x02],0x1B1
		Jmp[SkillIndexAddress1]
	}
}

void __declspec(naked) SkillIndex4() // OK
{
	static DWORD SkillIndexAddress1 = 0x0052F419;

	_asm
	{
		Cmp Byte Ptr Ds:[pMouseLButton],0x00
		Je EXIT
		Mov Byte Ptr Ds:[pMouseRButton],0x00
		Jmp EXIT
		EXIT:
		Cmp Word Ptr Ds:[Edi+0x02],0x1B1
		Jmp[SkillIndexAddress1]
	}
}

void __declspec(naked) SkillIndex5() // OK
{
	static DWORD SkillIndexAddress1 = 0x00530F31;

	_asm
	{
		Cmp Byte Ptr Ds:[pMouseLButton],0x00
		Je EXIT
		Mov Byte Ptr Ds:[pMouseRButton],0x00
		Jmp EXIT
		EXIT:
		Cmp Word Ptr Ds:[Edi+0x02],0x1B1
		Jmp[SkillIndexAddress1]
	}
}