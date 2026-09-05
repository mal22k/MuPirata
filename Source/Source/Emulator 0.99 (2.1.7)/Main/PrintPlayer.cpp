#include "stdafx.h"
#include "PrintPlayer.h"
#include "Offset.h"
#include "Protect.h"
#include "Util.h"

DWORD ViewIndex = 0;
DWORD ViewLevel = 0;
DWORD ViewReset = 0;
DWORD ViewValue = 0;
DWORD ViewPoint = 0;
DWORD ViewCurHP = 0;
DWORD ViewMaxHP = 0;
DWORD ViewCurMP = 0;
DWORD ViewMaxMP = 0;
DWORD ViewCurBP = 0;
DWORD ViewMaxBP = 0;
DWORD ViewDamageHP = 0;
DWORD ViewExperience = 0;
DWORD ViewNextExperience = 0;
DWORD ViewStrength = 0;
DWORD ViewDexterity = 0;
DWORD ViewVitality = 0;
DWORD ViewEnergy = 0;
DWORD ViewLeadership = 0;
DWORD ViewAddStrength = 0;
DWORD ViewAddDexterity = 0;
DWORD ViewAddVitality = 0;
DWORD ViewAddEnergy = 0;
DWORD ViewAddLeadership = 0;
DWORD ViewPhysiSpeed = 0;
DWORD ViewMagicSpeed = 0;

void InitPrintPlayer() // OK
{
	SetCompleteHook(0xE8,0x00545DDC,&PrintDrawCircleHPMP);

	SetCompleteHook(0xE8,0x00545EA1,&PrintDrawCircleHPMP);

	SetCompleteHook(0xE8,0x0054542E,&PrintDrawCircleBP);

	SetCompleteHook(0xE8,0x00548123,&PrintBarExperience1);

	SetCompleteHook(0xE8,0x00548158,&PrintBarExperience2);

	SetCompleteHook(0xE8,0x0040AFB4,&PrintPlayerViewHP1);

	SetCompleteHook(0xE8,0x00545E35,&PrintPlayerViewHP1);

	SetCompleteHook(0xE8,0x005876B6,&PrintPlayerViewHP1);

	SetCompleteHook(0xE8,0x00546A38,&PrintPlayerViewHP2);

	SetCompleteHook(0xE8,0x00545EFA,&PrintPlayerViewMP1);

	SetCompleteHook(0xE8,0x005877D0,&PrintPlayerViewMP1);

	SetCompleteHook(0xE8,0x00546A5F,&PrintPlayerViewMP2);

	SetCompleteHook(0xE8,0x00545490,&PrintPlayerViewBP1);

	SetCompleteHook(0xE8,0x00545448,&PrintPlayerViewBP2);

	SetCompleteHook(0xE8,0x00587B46,&PrintPlayerViewLevelUp);

	SetCompleteHook(0xE8,0x005866E2,&PrintPlayerViewStrength);

	SetCompleteHook(0xE8,0x0058720C,&PrintPlayerViewDexterity);

	SetCompleteHook(0xE8,0x00587633,&PrintPlayerViewVitality);

	SetCompleteHook(0xE8,0x00587752,&PrintPlayerViewEnergy);

	SetCompleteHook(0xE8,0x00587DC6,&PrintPlayerViewLeadership);

	SetCompleteHook(0xE8,0x0058746F,&PrintPlayerViewAttackSpeed);

	SetCompleteHook(0xE9,0x004FCAAC,&PrintPlayerSetAttackSpeed);

	SetCompleteHook(0xE8,0x00491233,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491AD7,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491C3A,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491C87,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491D0B,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491D58,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00491D75,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x0049496F,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00495274,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x004952A7,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00495D06,&PrintDamageOnScreenHP);

	SetCompleteHook(0xE8,0x00495D39,&PrintDamageOnScreenHP);
}

void PrintDamageOnScreenHP(DWORD a,DWORD b,float* c,DWORD d) // OK
{
	if(((int)b) > 0)
	{
		b = ViewDamageHP;
	}

	((void(*)(DWORD,DWORD,float*,DWORD))0x004F6080)(a,b,c,d);
}

void PrintDrawCircleHPMP(DWORD a,float b,float c,float d,float e,float f,float g,float h,float i,int j,int k,GLfloat l) // OK
{
	float HP = (float)(ViewMaxHP-ViewCurHP)/(float)ViewMaxHP;
	float MP = (float)(ViewMaxMP-ViewCurMP)/(float)ViewMaxMP;

	c = ((a==0xEB)?MP:HP)*48.0f+432.0f;
    e = 48.0f-((a==0xEB)?MP:HP)*48.0f;
    g = ((a==0xEB)?MP:HP)*48.0f/64.0f;
    i = (1.0f-((a==0xEB)?MP:HP))*48.0f/64.0f;

	return pDrawImage(a,b,c,d,e,f,g,h,i,j,k,l);
}

void PrintDrawCircleBP(DWORD a,float b,float c,float d,float e,float f,float g,float h,float i,int j,int k,GLfloat l) // OK
{
	float BP = (float)(ViewMaxBP-ViewCurBP)/(float)ViewMaxBP;

	c = BP*36.0f+438.0f;
    e = 36.0f-BP*36.0f;
	g = BP*36.0f/64.0f;
    i = (1.0f-BP)*36.0f/64.0f;

	return pDrawImage(a,b,c,d,e,f,g,h,i,j,k,l);
}

void PrintBarExperience1(float a,float b,float c,float d,float e,int f) // OK
{
	float TotalBarValue = 1782.0f*((float)(gLevelExperience[ViewLevel]-ViewExperience)/(float)(gLevelExperience[ViewLevel]-gLevelExperience[ViewLevel-1]));
	float BarSplit = TotalBarValue/198.0f;
	float Modf[2];

	Modf[0] = modf(BarSplit,&Modf[1]);

	pDrawBarForm(a,b,(Modf[0]==0)?0:(198.0f-(Modf[0]*198.0f)),d,e,f);
}

void PrintBarExperience2(float a,float b,DWORD c,float d,float e) // OK
{
	float TotalBarValue = 1782.0f*((float)(gLevelExperience[ViewLevel]-ViewExperience)/(float)(gLevelExperience[ViewLevel]-gLevelExperience[ViewLevel-1]));
	float BarSplit = TotalBarValue/198.0f;
	float Modf[2];

	Modf[0] = modf(BarSplit,&Modf[1]);

	pDrawBigText(a,b,((DWORD)(9-Modf[1])>9)?9:(DWORD)(9-Modf[1]),d,e);
}

void PrintPlayerViewHP1(char* a, char* b) // OK
{
	wsprintf(a,b,ViewCurHP,ViewMaxHP);
}

void PrintPlayerViewHP2(float a,float b,DWORD c,float d,float e) // OK
{
	PrintFixStatPoint();

	pDrawBigText(a+6,b,ViewCurHP,d,e);
}

void PrintPlayerViewMP1(char* a, char* b) // OK
{
	wsprintf(a,b,ViewCurMP,ViewMaxMP);
}

void PrintPlayerViewMP2(float a,float b,DWORD c,float d,float e) // OK
{
	pDrawBigText(a-20,b,ViewCurMP,d,e);
}

void PrintPlayerViewBP1(char* a, char* b) // OK
{
	wsprintf(a,b,ViewCurBP,ViewMaxBP);
}

void PrintPlayerViewBP2(float a,float b,DWORD c,float d,float e) // OK
{
	pDrawBigText(a,b,ViewCurBP,d,e);
}

void PrintPlayerViewLevelUp(char* a, char* b) // OK
{
	wsprintf(a,b,ViewPoint);
}

void PrintPlayerViewStrength(char* a, char* b) // OK
{
	wsprintf(a,b,ViewStrength+ViewAddStrength);
}

void PrintPlayerViewDexterity(char* a, char* b) // OK
{
	wsprintf(a,b,ViewDexterity+ViewAddDexterity);
}

void PrintPlayerViewVitality(char* a, char* b) // OK
{
	wsprintf(a,b,ViewVitality+ViewAddVitality);
}

void PrintPlayerViewEnergy(char* a, char* b) // OK
{
	wsprintf(a,b,ViewEnergy+ViewAddEnergy);
}

void PrintPlayerViewLeadership(char* a, char* b) // OK
{
	wsprintf(a,b,ViewLeadership+ViewAddLeadership);
}

void PrintPlayerViewAttackSpeed(char* a,char* b) // OK
{
	if(((*(BYTE*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x0B)) & 7) == 0)
	{
		wsprintf(a,b,ViewMagicSpeed);
	}
	else
	{
		wsprintf(a,b,ViewPhysiSpeed);
	}
}

void PrintFixStatPoint() // OK
{
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x18) = GET_MAX_WORD_VALUE(ViewStrength);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x2A) = GET_MAX_WORD_VALUE(ViewAddStrength);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1A) = GET_MAX_WORD_VALUE(ViewDexterity);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x2C) = GET_MAX_WORD_VALUE(ViewAddDexterity);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1C) = GET_MAX_WORD_VALUE(ViewVitality);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x2E) = GET_MAX_WORD_VALUE(ViewAddVitality);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x1E) = GET_MAX_WORD_VALUE(ViewEnergy);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x30) = GET_MAX_WORD_VALUE(ViewAddEnergy);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x20) = GET_MAX_WORD_VALUE(ViewLeadership);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x36) = GET_MAX_WORD_VALUE(ViewAddLeadership);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x0E) = GET_MAX_WORD_VALUE(ViewLevel);
	
	*(WORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x62) = GET_MAX_WORD_VALUE(ViewPoint);

	*(DWORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x10) = (ViewExperience > ViewNextExperience) ? ViewNextExperience : ViewExperience;
	
	*(DWORD*)(*(DWORD*)(MAIN_CHARACTER_STRUCT)+0x14) = ViewNextExperience;
}

__declspec(naked) void PrintPlayerSetAttackSpeed()
{
	static DWORD PrintPlayerSetAttackSpeedAddress1 = 0x004FCB6C;

	_asm
	{
		Mov Edx,ViewPhysiSpeed
		Mov Eax,Dword Ptr Ds:[gProtect.m_MainInfo.MaxAttackSpeed+Ecx*4]
		And Eax,0xFFFF
		Cmp Edx,Eax
		Jle NEXT1
		Mov Edx,Eax
		NEXT1:
		Lea Edi,[Esi+0x46]
		Mov Word Ptr Ds:[Edi],Dx
		Mov Edx,ViewMagicSpeed
		Mov Eax,Dword Ptr Ds:[gProtect.m_MainInfo.MaxAttackSpeed+Ecx*4]
		And Eax,0xFFFF
		Cmp Edx,Eax
		Jle NEXT2
		Mov Edx,Eax
		NEXT2:
		Lea Ebx,[Esi+0x52]
		Jmp[PrintPlayerSetAttackSpeedAddress1]
	}
}