// Protect.cpp: implementation of the CProtect class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Protect.h"
#include "CCRC32.H"
#include "Util.h"

CProtect gProtect;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProtect::CProtect() // OK
{

}

CProtect::~CProtect() // OK
{

}

bool CProtect::ReadMainFile(char* name) // OK
{
	CCRC32 CRC32;

	if(CRC32.FileCRC(name,&this->m_ClientFileCRC,1024) == 0)
	{
		return 0;
	}

	HANDLE file = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,0);

	if(file == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	if(GetFileSize(file,0) != sizeof(MAIN_FILE_INFO))
	{
		CloseHandle(file);
		return 0;
	}

	DWORD OutSize = 0;

	if(ReadFile(file,&this->m_MainInfo,sizeof(MAIN_FILE_INFO),&OutSize,0) == 0)
	{
		CloseHandle(file);
		return 0;
	}

	for(int n=0;n<sizeof(MAIN_FILE_INFO);n++)
	{
		((BYTE*)&this->m_MainInfo)[n] -= (BYTE)(0x30^HIBYTE(n));
		((BYTE*)&this->m_MainInfo)[n] += (BYTE)(0x20^HIBYTE(n));
		((BYTE*)&this->m_MainInfo)[n] ^= (BYTE)(0x10^LOBYTE(n));
	}

	CloseHandle(file);
	return 1;
}

void CProtect::CheckLauncher() // OK
{
	if((this->m_MainInfo.LauncherStart & 1) == 0)
	{
		return;
	}

	if(OpenMutex(MUTEX_ALL_ACCESS,0,this->m_MainInfo.LauncherMutex) == 0)
	{
		ExitProcess(0);
	}
}

void CProtect::CheckInstance() // OK
{
	if((this->m_MainInfo.LauncherStart & 2) == 0)
	{
		return;
	}

	char buff[256];

	wsprintf(buff,"MAIN_S1KOR_%s",this->m_MainInfo.IpAddress);

	if(OpenMutex(MUTEX_ALL_ACCESS,0,buff) == 0)
	{
		CreateMutex(0,0,buff);
	}
	else
	{
		ExitProcess(0);
	}
}

void CProtect::CheckClientFile() // OK
{
	if(this->m_MainInfo.ClientNameCRC == 0)
	{
		return;
	}

	char name[MAX_PATH] = {0};

	if(GetModuleFileName(0,name,sizeof(name)) == 0)
	{
		ExitProcess(0);
	}

	if(_stricmp(ConvertModuleFileName(name),this->m_MainInfo.ClientName) != 0)
	{
		ExitProcess(0);
	}

	CCRC32 CRC32;

	DWORD ClientCRC32;

	if(CRC32.FileCRC(this->m_MainInfo.ClientName,&ClientCRC32,1024) == 0)
	{
		ExitProcess(0);
	}

	if(this->m_MainInfo.ClientNameCRC != ClientCRC32)
	{
		ExitProcess(0);
	}
}

void CProtect::CheckPluginFile() // OK
{
	for(int i=0;i < 3;i++)
	{
		if(this->m_MainInfo.PluginNameCRC[i] == 0)
		{
			continue;
		}

		CCRC32 CRC32;

		DWORD CameraCRC32;

		if(CRC32.FileCRC(this->m_MainInfo.PluginName[i],&CameraCRC32,1024) == 0)
		{
			ExitProcess(0);
		}

		if(this->m_MainInfo.PluginNameCRC[i] != CameraCRC32)
		{
			ExitProcess(0);
		}

		HMODULE module = LoadLibrary(this->m_MainInfo.PluginName[i]);

		if(module == 0)
		{
			ExitProcess(0);
		}

		void (*EntryProc)() = (void(*)())GetProcAddress(module,"EntryProc");

		if(EntryProc != 0)
		{
			EntryProc();
		}
	}
}