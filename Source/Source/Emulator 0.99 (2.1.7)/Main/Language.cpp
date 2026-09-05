// CLanguage.cpp: implementation of the CLanguage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Language.h"
#include "Util.h"

char lang[4],filename[10][MAX_PATH];

void InitLanguage() // OK
{
	HKEY key;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Webzen\\Mu\\Config",0,KEY_ALL_ACCESS,&key) == ERROR_SUCCESS)
	{
		DWORD type=REG_SZ,size=sizeof(lang);

		if(RegQueryValueEx(key,"LangSelection",0,&type,(BYTE*)lang,&size) != ERROR_SUCCESS)
		{
			strcpy_s(lang,"Eng");
		}

		RegCloseKey(key);
	}

	if(_stricmp(lang,"Eng") != 0 && _stricmp(lang,"Por") != 0 && _stricmp(lang,"Spn") != 0)
	{
		strcpy_s(lang,"Eng");
	}

	wsprintf(filename[0],"Data\\Local\\%s\\Dialog_%s.bmd",lang,lang);
	wsprintf(filename[1],"Data\\Local\\%s\\Item_%s.bmd",lang,lang);
	wsprintf(filename[2],"Data\\Local\\%s\\ItemSetOption_%s.bmd",lang,lang);
	wsprintf(filename[3],"Data\\Local\\%s\\Movereq_%s.bmd",lang,lang);
	wsprintf(filename[4],"Data\\Local\\%s\\NpcName_%s.txt",lang,lang);
	wsprintf(filename[5],"Data\\Local\\%s\\Quest_%s.bmd",lang,lang);
	wsprintf(filename[6],"Data\\Local\\%s\\Skill_%s.bmd",lang,lang);
	wsprintf(filename[7],"Data\\Local\\%s\\Slide_%s.bmd",lang,lang);
	wsprintf(filename[8],"Data\\Local\\%s\\Text_%s.bmd",lang,lang);

	SetDword(0x005B06D1,(DWORD)&filename[0]);
	SetDword(0x005B05B7,(DWORD)&filename[1]);
	SetDword(0x005B069F,(DWORD)&filename[1]);
	SetDword(0x00407B28,(DWORD)&filename[2]);
	SetDword(0x005B06F1,(DWORD)&filename[3]);
	SetDword(0x005B07AA,(DWORD)&filename[3]);
	SetDword(0x005B0824,(DWORD)&filename[4]);
	SetDword(0x005B06E4,(DWORD)&filename[5]);
	SetDword(0x005B05E7,(DWORD)&filename[6]);
	SetDword(0x005B06A9,(DWORD)&filename[6]);
	SetDword(0x005B0844,(DWORD)&filename[7]);
	SetDword(0x005B0979,(DWORD)&filename[8]);
	SetDword(0x005B0997,(DWORD)&filename[8]);	
}