#include "stdafx.h"
#include "resource.h"
#include "Main.h"
#include "Camera.h"
#include "CCRC32.H"
#include "ChaosBox.h"
#include "Common.h"
#include "EventEntryLevel.h"
#include "Font.h"
#include "HackCheck.h"
#include "HealthBar.h"
#include "Item.h"
#include "Language.h"
#include "Map.h"
#include "Monster.h"
#include "Offset.h"
#include "PacketManager.h"
#include "PrintPlayer.h"
#include "Protect.h"
#include "Protocol.h"
#include "Reconnect.h"
#include "Resolution.h"
#include "ServerList.h"
#include "Shop.h"
#include "TrayMode.h"
#include "Util.h"
#include "Fog.h"

HHOOK HookKB,HookMS;
HINSTANCE hins;
static DWORD ReloadScript = 0;

LRESULT CALLBACK KeyboardProc(int nCode,WPARAM wParam,LPARAM lParam) // OK
{
	if(nCode == HC_ACTION)
	{
		if(((DWORD)lParam & (1 << 30)) != 0 && ((DWORD)lParam & (1 << 31)) != 0 && GetForegroundWindow() == *(HWND*)(MAIN_WINDOW))
		{
			if(gProtect.m_MainInfo.KeyCodeHealthBarSwitch != 0 && wParam == gProtect.m_MainInfo.KeyCodeHealthBarSwitch)
			{
				HealthBarToggle();
			}
			else if(gProtect.m_MainInfo.KeyCodeCamera3DSwitch != 0 && wParam == gProtect.m_MainInfo.KeyCodeCamera3DSwitch)
			{
				gCamera.Toggle();
			}
			else if(gProtect.m_MainInfo.KeyCodeCamera3DRestore != 0 && wParam == gProtect.m_MainInfo.KeyCodeCamera3DRestore)
			{
				gCamera.Restore();
			}
			else if(gProtect.m_MainInfo.KeyCodeTrayModeSwitch != 0 && wParam == gProtect.m_MainInfo.KeyCodeTrayModeSwitch)
			{
				gTrayMode.Toggle();
			}
		}
	}

	return CallNextHookEx(HookKB,nCode,wParam,lParam);
}

LRESULT CALLBACK MouseProc(int nCode,WPARAM wParam,LPARAM lParam) // OK
{
	if(nCode == HC_ACTION)
	{
		MOUSEHOOKSTRUCTEX* HookStruct =(MOUSEHOOKSTRUCTEX*)lParam;

		if(GetForegroundWindow() == *(HWND*)(MAIN_WINDOW))
		{
			switch(wParam)
			{
				case WM_MOUSEMOVE:
					gCamera.Move(HookStruct);
					break;
				case WM_MBUTTONDOWN:
					gCamera.SetIsMove(1);
					gCamera.SetCursorX(HookStruct->pt.x);
					gCamera.SetCursorY(HookStruct->pt.y);
					break;
				case WM_MBUTTONUP:
					gCamera.SetIsMove(0);
					break;
				case WM_MOUSEWHEEL:
					gCamera.Zoom(HookStruct);
					break;
			}
		}
	}

	return CallNextHookEx(HookMS,nCode,wParam,lParam);
}

SHORT WINAPI KeysProc(int nCode) // OK
{
	if(GetForegroundWindow() != *(HWND*)(MAIN_WINDOW))
	{
		return 0;
	}

	return GetAsyncKeyState(nCode);
}

HICON WINAPI IconProc(HINSTANCE hInstance,LPCSTR lpIconName) // OK
{
	FILE* file;

	if(fopen_s(&file,".\\main.ico","r") != 0)
	{
		gTrayMode.m_TrayIcon = (HICON)LoadImage(hins,MAKEINTRESOURCE(IDI_CLIENT),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR);
	}
	else
	{
		fclose(file);
		gTrayMode.m_TrayIcon = (HICON)LoadImage(hins,".\\main.ico",IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_LOADFROMFILE | LR_DEFAULTCOLOR);
	}

	return gTrayMode.m_TrayIcon;
}

void WINAPI ReduceConsumeProc() // OK
{
	while(true)
	{
		Sleep(5000);
		SetProcessWorkingSetSize(GetCurrentProcess(),0xFFFFFFFF,0xFFFFFFFF);
		SetThreadPriority(GetCurrentProcess(),THREAD_PRIORITY_LOWEST);
	}
}

extern "C" _declspec(dllexport) void EntryProc() // OK
{
	if(gProtect.ReadMainFile("ServerInfo.sse") != 0)
	{
		SetByte(0x00605EE4,0xA0); // Accent
		SetByte(0x00482C29,0xEB); // Crack (mu.exe)
		SetByte(0x00482CA2,0xEB); // Multi Instance
		SetByte(0x004830A6,0x03); // Fix Font Blurry
		SetByte(0x004830E7,0x03); // Fix Font Blurry
		SetByte(0x00483128,0x03); // Fix Font Blurry
		SetByte(0x004CDD8B,0x0D); // Fix Effect +13
		SetByte(0x00523F8E,0xFF); // Fix Dark Knight Skills
		SetByte(0x00523F93,0xFF); // Fix Dark Knight Skills
		SetByte(0x006069EC,(gProtect.m_MainInfo.ClientVersion[0]+1)); // Version
		SetByte(0x006069ED,(gProtect.m_MainInfo.ClientVersion[2]+2)); // Version
		SetByte(0x006069EE,(gProtect.m_MainInfo.ClientVersion[3]+3)); // Version
		SetByte(0x006069EF,(gProtect.m_MainInfo.ClientVersion[5]+4)); // Version
		SetByte(0x006069F0,(gProtect.m_MainInfo.ClientVersion[6]+5)); // Version
		SetWord(0x0061065C,(gProtect.m_MainInfo.IpAddressPort)); // IpAddressPort
		SetDword(0x00481B93,(DWORD)gProtect.m_MainInfo.WindowName);
		SetDword(0x005C86AB,(DWORD)gProtect.m_MainInfo.ScreenShotPath);
		SetDword(0x005FA368,(DWORD)&KeysProc);
		SetDword(0x005FA3F0,(DWORD)&IconProc);

		MemorySet(0x00414E30,0x90,0x1D); // Remove MuError.log

		MemorySet(0x0047A64C,0x90,0x05); // Website

		MemorySet(0x0047A661,0x90,0x05); // Website

		MemorySet(0x00498045,0x90,0x06); // Fix Move Cursor

		MemorySet(0x004B2D90,0x90,0x1E); // Remove Reflect Effect

		MemorySet(0x005613C0,0x90,0x02); // Item Move Inventory -> Interface

		MemorySet(0x00561369,0x90,0x02); // Item Move Interface -> Inventory

		MemoryCpy(0x00605FA6,gProtect.m_MainInfo.IpAddress,sizeof(gProtect.m_MainInfo.IpAddress)); // IpAddress

		MemoryCpy(0x006069F4,gProtect.m_MainInfo.ClientSerial,sizeof(gProtect.m_MainInfo.ClientSerial)); // ClientSerial

		SetCompleteHook(0xE8,0x005443AF,&DrawNewHealthBar);

		SetCompleteHook(0xFF,0x004A3B8B,&ProtocolCoreEx);

		gCustomEffect.Load(gProtect.m_MainInfo.CustomEffectInfo);

		gCustomFog.Load(gProtect.m_MainInfo.CustomFogInfo);

		gCustomItem.Load(gProtect.m_MainInfo.CustomItemInfo);

		gCustomMap.Load(gProtect.m_MainInfo.CustomMapInfo);

		gCustomMonster.Load(gProtect.m_MainInfo.CustomMonsterInfo);

		gCustomMonsterSkin.Load(gProtect.m_MainInfo.CustomMonsterSkinInfo);

		gCustomTooltip.Load(gProtect.m_MainInfo.CustomTooltipInfo);

		gItemStack.Load(gProtect.m_MainInfo.ItemStackInfo);

		gItemValue.Load(gProtect.m_MainInfo.ItemValueInfo);

		gPacketManager.LoadEncryptionKey("Data\\Enc1.dat");

		gPacketManager.LoadDecryptionKey("Data\\Dec2.dat");

		InitChaosBox();

		InitCommon();

		InitEventEntryLevel();

		InitFog();

		InitFont();

		InitHackCheck();

		InitItem();

		InitLanguage();

		InitMap();

		InitMonster();

		InitPrintPlayer();

		InitResolution();

		InitReconnect();

		InitServerList();

		InitShop();

		gProtect.CheckLauncher();

		gProtect.CheckInstance();

		gProtect.CheckClientFile();

		gProtect.CheckPluginFile();

		HookKB = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,hins,GetCurrentThreadId());

		HookMS = SetWindowsHookEx(WH_MOUSE,MouseProc,hins,GetCurrentThreadId());

		CreateThread(0,0,(LPTHREAD_START_ROUTINE)ReduceConsumeProc,0,0,0);
	}
	else
	{
		ErrorMessageBox("Could not load ServerInfo.sse!");
		ExitProcess(0);
	}
}

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) // OK
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hins = (HINSTANCE)hModule;
			break;
	}

	return 1;
}