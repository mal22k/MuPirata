#include "stdafx.h"
#include "Font.h"
#include "Offset.h"
#include "Util.h"

int FontSize;

DWORD FontCharSet;

char FontName[100];

int ReadFontFile(char* path) // OK
{	
	FontSize = GetPrivateProfileInt("FontInfo","FontSize",13,path);
	
	FontCharSet = GetPrivateProfileInt("FontInfo","FontCharSet",1,path);
	
	GetPrivateProfileString("FontInfo","FontName","Tahoma",FontName,sizeof(FontName),path);

	return GetPrivateProfileInt("FontInfo","ChangeFontSwitch",0,path);
}

void InitFont() // OK
{
	if(ReadFontFile(".\\Config.ini") != 0)
	{
		SetCompleteHook(0xE8,0x004830C7,&FontNormal);
		
		SetCompleteHook(0xE8,0x00483108,&FontBold);
		
		SetCompleteHook(0xE8,0x0048314B,&FontBig);

		SetByte(0x004830CC,0x90);
		
		SetByte(0x0048310D,0x90);
		
		SetByte(0x00483150,0x90);
	}
}

void ReloadFont() // OK
{
	pFontNormal = FontNormal();

	pFontBold = FontBold();

	pFontBig = FontBig();

	*(DWORD*)MAIN_FONT_SIZE = FontSize;

	((void(*)())0x005AE400)();
}

HFONT FontNormal() // OK
{
	return CreateFont(FontSize,0,0,0,FW_NORMAL,0,0,0,FontCharSet,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,FontName);
}

HFONT FontBold() // OK
{
	return CreateFont(FontSize,0,0,0,FW_BOLD,0,0,0,FontCharSet,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,FontName);
}

HFONT FontBig() // OK
{
	return CreateFont(FontSize*2,0,0,0,FW_BOLD,0,0,0,FontCharSet,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,FontName);
}