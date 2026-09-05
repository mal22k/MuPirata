// ============================================================================
// LuaPlugin.cpp
// DLL plugin para el cliente MU Online EX603 (SSeMU Season 6).
//
// METODO DE INYECCION (flujo GetMainInfo):
//   1) main.exe parcheado carga main.dll y llama su EntryProc()
//   2) main.dll lee ServerInfo.sse y ejecuta gProtect.CheckPluginFile()
//   3) CheckPluginFile hace LoadLibrary("Lua.dll") y llama a EntryProc()
//      -> este codigo corre DENTRO del proceso del cliente, en el hilo principal,
//         con los hooks base de main.dll YA instalados.
//
// Registro: MainInfo.ini -> [MainInfo] PluginName1=Lua.dll
//           + GetMainInfo.exe -> genera ServerInfo.sse (con CRC de la DLL)
//
// PENDIENTE DE VERIFICAR: las direcciones de Offset.h son del EX603 (Season 6
// Episode 3). Si tu cliente difiere, ajustalas antes de probar.
// ============================================================================

#include "stdafx.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "LuaPlugin.h"
#include "Offset.h"
#include "PacketManager.h"
#include "Util.h"
#include "MoneyCache.h"
#include "ClientStateCache.h"
#include "MouseBlock.h"

// Funcion de envio del cliente (puntero que main.dll sustituye por MySend)
typedef int(WINAPI*WSSEND)(SOCKET,char*,int,int);

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------
HINSTANCE g_PluginInstance = 0;
lua_State* g_Lua = 0;
bool g_LuaReady = false;
bool g_LuaScriptError = false;
bool g_LuaRenderError = false;
bool g_LuaPacketError = false;

// Funcion que se llamaba antes del hook (barras de vida de main.dll o la original)
DWORD g_OriginalRenderFunction = RENDER_ORIGINAL;

// Dispatcher de packets previo (ProtocolCoreEx de main.dll o el original)
BOOL(*g_OriginalPacketHandler)(BYTE,BYTE*,int,int) = 0;

// ---------------------------------------------------------------------------
// Input polling (on_key / on_click): se sondean en el hook de render, por eso
// no dependen del orden de instalacion de los hooks de teclado/mouse de main.dll
// (que se instalan DESPUES de cargar los plugins).
// ---------------------------------------------------------------------------
static BYTE g_SubscribedKeys[32];
static int g_SubscribedKeyCount = 0;
static BYTE g_PrevKeyState[256];
static bool g_LuaInputError = false;
static int g_ConsecutiveInputErrors = 0; // solo desactiva el input tras 5 fallos seguidos

// ---------------------------------------------------------------------------
// Router de opcodes: RegisterPacketHandler(head, sub|nil, fn)
// Los handlers se guardan como referencias en el registro de Lua (luaL_ref) y
// se consultan ANTES del on_packet global. sub=-1 = wildcard (cualquier sub).
// ---------------------------------------------------------------------------
struct PACKET_HANDLER
{
	BYTE head;
	int sub;
	int luaRef;
};

static PACKET_HANDLER g_PacketHandlers[32];
static int g_PacketHandlerCount = 0;

// ---------------------------------------------------------------------------
// Log a archivo (Lua\lua_plugin.log, junto al cliente)
// ---------------------------------------------------------------------------
void LuaLog(const char* fmt, ...)
{
	char buff[512] = {0};

	va_list arg;
	va_start(arg,fmt);
	vsprintf_s(buff,sizeof(buff),fmt,arg);
	va_end(arg);

	FILE* file = 0;

	fopen_s(&file,LUA_LOG_PATH,"a");

	if(file != 0)
	{
		fprintf(file,"%s\n",buff);
		fclose(file);
	}
}

// ---------------------------------------------------------------------------
// Lectura SEGURA de la memoria del cliente (MAIN_CHARACTER_STRUCT y derivados):
// si un offset apunta a memoria invalida (sin personaje cargado, build distinto),
// se devuelve 0 en vez de crashear el cliente.
// ---------------------------------------------------------------------------
static bool SafeIsReadable(DWORD addr,DWORD size)
{
	if(addr == 0)
	{
		return false;
	}

	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery((LPCVOID)addr,&mbi,sizeof(mbi)) == 0)
	{
		return false;
	}

	if(mbi.State != MEM_COMMIT)
	{
		return false;
	}

	if(mbi.Protect == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD) == PAGE_GUARD)
	{
		return false;
	}

	return true;
}

static BYTE  SafeReadByte(DWORD addr)  { return SafeIsReadable(addr,1) ? *(BYTE*)addr  : 0; }
static WORD  SafeReadWord(DWORD addr)  { return SafeIsReadable(addr,2) ? *(WORD*)addr  : 0; }
static DWORD SafeReadDword(DWORD addr) { return SafeIsReadable(addr,4) ? *(DWORD*)addr : 0; }

// Puntero al struct del personaje (0 si no hay personaje cargado)
static DWORD CharacterBase()
{
	if(SafeIsReadable(MAIN_CHARACTER_STRUCT,4) == false)
	{
		return 0;
	}

	return *(DWORD*)MAIN_CHARACTER_STRUCT;
}

// Cache del dinero del personaje: NO vive en MAIN_CHARACTER_STRUCT (el struct
// del cliente no guarda el dinero); se parsea de los packets del servidor
// (C3:F3:03 y C3:22 result=0xFE) en LuaProtocolCoreEx.
static DWORD g_MoneyCache = 0;

// Cache de estados del cliente (trade, personal shop, caja del caos): se
// actualiza en LuaProtocolCoreEx con el MISMO parser que usa el test harness
// (ClientStateCache.h). El NPC dialog y los slots de inventario NO estan
// documentados en el source viejo -> se resuelven desde Lua (calibracion de
// ventanas) o quedan NO CONFIRMADO (ver reporte).
static CLIENT_STATE g_ClientState;

// Log que solo escribe la PRIMERA vez (para funciones NO CONFIRMADAS: evita
// spamear el log por cada llamada)
static void LuaLogOnce(bool& flag,const char* fmt,...)
{
	if(flag)
	{
		return;
	}

	flag = true;

	char buff[512] = {0};

	va_list arg;
	va_start(arg,fmt);
	vsprintf_s(buff,sizeof(buff),fmt,arg);
	va_end(arg);

	LuaLog("%s",buff);
}

// Declaracion anticipada (definida mas abajo, seccion de hooks de codigo)
static bool IsExecutableCode(DWORD addr);

// ---------------------------------------------------------------------------
// BLOQUEO DE CLICKS SOBRE UI CUSTOM (FASE 6)
//
// Enfoque: hook del CALL SITE que invoca MouseClick (0x007D2920), el mismo
// patron que ya usan los hooks de render y packets de este plugin (y el mismo
// del Main_EX603: SetCompleteHook(0xE8, 0x007D2B0C, &HelperMouseClick)).
// NO se hace jmp-hook de la funcion (evita los riesgos de trampoline y de
// partir una instruccion multi-byte del prologo): solo se redirige un call
// E8/E9 ya existente.
//   - Localizacion del call site: primero el conocido del source viejo
//     (0x007D2B0C) si sigue siendo E8/E9; si no, escaneo del .text del modulo
//     principal buscando E8/E9 -> 0x007D2920 (una sola vez al arrancar).
//   - Si main.dll (u otro) ya hookeo ese call, se guarda su destino y se
//     ENCADENA (SetCompleteHook(0xFF,...) conserva el opcode E8/E9 y solo
//     escribe el rel32; identico a InstallPacketHook).
//   - Si no hay call site, la feature queda DESACTIVADA y logueada (fallback
//     Lua: on_click sigue detectando la UI, el juego recibe el click).
//   - El bloqueo es OPT-IN: sin rectangulos (UI.BlockMouse) o con
//     UI.SetMouseBlockEnabled(false), el hook llama directo al original
//     (cero cambio de comportamiento).
// ---------------------------------------------------------------------------
#define MOUSE_CLICK_CALLSITE_REF 0x007D2B0C   // call site conocido del Main_EX603

static MOUSE_BLOCK_STATE g_MouseBlock;
static DWORD g_MouseClickCallSite = 0;        // call que invoca MouseClick
static DWORD g_OriginalMouseClick = 0;        // destino original del call
static bool g_MouseClickOriginalIsThisCall = false;
static bool g_MouseClickHooked = false;

// --- Diagnostico por capa de bloqueo (UI.BlockedBy*) ---
static int g_MouseHookBlockedCount = 0;   // capa 1: hook del call site de MouseClick
static int g_WndProcBlockedCount = 0;     // capa 2: WndProc traga el mensaje de raton
static int g_SendBlockedCount = 0;        // capa 3: el packet C1:04/05/06 no sale a la red

// --- Capa 2: WndProc del cliente (patron TrayMode del Main_EX603) ---
// Se instala LAZY desde DrawLuaUI (primer frame de render: ahi la ventana del
// cliente ya existe). Encadena con CallWindowProc al wndproc previo (el de
// main.dll si lo hubiera subclaseado antes -> no se rompe su cadena).
// Se subclasean TODAS las ventanas top-level visibles del proceso (normalmente
// solo una, la del juego): si el mouse va a parar a cualquiera de ellas, el
// mensaje pasa por el guard.
#define MAX_GUARD_WINDOWS 8

static WNDPROC g_OriginalWndProcs[MAX_GUARD_WINDOWS];
static HWND g_GuardWindows[MAX_GUARD_WINDOWS];
static int g_GuardWindowCount = 0;
static bool g_WndProcInstalled = false;
static bool g_WndProcLogOnce = false;

// --- Capa 3: guard de envio sobre CLIENT_SEND_POINTER ---
// Intercepta TODOS los envios del cliente en el mismo punto donde main.dll
// pone su MySend (cifrado de stream). Si el cursor esta sobre un rect de UI
// custom y el packet es C1:04 (ataque) / C1:05 (mover) / C1:06 (cancelar), lo
// traga y devuelve exito simulado: el juego no nota nada y el personaje NO se
// mueve ni ataca aunque el click haya llegado al procesamiento del juego.
static WSSEND g_OriginalClientSend = 0;
static bool g_SendGuardInstalled = false;
static bool g_SendGuardLogOnce = false; // log del re-assert solo la primera vez

// el send crudo de ws2_32? (si el puntero aun apunta ahi, main.dll no ha
// instalado su MySend y NO se debe tocar: el guard veria buffers cifrados y
// ademas se perderia el cifrado de stream)
static bool IsRawWs2Send(WSSEND fn)
{
	HMODULE ws2 = GetModuleHandleA("ws2_32.dll");

	if(ws2 == 0)
	{
		return false;
	}

	DWORD base = (DWORD)ws2;

	return ((DWORD)fn >= base && (DWORD)fn < base + 0x20000);
}

// Deberia bloquear el click actual? (logica pura del estado compartido
// MouseBlock.h; solo lee el cursor del cliente)
static bool MouseClickShouldBlock()
{
	return g_MouseBlock.ShouldBlock(pCursorX,pCursorY);
}

// Misma firma que HelperMouseClick del Main_EX603: char MouseClickHook(char* This).
// Se llama desde el call site parcheado con los mismos argumentos que la
// funcion original. Devuelve 0 (consumido) cuando el click cae sobre un rect
// de UI custom; si no, delega en el original (thiscall) o en el hook previo
// (cdecl, p.ej. el de main.dll).
static char MouseClickHook(char* This)
{
	if(MouseClickShouldBlock())
	{
		g_MouseBlock.consumed = true;
		g_MouseHookBlockedCount++;

		return 0; // click consumido (0 = no procesado, igual que en el source viejo)
	}

	if(g_MouseClickOriginalIsThisCall)
	{
		// La funcion original del cliente se llama como thiscall (patron del
		// Main_EX603: ((char(__thiscall*)(char*))0x007D2920)(This)).
		return ((char(__thiscall*)(char*))g_OriginalMouseClick)(This);
	}

	// Destino encadenado (hook previo, p.ej. main.dll): convencion cdecl
	return ((char(*)(char*))g_OriginalMouseClick)(This);
}

// Busca el call site que invoca MouseClick (0x007D2920). Primero el conocido
// del source viejo; si no, escaneo del .text del modulo principal.
static DWORD FindMouseClickCallSite()
{
	// 1) call site documentado (0x007D2B0C): debe seguir siendo un call/jmp E8/E9
	if(IsExecutableCode(MOUSE_CLICK_CALLSITE_REF))
	{
		BYTE b = *(BYTE*)MOUSE_CLICK_CALLSITE_REF;

		if(b == 0xE8 || b == 0xE9)
		{
			DWORD target = MOUSE_CLICK_CALLSITE_REF + 5 + (*(DWORD*)(MOUSE_CLICK_CALLSITE_REF+1));

			// call directo a MouseClick o ya hookeado (destino ejecutable)
			if(target == MOUSE_CLICK_OFFSET || IsExecutableCode(target))
			{
				return MOUSE_CLICK_CALLSITE_REF;
			}
		}
	}

	// 2) escaneo del .text del modulo principal: E8/E9 -> MOUSE_CLICK_OFFSET
	HMODULE mod = GetModuleHandle(0);

	if(mod == 0)
	{
		return 0;
	}

	__try
	{
		PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)mod;

		if(dos->e_magic != IMAGE_DOS_SIGNATURE)
		{
			return 0;
		}

		PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)mod + dos->e_lfanew);

		if(nt->Signature != IMAGE_NT_SIGNATURE)
		{
			return 0;
		}

		DWORD numSec = nt->FileHeader.NumberOfSections;
		PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);

		for(DWORD s=0;s<numSec;s++)
		{
			DWORD va = (DWORD)mod + sec[s].VirtualAddress;
			DWORD size = sec[s].Misc.VirtualSize;

			if(size == 0)
			{
				continue;
			}

			MEMORY_BASIC_INFORMATION mbi;

			if(VirtualQuery((LPCVOID)va,&mbi,sizeof(mbi)) == 0)
			{
				continue;
			}

			switch(mbi.Protect & 0xFF)
			{
				case PAGE_EXECUTE:
				case PAGE_EXECUTE_READ:
				case PAGE_EXECUTE_READWRITE:
				case PAGE_EXECUTE_WRITECOPY:
					break;
				default:
					continue;
			}

			// no escanear mas alla de lo que esta realmente mapeado
			if(mbi.RegionSize < size)
			{
				size = (DWORD)mbi.RegionSize;
			}

			for(DWORD i=0;i+5<=size;i++)
			{
				BYTE b = *(BYTE*)(va+i);

				if(b == 0xE8 || b == 0xE9)
				{
					DWORD target = (va+i) + 5 + (*(DWORD*)(va+i+1));

					if(target == MOUSE_CLICK_OFFSET)
					{
						LuaLog("[LuaPlugin] MouseClick: call site encontrado por escaneo en 0x%08X",va+i);
						return va+i;
					}
				}
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}

	return 0;
}

static void InstallMouseClickHook()
{
	DWORD callSite = FindMouseClickCallSite();

	if(callSite == 0)
	{
		LuaLog("[LuaPlugin] MouseClick: call site de 0x%08X no encontrado, bloqueo de clicks desactivado (fallback Lua: on_click detecta la UI)",MOUSE_CLICK_OFFSET);
		return;
	}

	g_MouseClickCallSite = callSite;

	BYTE opcode = *(BYTE*)callSite;

	// Destino actual del call: si ya lo hookeo main.dll, se encadena
	if(opcode == 0xE8 || opcode == 0xE9)
	{
		DWORD target = callSite + 5 + (*(DWORD*)(callSite+1));

		if(target == MOUSE_CLICK_OFFSET)
		{
			// call directo a la funcion original: thiscall (patron del source viejo)
			g_OriginalMouseClick = MOUSE_CLICK_OFFSET;
			g_MouseClickOriginalIsThisCall = true;

			LuaLog("[LuaPlugin] MouseClick: call site 0x%08X -> original 0x%08X (thiscall)",callSite,MOUSE_CLICK_OFFSET);
		}
		else if(IsExecutableCode(target))
		{
			// ya hookeado (p.ej. main.dll): encadenar con su convencion
			g_OriginalMouseClick = target;
			g_MouseClickOriginalIsThisCall = false;

			LuaLog("[LuaPlugin] MouseClick: call site 0x%08X encadenado a 0x%08X",callSite,target);
		}
		else
		{
			g_OriginalMouseClick = MOUSE_CLICK_OFFSET;
			g_MouseClickOriginalIsThisCall = true;

			LuaLog("[LuaPlugin] MouseClick: destino 0x%08X no ejecutable, original=0x%08X",target,MOUSE_CLICK_OFFSET);
		}
	}
	else
	{
		LuaLog("[LuaPlugin] MouseClick: call site 0x%08X con opcode 0x%02X inesperado, bloqueo de clicks desactivado",callSite,opcode);
		return;
	}

	// Redirigir el call: 0xFF conserva el opcode E8/E9 y escribe solo el rel32
	SetCompleteHook(0xFF,callSite,&MouseClickHook);

	g_MouseClickHooked = true;

	LuaLog("[LuaPlugin] MouseClick: hook instalado en el call site 0x%08X",callSite);
}

// ---------------------------------------------------------------------------
// CAPA 2 - WndProc del cliente (patron TrayMode del Main_EX603)
//
// Los mensajes de raton (WM_LBUTTONDOWN/UP, RBUTTON, MBUTTON, DBLCLK) con el
// cursor sobre un rect de UI custom se tragan ANTES de que el juego los vea:
// el juego ni siquiera procesa el click (no camina, no ataca, no cierra una
// ventana nativa que quede detras de la UI custom). El input de Lua NO depende
// del WndProc (se sondea con GetAsyncKeyState en el hook de render), asi que
// los botones siguen respondiendo.
//
// Instalacion LAZY desde DrawLuaUI: en el primer frame de render la ventana
// del cliente ya existe (IsWindow) y main.dll ya instalo sus hooks -> el
// wndproc guardado es el de main.dll (o el del cliente) y se ENCADENA.
// ---------------------------------------------------------------------------
static LRESULT CALLBACK LuaWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	// Coordenadas de los mensajes de raton: client area, en lParam. La lista de
	// rects se registra en el MISMO espacio (pCursorX/pCursorY = client area del
	// cliente, igual que Draw.Bar/on_click): si en un build el cursor global
	// no fuera client area, esta capa 2 dejaria de coincidir con las capas 1/3
	// (bloqueo reducido, sin crash; se ve en el panel SYS con "wnd NO" o
	// contadores sin incrementar).
	int mx = (int)(short)LOWORD(lParam);
	int my = (int)(short)HIWORD(lParam);

	if(ShouldSwallowClickMessage(g_MouseBlock,msg,mx,my))
	{
		g_WndProcBlockedCount++;
		return 0; // tragado: el juego no ve el mensaje
	}

	// buscar el wndproc original de ESTA ventana (cada una tiene el suyo)
	WNDPROC orig = (g_GuardWindowCount > 0) ? g_OriginalWndProcs[0] : 0;

	for(int n=0;n<g_GuardWindowCount;n++)
	{
		if(g_GuardWindows[n] == hWnd)
		{
			orig = g_OriginalWndProcs[n];
			break;
		}
	}

	return CallWindowProc(orig,hWnd,msg,wParam,lParam);
}

// Busca las ventanas del juego por PROCESO (EnumWindows): no depende del offset
// MAIN_WINDOW (que en otro build podria no apuntar al HWND real). Recoge TODAS
// las ventanas top-level visibles de este proceso (normalmente solo una).
struct WNDPROC_SCAN
{
	HWND windows[MAX_GUARD_WINDOWS];
	int count;
};

static BOOL CALLBACK LuaWndProcScan(HWND hwnd,LPARAM lParam)
{
	DWORD pid = 0;

	GetWindowThreadProcessId(hwnd,&pid);

	if(pid != GetCurrentProcessId() || IsWindowVisible(hwnd) == 0)
	{
		return TRUE; // seguir buscando
	}

	WNDPROC_SCAN* scan = (WNDPROC_SCAN*)lParam;

	if(scan->count < MAX_GUARD_WINDOWS)
	{
		scan->windows[scan->count++] = hwnd;
	}

	return TRUE;
}

static void InstallWndProcGuard()
{
	if(g_WndProcInstalled)
	{
		return;
	}

	// 1) ventanas del juego por proceso (robusto: funciona aunque MAIN_WINDOW
	//    no sea valido en este build)
	WNDPROC_SCAN scan;

	scan.count = 0;
	memset(scan.windows,0,sizeof(scan.windows));

	EnumWindows(LuaWndProcScan,(LPARAM)&scan);

	// 2) fallback: offset del source viejo (solo si no se encontro ninguna)
	if(scan.count == 0)
	{
		HWND hwnd = *(HWND*)MAIN_WINDOW;

		if(hwnd != 0 && IsWindow(hwnd) != 0)
		{
			scan.windows[scan.count++] = hwnd;
		}
	}

	if(scan.count == 0)
	{
		// la ventana aun no existe (arranque): reintentar el proximo frame
		if(g_WndProcLogOnce == 0)
		{
			LuaLog("[LuaPlugin] WndProc guard: esperando ventana del cliente (EnumWindows + MAIN_WINDOW sin resultado)");
			g_WndProcLogOnce = true;
		}
		return;
	}

	// 3) subclasear cada ventana (encadenando con CallWindowProc al wndproc
	//    previo de cada una)
	for(int n=0;n<scan.count;n++)
	{
		HWND hwnd = scan.windows[n];

		if(g_GuardWindowCount >= MAX_GUARD_WINDOWS)
		{
			break;
		}

		LONG_PTR prev = GetWindowLongPtrW(hwnd,GWLP_WNDPROC);

		if(prev == 0)
		{
			continue;
		}

		// Asignar el original ANTES de instalar el nuestro: si SetWindowLongPtr
		// llegara a despachar algun mensaje de forma reentrante, LuaWndProc ya
		// tiene a quien delegar (nunca CallWindowProc(0,...)).
		g_OriginalWndProcs[g_GuardWindowCount] = (WNDPROC)prev;

		LONG_PTR result = SetWindowLongPtrW(hwnd,GWLP_WNDPROC,(LONG_PTR)LuaWndProc);

		if(result == 0 && GetLastError() != 0)
		{
			continue; // esta ventana fallo: probar la siguiente
		}

		g_GuardWindows[g_GuardWindowCount] = hwnd;
		g_GuardWindowCount++;
	}

	if(g_GuardWindowCount > 0)
	{
		g_WndProcInstalled = true;

		LuaLog("[LuaPlugin] WndProc guard instalado en %d ventana(s) (primera 0x%08X)",g_GuardWindowCount,(DWORD)g_GuardWindows[0]);
	}
	else
	{
		LuaLog("[LuaPlugin] WndProc guard: no se pudo subclasear ninguna ventana, capa 2 desactivada");
	}
}

// ---------------------------------------------------------------------------
// CAPA 3 - Guard de envio sobre CLIENT_SEND_POINTER
//
// Todos los packets que el cliente envia pasan por el puntero de envio
// (main.dll lo sustituye por MySend). Si el cursor esta sobre un rect de UI
// custom y el packet es C1:04 (ataque) / C1:05 (mover) / C1:06 (cancelar
// movimiento), el packet se TRAGA y se devuelve exito simulado: el juego cree
// que lo envio y el personaje no se mueve ni ataca, SIN importar como proceso
// el click el juego (WndProc, polling, etc.).
//
// Solo se instala si el puntero actual es codigo ejecutable y NO es el send
// crudo de ws2_32 (significaria que main.dll aun no lo hookeo -> no tocar).
// El guard solo actua cuando buff[0]==0xC1 y op en {04,05,06}: si el buffer
// ya viniera cifrado (otro layout), no bloquea nada -> cero riesgo.
// ---------------------------------------------------------------------------
static int WINAPI LuaSendGuard(SOCKET s,char* buff,int size,int flags)
{
	if(ShouldBlockPacket(g_MouseBlock,pCursorX,pCursorY,(const BYTE*)buff,size))
	{
		g_SendBlockedCount++;
		return size; // exito simulado: el packet no sale, el juego no lo nota
	}

	// NOTA: el SendPacket del propio plugin tambien pasa por este guard (lee el
	// MISMO CLIENT_SEND_POINTER). Si un script enviara deliberadamente C1:05/04/06
	// con el cursor sobre un rect de UI, se tragaria igual: es el comportamiento
	// deseado del bloqueo, pero ojo al escribir scripts que automaticen
	// movimiento/ataque mientras el cursor este sobre una ventana custom.

	return g_OriginalClientSend(s,buff,size,flags);
}

// Escribe el guard sobre el puntero actual (valida antes: ejecutable y NO
// ws2_32). Usado por la instalacion inicial y por el mantenimiento por frame.
static void WriteSendGuard(WSSEND cur)
{
	if(cur == LuaSendGuard)
	{
		return; // ya instalado
	}

	if(cur == 0 || IsExecutableCode((DWORD)cur) == false || IsRawWs2Send(cur))
	{
		return; // aun sin MySend (o invalido): se reintenta el proximo frame
	}

	g_OriginalClientSend = cur;

	// CLIENT_SEND_POINTER vive en .data del cliente: escribir el guard
	DWORD oldProtect = 0;

	VirtualProtect((LPVOID)CLIENT_SEND_POINTER,4,PAGE_READWRITE,&oldProtect);

	*(WSSEND*)CLIENT_SEND_POINTER = LuaSendGuard;

	VirtualProtect((LPVOID)CLIENT_SEND_POINTER,4,oldProtect,&oldProtect);

	g_SendGuardInstalled = true;

	if(g_SendGuardLogOnce == false)
	{
		g_SendGuardLogOnce = true;
		LuaLog("[LuaPlugin] SendGuard activo: puntero 0x%08X -> LuaSendGuard (original 0x%08X)",CLIENT_SEND_POINTER,(DWORD)cur);
	}
}

static void InstallSendGuard()
{
	WriteSendGuard(*(WSSEND*)CLIENT_SEND_POINTER);

	if(g_SendGuardInstalled == false)
	{
		LuaLog("[LuaPlugin] SendGuard: el puntero 0x%08X aun no es MySend (ws2_32 u otro), se reintenta por frame",(DWORD)*(WSSEND*)CLIENT_SEND_POINTER);
	}
}

// Mantenimiento por frame (se llama desde DrawLuaUI). Dos casos que la
// instalacion inicial no cubre:
//   a) Instalacion tardia: main.dll puede escribir MySend en CLIENT_SEND_POINTER
//      DESPUES de cargar los plugins -> aqui se instala en cuanto existe.
//   b) Re-escritura: si otro modulo reemplaza el puntero despues de instalar
//      (dejando el guard saltado y los packets de movimiento saliendo -> el
//      personaje se movia al clickear la UI), aqui se re-instala encadenando
//      al valor nuevo.
static void MaintainSendGuard()
{
	WriteSendGuard(*(WSSEND*)CLIENT_SEND_POINTER);
}

// ---------------------------------------------------------------------------
// API Lua: Draw
// ---------------------------------------------------------------------------
static int LuaDrawText(lua_State* L) // Draw.Text(x, y, texto, r, g, b)
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	const char* text = luaL_checkstring(L,3);
	BYTE r = (BYTE)luaL_optinteger(L,4,255);
	BYTE g = (BYTE)luaL_optinteger(L,5,255);
	BYTE b = (BYTE)luaL_optinteger(L,6,255);

	pSetTextColor(pTextThis(),0xFF,r,g,b);
	pDrawText(pTextThis(),x,y,(char*)text,0,0,(LPINT)1,0);

	return 0;
}

static int LuaDrawBar(lua_State* L) // Draw.Bar(x, y, w, h, r, g, b, a)
{
	float x = (float)luaL_checknumber(L,1);
	float y = (float)luaL_checknumber(L,2);
	float w = (float)luaL_checknumber(L,3);
	float h = (float)luaL_checknumber(L,4);
	float r = (float)luaL_optnumber(L,5,255.0)/255.0f;
	float g = (float)luaL_optnumber(L,6,255.0)/255.0f;
	float b = (float)luaL_optnumber(L,7,255.0)/255.0f;
	float a = (float)luaL_optnumber(L,8,255.0)/255.0f;

	pSetBlend(true);
	glColor4f(r,g,b,a);
	pDrawBarForm(x,y,w,h,0.0f,0);
	pGLSwitchBlend();

	return 0;
}

static int LuaDrawMessage(lua_State* L) // Draw.Message(texto, tipo)
{
	const char* text = luaL_checkstring(L,1);
	int type = (int)luaL_optinteger(L,2,1);

	pDrawMessage(text,type);

	return 0;
}

// ---------------------------------------------------------------------------
// API Lua: Input
// ---------------------------------------------------------------------------
static int LuaCursorX(lua_State* L) // Input.CursorX()
{
	lua_pushinteger(L,pCursorX);
	return 1;
}

static int LuaCursorY(lua_State* L) // Input.CursorY()
{
	lua_pushinteger(L,pCursorY);
	return 1;
}

// ---------------------------------------------------------------------------
// API Lua: Client
// ---------------------------------------------------------------------------
static int LuaScreenState(lua_State* L) // Client.ScreenState()
{
	lua_pushinteger(L,*(DWORD*)MAIN_SCREEN_STATE);
	return 1;
}

static int LuaInGame(lua_State* L) // Client.InGame()
{
	lua_pushboolean(L,(*(DWORD*)MAIN_SCREEN_STATE == 5) ? 1 : 0);
	return 1;
}

static int LuaResolutionX(lua_State* L) // Client.ResolutionX()
{
	lua_pushinteger(L,*(DWORD*)MAIN_RESOLUTION_X);
	return 1;
}

static int LuaResolutionY(lua_State* L) // Client.ResolutionY()
{
	lua_pushinteger(L,*(DWORD*)MAIN_RESOLUTION_Y);
	return 1;
}

static int LuaLogMsg(lua_State* L) // Log(mensaje)
{
	const char* text = luaL_checkstring(L,1);

	LuaLog("%s",text);

	return 0;
}

// ---------------------------------------------------------------------------
// API Lua: Client.CharacterName() (nombre del personaje, struct en +0x00)
// ---------------------------------------------------------------------------
static int LuaCharacterName(lua_State* L) // Client.CharacterName()
{
	DWORD base = CharacterBase();

	if(base == 0)
	{
		lua_pushstring(L,"");
		return 1;
	}

	lua_pushstring(L,(char*)base);

	return 1;
}

// ---------------------------------------------------------------------------
// API Lua: Client - datos del personaje (struct EX603 + cache de dinero)
//   MAIN_CHARACTER_STRUCT (0x08128AC8) -> puntero al struct; offsets del source
//   viejo Main_EX603 (Protocol.cpp / PrintPlayer.cpp):
//     +0x00 nombre, +0x0B clase, +0x0E level, +0x10 exp, +0x14 next exp,
//     +0x18 str, +0x1A dex, +0x1C vit, +0x1E ene, +0x20 lider, +0x22 life,
//     +0x24 mana, +0x26 maxLife, +0x28 maxMana, +0x2A shield, +0x2C maxShield,
//     +0x40 BP, +0x42 maxBP, +0x74 levelUpPoint
// ---------------------------------------------------------------------------
static int LuaCharacterLevel(lua_State* L)          { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x0E : 0)); return 1; }
static int LuaCharacterClass(lua_State* L)          { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadByte(b ? b+0x0B : 0)); return 1; }
static int LuaCharacterHP(lua_State* L)             { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x22 : 0)); return 1; }
static int LuaCharacterMaxHP(lua_State* L)          { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x26 : 0)); return 1; }
static int LuaCharacterMP(lua_State* L)             { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x24 : 0)); return 1; }
static int LuaCharacterMaxMP(lua_State* L)          { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x28 : 0)); return 1; }
static int LuaCharacterShield(lua_State* L)         { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x2A : 0)); return 1; }
static int LuaCharacterMaxShield(lua_State* L)      { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x2C : 0)); return 1; }
static int LuaCharacterBP(lua_State* L)             { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x40 : 0)); return 1; }
static int LuaCharacterMaxBP(lua_State* L)          { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x42 : 0)); return 1; }
static int LuaCharacterStrength(lua_State* L)       { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x18 : 0)); return 1; }
static int LuaCharacterDexterity(lua_State* L)      { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x1A : 0)); return 1; }
static int LuaCharacterVitality(lua_State* L)       { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x1C : 0)); return 1; }
static int LuaCharacterEnergy(lua_State* L)         { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x1E : 0)); return 1; }
static int LuaCharacterLeadership(lua_State* L)     { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x20 : 0)); return 1; }
static int LuaCharacterLevelUpPoint(lua_State* L)   { DWORD b = CharacterBase(); lua_pushinteger(L,SafeReadWord(b ? b+0x74 : 0)); return 1; }
// NOTA (v0.3.5): el exp se guarda como DWORD SIN SIGNO. lua_pushinteger en
// Win32 es un entero con signo de 32 bits -> exp > 2.147.483.647 se desbordaba
// a negativo (detectado en vivo por el auto-diagnostico DIAG, FASE 11). Se
// empuja como numero (double) para conservar el valor unsigned completo.
static int LuaCharacterExperience(lua_State* L)     { DWORD b = CharacterBase(); lua_pushnumber(L,(double)SafeReadDword(b ? b+0x10 : 0)); return 1; }
static int LuaCharacterNextExperience(lua_State* L) { DWORD b = CharacterBase(); lua_pushnumber(L,(double)SafeReadDword(b ? b+0x14 : 0)); return 1; }
static int LuaCharacterMoney(lua_State* L)          { lua_pushinteger(L,g_MoneyCache); return 1; }
static int LuaCharacterMap(lua_State* L)            { lua_pushinteger(L,SafeReadDword(MAIN_CURRENT_MAP)); return 1; }

// ---------------------------------------------------------------------------
// API Lua: Client - estados cacheados por packets (trade / personal shop /
// caja del caos). Ver ClientStateCache.h: los layouts vienen del GameServer
// del pack (Trade.h, PersonalShop.h, ChaosBox.cpp) y estan documentados en
// REPORTE_CLIENTE_EX603.txt. Lo que NO se puede confirmar devuelve nil/false
// con un log UNICO (no spamea) y queda documentado en REPORTE_FUNCIONES_CLIENTE.txt.
// ---------------------------------------------------------------------------
static int LuaClientIsTradeOpen(lua_State* L)       { lua_pushboolean(L,g_ClientState.tradeOpen ? 1 : 0); return 1; }
static int LuaClientIsTradeAccepted(lua_State* L)   { lua_pushboolean(L,g_ClientState.tradeAccepted ? 1 : 0); return 1; }
static int LuaClientGetTradeMoney(lua_State* L)     { lua_pushinteger(L,g_ClientState.tradeMoney); return 1; }

static int LuaClientGetTradeItem(lua_State* L) // Client.GetTradeItem(slot) -> {slot,index,level,dur} | nil
{
	int slot = (int)luaL_checkinteger(L,1);

	if(slot < 0 || slot >= TRADE_MAX_ITEMS || g_ClientState.tradeItems[slot].index <= 0)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	lua_pushinteger(L,g_ClientState.tradeItems[slot].slot);  lua_setfield(L,-2,"slot");
	lua_pushinteger(L,g_ClientState.tradeItems[slot].index); lua_setfield(L,-2,"index");
	lua_pushinteger(L,g_ClientState.tradeItems[slot].level); lua_setfield(L,-2,"level");
	lua_pushinteger(L,g_ClientState.tradeItems[slot].dur);   lua_setfield(L,-2,"dur");

	return 1;
}

static int LuaClientIsShopOpen(lua_State* L)        { lua_pushboolean(L,g_ClientState.shopOpen ? 1 : 0); return 1; }

static int LuaClientIsChaosBoxOpen(lua_State* L) // evidencia: se recibio C1:88 (el jugador uso la caja del caos)
{
	lua_pushboolean(L,g_ClientState.chaosBoxSeen ? 1 : 0);
	return 1;
}

// --- NO CONFIRMADOS: devuelven nil/false y loguean UNA vez ---
static int LuaClientIsNpcDialogOpen(lua_State* L) // Client.IsNpcDialogOpen()
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.IsNpcDialogOpen: NO CONFIRMADO (C1:30/C1:31 los envia el CLIENTE, no se ven por el hook de recepcion; aproximar con Interface.IsOpen del window ID calibrado desde Lua)");

	lua_pushboolean(L,0);
	return 1;
}

static int LuaClientGetNpcIndex(lua_State* L) // Client.GetNpcIndex()
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetNpcIndex: NO CONFIRMADO (el indice del NPC no llega por packets de recepcion)");

	lua_pushinteger(L,-1);
	return 1;
}

static int LuaClientGetNpcName(lua_State* L) // Client.GetNpcName()
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetNpcName: NO CONFIRMADO (el nombre del NPC no llega por packets de recepcion)");

	lua_pushstring(L,"");
	return 1;
}

static int LuaClientGetInventoryItem(lua_State* L) // Client.GetInventoryItem(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetInventoryItem: NO CONFIRMADO (el layout de los slots de inventario del personaje no esta documentado en el source viejo; la tabla 0x08128AC0 es indice->modelo, no slots)");

	lua_pushnil(L);
	return 1;
}

static int LuaClientIsInventorySlotEmpty(lua_State* L) // Client.IsInventorySlotEmpty(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.IsInventorySlotEmpty: NO CONFIRMADO (mismo motivo que GetInventoryItem)");

	lua_pushnil(L);
	return 1;
}

static int LuaClientGetWearItem(lua_State* L) // Client.GetWearItem(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetWearItem: NO CONFIRMADO (los wear slots no estan documentados en el source viejo)");

	lua_pushnil(L);
	return 1;
}

static int LuaClientGetShopItem(lua_State* L) // Client.GetShopItem(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetShopItem: NO CONFIRMADO (el layout de C2:3F:05/13 no esta suficientemente documentado para parseo seguro)");

	lua_pushnil(L);
	return 1;
}

static int LuaClientGetShopPrice(lua_State* L) // Client.GetShopPrice(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetShopPrice: NO CONFIRMADO (precios de personal shop; el source viejo solo documenta el shop NPC por C1:F3:E7, sin slot->precio en runtime)");

	lua_pushnil(L);
	return 1;
}

static int LuaClientGetChaosItem(lua_State* L) // Client.GetChaosItem(slot)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Client.GetChaosItem: NO CONFIRMADO (los slots de la caja del caos son ventana nativa sin layout documentado)");

	lua_pushnil(L);
	return 1;
}

// ---------------------------------------------------------------------------
// API Lua: Interface (ventanas nativas del cliente)
// ---------------------------------------------------------------------------
static int LuaInterfaceOpen(lua_State* L) // Interface.Open(wid)
{
	int wid = (int)luaL_checkinteger(L,1);

	lua_pushinteger(L,pOpenWindow(pWindowThis(),wid));

	return 1;
}

static int LuaInterfaceClose(lua_State* L) // Interface.Close(wid)
{
	int wid = (int)luaL_checkinteger(L,1);

	lua_pushinteger(L,pClosekWindow(pWindowThis(),wid));

	return 1;
}

static int LuaInterfaceIsOpen(lua_State* L) // Interface.IsOpen(wid) -> bool
{
	int wid = (int)luaL_checkinteger(L,1);

	lua_pushboolean(L,pCheckWindow(pWindowThis(),wid) ? 1 : 0);

	return 1;
}

static int LuaInterfaceGetOpenWindows(lua_State* L) // Interface.GetOpenWindows() -> tabla con los ids abiertos
{
	LPVOID wnd = pWindowThis();

	lua_newtable(L);

	int count = 0;

	for(int id=1;id<=0x1F;id++)
	{
		if(pCheckWindow(wnd,id))
		{
			count++;
			lua_pushinteger(L,id);
			lua_rawseti(L,-2,count);
		}
	}

	return 1;
}

static int LuaInterfaceGetActiveWindow(lua_State* L) // Interface.GetActiveWindow() -> -1 (NO CONFIRMADO)
{
	static bool logged = false;

	LuaLogOnce(logged,"[LuaPlugin] Interface.GetActiveWindow: NO CONFIRMADO (el source viejo no documenta como determinar la ventana activa); devuelve -1");

	lua_pushinteger(L,-1);

	return 1;
}

// ---------------------------------------------------------------------------
// API Lua: UI (bloqueo de clicks sobre UI custom)
// ---------------------------------------------------------------------------
static int LuaBlockMouse(lua_State* L) // UI.BlockMouse(x, y, w, h) -> bool
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	int w = (int)luaL_checkinteger(L,3);
	int h = (int)luaL_checkinteger(L,4);

	bool ok = g_MouseBlock.AddRect(x,y,w,h);

	if(!ok)
	{
		LuaLog("[LuaPlugin] UI.BlockMouse: sin slots libres (max %d)",MAX_BLOCKED_RECTS);
	}

	lua_pushboolean(L,ok ? 1 : 0);

	return 1;
}

static int LuaClearBlockedRects(lua_State* L) // UI.ClearBlockedRects()
{
	g_MouseBlock.Clear();

	return 0;
}

static int LuaSetMouseBlockEnabled(lua_State* L) // UI.SetMouseBlockEnabled(bool) -> bool
{
	g_MouseBlock.active = (lua_toboolean(L,1) != 0);

	lua_pushboolean(L,g_MouseBlock.active ? 1 : 0);

	return 1;
}

static int LuaIsMouseInside(lua_State* L) // UI.IsMouseInside(x, y, w, h) -> bool
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	int w = (int)luaL_checkinteger(L,3);
	int h = (int)luaL_checkinteger(L,4);

	int cx = pCursorX;
	int cy = pCursorY;

	lua_pushboolean(L,(cx >= x && cx <= x+w && cy >= y && cy <= y+h) ? 1 : 0);

	return 1;
}

static int LuaConsumeClick(lua_State* L) // UI.ConsumeClick() -> bool (consumio el juego este click?)
{
	lua_pushboolean(L,g_MouseBlock.ConsumeClick() ? 1 : 0);

	return 1;
}

static int LuaIsMouseBlockHooked(lua_State* L) // UI.IsMouseBlockHooked() -> bool (el hook del call site quedo instalado?)
{
	lua_pushboolean(L,g_MouseClickHooked ? 1 : 0);

	return 1;
}

static int LuaMouseClickCallSite(lua_State* L) // UI.MouseClickCallSite() -> call site del hook (0 = no instalado)
{
	lua_pushinteger(L,g_MouseClickCallSite);

	return 1;
}

static int LuaWndProcInstalled(lua_State* L) // UI.WndProcInstalled() -> bool (capa 2: el WndProc guard quedo instalado?)
{
	lua_pushboolean(L,g_WndProcInstalled ? 1 : 0);

	return 1;
}

static int LuaSendGuardInstalled(lua_State* L) // UI.SendGuardInstalled() -> bool (capa 3: el guard de envio quedo instalado?)
{
	lua_pushboolean(L,g_SendGuardInstalled ? 1 : 0);

	return 1;
}

static int LuaBlockedByHook(lua_State* L) // UI.BlockedByHook() -> clicks consumidos por el hook del call site (capa 1)
{
	lua_pushinteger(L,g_MouseHookBlockedCount);

	return 1;
}

static int LuaBlockedByWndProc(lua_State* L) // UI.BlockedByWndProc() -> mensajes de raton tragados por el WndProc guard (capa 2)
{
	lua_pushinteger(L,g_WndProcBlockedCount);

	return 1;
}

static int LuaBlockedBySend(lua_State* L) // UI.BlockedBySend() -> packets C1:04/05/06 tragados por el SendGuard (capa 3)
{
	lua_pushinteger(L,g_SendBlockedCount);

	return 1;
}

static int LuaCursorInsideUI(lua_State* L) // UI.CursorInsideUI() -> bool (el cursor esta dentro de un rect de UI ahora mismo?)
{
	lua_pushboolean(L,g_MouseBlock.ShouldBlock(pCursorX,pCursorY) ? 1 : 0);

	return 1;
}

// ---------------------------------------------------------------------------
// API Lua: Draw.Image / Draw.LoadImage (sprites custom) - v0.6: por OPENGL
// DIRECTO (glGenTextures/glTexImage2D + quad texturizado). NO usa pLoadImage
// (el cargador nativo 0x00772330 crashea este build de cliente): el archivo
// BMP/TGA se lee con codigo propio y se sube a GL -> cero riesgo de crash.
// ---------------------------------------------------------------------------
#define GL_TEXTURE_SLOTS 64

struct GLTEXTURE_SLOT
{
	GLuint tex;
	int    w;
	int    h;
	bool   used;
};

static GLTEXTURE_SLOT g_GLTextures[GL_TEXTURE_SLOTS];

// Lee un BMP (24/32bpp, BI_RGB) o TGA (24/32bpp, uncompressed/RLE) y devuelve
// pixeles RGBA (fila 0 = arriba). Devuelve false si el formato no se soporta.
static bool LoadImageFile(const char* path,int* outW,int* outH,unsigned char** outPixels)
{
	FILE* f = 0;
	fopen_s(&f,path,"rb");

	if(f == 0)
	{
		return false;
	}

	unsigned char header[54] = {0};
	size_t got = fread(header,1,54,f);
	bool ok = false;

	if(got >= 54 && header[0] == 'B' && header[1] == 'M')
	{
		// ---- BMP ----
		int dataOffset = header[10] | (header[11]<<8) | (header[12]<<16) | ((int)header[13]<<24);
		int w  = header[18] | (header[19]<<8) | (header[20]<<16) | ((int)header[21]<<24);
		int h  = header[22] | (header[23]<<8) | (header[24]<<16) | ((int)header[25]<<24);
		int bpp = header[28] | (header[29]<<8);
		int comp = header[30] | (header[31]<<8) | (header[32]<<16) | ((int)header[33]<<24);

		int absH = (h < 0) ? -h : h;
		bool topDown = (h < 0);

		if(w > 0 && absH > 0 && (bpp == 24 || bpp == 32) && comp == 0)
		{
			int bytesPP = bpp / 8;
			int rowSize = ((w * bytesPP + 3) / 4) * 4;
			unsigned char* pixels = (unsigned char*)malloc(w * absH * 4);

			if(pixels)
			{
				unsigned char* row = (unsigned char*)malloc(rowSize);
				fseek(f,dataOffset,SEEK_SET);

				for(int y = 0; y < absH; y++)
				{
					int srcRow = topDown ? y : (absH - 1 - y);
					if(fread(row,1,rowSize,f) != (size_t)rowSize) break;

					for(int x = 0; x < w; x++)
					{
						unsigned char* s = row + x * bytesPP;
						unsigned char* d = pixels + (y * w + x) * 4;
						d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
						d[3] = (bpp == 32) ? s[3] : 255;
					}
				}

				free(row);
				*outW = w; *outH = absH; *outPixels = pixels;
				ok = true;
			}
		}
	}
	else if(got >= 18 && header[1] == 0 && header[2] == 2)
	{
		// ---- TGA (imageType 2 = uncompressed, 10 = RLE) ----
		int imageType = header[2];
		int w  = header[12] | (header[13]<<8);
		int h  = header[14] | (header[15]<<8);
		int bpp = header[16];
		int desc = header[17];

		if((imageType == 2 || imageType == 10) && w > 0 && h > 0 && (bpp == 24 || bpp == 32))
		{
			int bytesPP = bpp / 8;
			bool rle = (imageType == 10);
			bool topDown = (desc & 0x20) != 0;
			unsigned char* pixels = (unsigned char*)malloc(w * h * 4);

			if(pixels)
			{
				fseek(f,18 + header[0],SEEK_SET); // saltar el id field
				unsigned char* tmp = (unsigned char*)malloc(w * h * bytesPP);
				int total = w * h;
				int idx = 0;

				if(rle)
				{
					while(idx < total)
					{
						unsigned char packet = 0;
						if(fread(&packet,1,1,f) != 1) break;
						int count = (packet & 0x7F) + 1;

						if(packet & 0x80) // run repetido
						{
							unsigned char px[4] = {0,0,0,255};
							if(fread(px,1,bytesPP,f) != (size_t)bytesPP) break;

							while(count-- > 0 && idx < total)
							{
								memcpy(tmp + idx * bytesPP,px,bytesPP);
								idx++;
							}
						}
						else // run directo
						{
							while(count-- > 0 && idx < total)
							{
								if(fread(tmp + idx * bytesPP,1,bytesPP,f) != (size_t)bytesPP) break;
								idx++;
							}
						}
					}
				}
				else
				{
					fread(tmp,1,(size_t)total * bytesPP,f);
				}

				for(int y = 0; y < h; y++)
				{
					int srcRow = topDown ? y : (h - 1 - y);

					for(int x = 0; x < w; x++)
					{
						unsigned char* s = tmp + (srcRow * w + x) * bytesPP;
						unsigned char* d = pixels + (y * w + x) * 4;
						d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
						d[3] = (bpp == 32) ? s[3] : 255;
					}
				}

				free(tmp);
				*outW = w; *outH = h; *outPixels = pixels;
				ok = true;
			}
		}
	}

	fclose(f);

	if(ok == false && outPixels && *outPixels)
	{
		free(*outPixels);
		*outPixels = 0;
	}

	return ok;
}

static int LuaLoadImage(lua_State* L) // Draw.LoadImage(path,w,h) -> id (textura GL)
{
	const char* path = luaL_checkstring(L,1);
	int w = 0, h = 0;
	unsigned char* pixels = 0;

	if(LoadImageFile(path,&w,&h,&pixels) == false)
	{
		LuaLog("[LuaPlugin] Draw.LoadImage: no se pudo leer '%s' (BMP/TGA 24/32bpp)",path);
		lua_pushinteger(L,0);
		return 1;
	}

	int slot = -1;

	for(int n = 0; n < GL_TEXTURE_SLOTS; n++)
	{
		if(g_GLTextures[n].used == false)
		{
			slot = n;
			break;
		}
	}

	if(slot == -1)
	{
		free(pixels);
		LuaLog("[LuaPlugin] Draw.LoadImage: sin slots de textura libres (max %d)",GL_TEXTURE_SLOTS);
		lua_pushinteger(L,0);
		return 1;
	}

	GLuint tex = 0;
	bool created = false;

	// glGenTextures/glTexImage2D: si no hay contexto GL (login/select) o algo
	// falla, NO debe tumbar el cliente: SEH + devuelve 0 (fallback dibujado).
	__try
	{
		glGenTextures(1,&tex);

		if(tex != 0)
		{
			glBindTexture(GL_TEXTURE_2D,tex);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
			glBindTexture(GL_TEXTURE_2D,0);
			created = true;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		LuaLog("[LuaPlugin] Draw.LoadImage: excepcion GL cargando '%s' (fallback)",path);
		tex = 0;
		created = false;
	}

	free(pixels);

	if(created == false)
	{
		lua_pushinteger(L,0);
		return 1;
	}

	g_GLTextures[slot].tex = tex;
	g_GLTextures[slot].w = w;
	g_GLTextures[slot].h = h;
	g_GLTextures[slot].used = true;

	LuaLog("[LuaPlugin] Draw.LoadImage: '%s' (%dx%d) -> id %d (GL)",path,w,h,slot + 1);

	lua_pushinteger(L,slot + 1);
	return 1;
}

static int LuaDrawImage(lua_State* L) // Draw.Image(id,x,y,w,h,u0,v0,u1,v1,alpha)
{
	DWORD id = (DWORD)luaL_checkinteger(L,1);
	float x = (float)luaL_checknumber(L,2);
	float y = (float)luaL_checknumber(L,3);
	float w = (float)luaL_checknumber(L,4);
	float h = (float)luaL_checknumber(L,5);
	float u0 = (float)luaL_optnumber(L,6,0.0f);
	float v0 = (float)luaL_optnumber(L,7,0.0f);
	float u1 = (float)luaL_optnumber(L,8,1.0f);
	float v1 = (float)luaL_optnumber(L,9,1.0f);
	float alpha = (float)luaL_optnumber(L,10,1.0f);

	if(id < 1 || id > GL_TEXTURE_SLOTS || g_GLTextures[id-1].used == false)
	{
		return 0; // id invalido: no dibujar nada
	}

	GLTEXTURE_SLOT* slot = &g_GLTextures[id-1];

	// Quad texturizado con save/restore del estado GL del cliente (mismo patron
	// que el ShaderPlugin): NUNCA dejar el contexto GL modificado.
	GLboolean texEnabled = 0;
	GLboolean blendEnabled = 0;
	GLint prevTex = 0;
	GLint prevMatrixMode = 0;
	GLfloat prevProjection[16];
	GLfloat prevModelview[16];
	GLfloat prevColor[4];
	bool pushed = false;

	__try
	{
		GLint vp[4];

		texEnabled   = glIsEnabled(GL_TEXTURE_2D);
		blendEnabled = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_TEXTURE_BINDING_2D,&prevTex);
		glGetIntegerv(GL_MATRIX_MODE,&prevMatrixMode);
		glGetFloatv(GL_PROJECTION_MATRIX,prevProjection);
		glGetFloatv(GL_MODELVIEW_MATRIX,prevModelview);
		glGetFloatv(GL_CURRENT_COLOR,prevColor);
		glGetIntegerv(GL_VIEWPORT,vp);

		// proyeccion ortografica en pixeles de pantalla (coordenadas de la UI)
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,(GLdouble)vp[2],(GLdouble)vp[3],0,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		pushed = true;

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D,slot->tex);
		glColor4f(1.0f,1.0f,1.0f,alpha);

		glBegin(GL_QUADS);
		glTexCoord2f(u0,v0); glVertex2f(x,y);
		glTexCoord2f(u1,v0); glVertex2f(x+w,y);
		glTexCoord2f(u1,v1); glVertex2f(x+w,y+h);
		glTexCoord2f(u0,v1); glVertex2f(x,y+h);
		glEnd();
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		LuaLog("[LuaPlugin] Draw.Image: excepcion GL dibujando (id=%u), ignorado",(unsigned)id);
	}

	// --- restaurar SIEMPRE (dentro y fuera del handler: un fallo GL no debe
	// dejar el contexto del cliente corrupto) ---
	__try
	{
		glBindTexture(GL_TEXTURE_2D,prevTex);

		if(texEnabled == 0)   glDisable(GL_TEXTURE_2D);
		if(blendEnabled == 0) glDisable(GL_BLEND);

		glColor4f(prevColor[0],prevColor[1],prevColor[2],prevColor[3]);

		if(pushed)
		{
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(prevProjection);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(prevModelview);
		glMatrixMode(prevMatrixMode);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// si hasta restaurar falla, no hay nada mas que hacer: el cliente sigue
	}

	return 0;
}

static int LuaDrawTooltip(lua_State* L) // Draw.Tooltip(x, y, texto)
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	const char* text = luaL_checkstring(L,3);

	pDrawToolTip(x,y,(LPCSTR)text);

	return 0;
}

// ---------------------------------------------------------------------------
// Draw.TextRaw (v0.3.6) - texto en espacio RAW de pixels (resolucion real)
//
// PROBLEMA que resuelve: Draw.Text delega en la funcion nativa del cliente
// (pDrawText, 0x00420150), que posiciona los glifos en el espacio VIRTUAL
// 640x480 que el motor del cliente escala a la resolucion real. Con escalas
// no enteras (800x600 = 1.25) cada glifo cae en pixels fraccionarios y el
// bitmap NONANTIALIASED de la fuente (rasterizado por GDI al tamano nativo)
// se re-muestrea: texto "sucio"/tembloroso, y al redibujar con otro color el
// muestreo cambia el grosor aparente de los trazos ("engorda").
//   El escalado virtual->pantalla ocurre DENTRO del cliente (no en este
// plugin), por eso no se puede corregir el redondeo desde Draw.Text sin
// parchear el binario. La solucion es TextRaw: cada glifo se rasteriza con
// GDI AL TAMANO FINAL (nativo x escala, redondeado a entero) y se dibuja como
// quad texturizado (GL) en coordenadas de pixels REALES y ENTEROS -> cero
// re-muestreo, cero jitter, cero engorde. La posicion la escala/snapea el
// script en Lua (igual que con Draw.Image).
//
// Uso: Draw.TextRaw(x, y, texto, r, g, b, [a])   // x,y en pixels REALES
//   Ejemplo: Draw.TextRaw(snapX(12), snapY(8), "Hola", 255, 255, 255)
// NOTA: el texto se rasteriza por BYTE (ANSI/cp1252, como el pDrawText nativo
// que recibe char*). NO pasar UTF-8 multibyte: 'á' = 0xC3 0xA1 saldria como
// dos glifos. Usar strings ANSI en los scripts.
// ---------------------------------------------------------------------------
#define TEXTTRAW_GLYPHS 256
#define TEXTTRAW_MAX_FONT 96
#define TEXTTRAW_MIN_FONT 6

struct TEXTTRAW_GLYPH
{
	GLuint tex;   // textura GL del glifo (0 si el char solo tiene avance)
	int    w;     // ancho del trazo (abcB) en px
	int    h;     // alto de celda (tmHeight) en px
	int    texW;  // ancho de textura (nextPow2) en px
	int    texH;  // alto de textura (nextPow2) en px
	int    offsetX; // desplazamiento del bitmap respecto al cursor (abcA)
	int    advance; // avance del caracter (abcA+abcB+abcC) en px
	int    size;  // tamano con el que se rasterizo (invalida cache al cambiar)
	bool   valid; // glifo listo (con textura o solo avance)
};

static TEXTTRAW_GLYPH g_TextRawGlyphs[TEXTTRAW_GLYPHS];
static int g_TextRawLineHeight = 0; // salto de linea para '\n' (px)

// Siguiente potencia de 2 (las texturas GL 1.1 del cliente la exigen)
static int TextRawNextPow2(int v)
{
	int p = 1;

	while(p < v) p <<= 1;

	return p;
}

// Fuente del texto: se lee del MISMO Config.ini del cliente (como hace el
// juego en Font.cpp ReadFontFile): FontName (Tahoma), FontSize (13) y
// FontCharSet (1). Asi los glifos usan la MISMA familia y tamano base que el
// texto nativo -> la fuente no se ve "rara" respecto al resto de la UI.
static int  g_TextRawFontSize = 0;
static DWORD g_TextRawFontKey = 0;
static char  g_TextRawFontName[64] = "Tahoma";
static BYTE  g_TextRawFontCharSet = DEFAULT_CHARSET;

static void TextRawLoadFontInfo()
{
	g_TextRawFontSize = 0;

	// .\Config.ini del cliente (el CWD del proceso es la carpeta del juego)
	int cfgSize    = GetPrivateProfileIntA("FontInfo","FontSize",13,".\\Config.ini");
	int cfgCharSet = GetPrivateProfileIntA("FontInfo","FontCharSet",1,".\\Config.ini");

	GetPrivateProfileStringA("FontInfo","FontName","Tahoma",g_TextRawFontName,sizeof(g_TextRawFontName),".\\Config.ini");

	if(g_TextRawFontName[0] == 0)
	{
		strcpy_s(g_TextRawFontName,sizeof(g_TextRawFontName),"Tahoma");
	}

	g_TextRawFontCharSet = (cfgCharSet >= 0 && cfgCharSet <= 255) ? (BYTE)cfgCharSet : DEFAULT_CHARSET;

	// tamano base (virtual) x factor virtual->real (promedio de X e Y)
	DWORD rx = *(DWORD*)MAIN_RESOLUTION_X;
	DWORD ry = *(DWORD*)MAIN_RESOLUTION_Y;

	double sx = (rx > 0) ? (double)rx / 640.0 : 1.0;
	double sy = (ry > 0) ? (double)ry / 480.0 : 1.0;

	int size = (int)((double)cfgSize * ((sx + sy) * 0.5) + 0.5);

	if(size < TEXTTRAW_MIN_FONT) size = TEXTTRAW_MIN_FONT;
	if(size > TEXTTRAW_MAX_FONT) size = TEXTTRAW_MAX_FONT;

	g_TextRawFontSize = size;
}

static int TextRawFontSize()
{
	DWORD rx = *(DWORD*)MAIN_RESOLUTION_X;
	DWORD ry = *(DWORD*)MAIN_RESOLUTION_Y;

	// el tamano solo se recalcula si cambia la resolucion (jamas a mitad de
	// juego): los glifos no se re-rasterizan entre frames
	DWORD key = rx * 10000 + ry;

	if(g_TextRawFontKey != key)
	{
		TextRawLoadFontInfo();
		g_TextRawFontKey = key;
	}

	return g_TextRawFontSize;
}

// Rasteriza el caracter con GDI al tamano final y lo sube a GL. Cache por
// char; si cambia el tamano (cambio de resolucion) se re-rasteriza.
static bool TextRawEnsureGlyph(int c,int size)
{
	if(c < 0 || c >= TEXTTRAW_GLYPHS)
	{
		return false;
	}

	TEXTTRAW_GLYPH* slot = &g_TextRawGlyphs[c];

	if(slot->valid && slot->size == size)
	{
		return true;
	}

	// liberar textura previa (resolucion distinta)
	if(slot->valid && slot->tex != 0)
	{
		__try
		{
			glDeleteTextures(1,&slot->tex);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}

		slot->tex = 0;
	}

	slot->tex = 0;
	slot->valid = false;

	HDC hdc = CreateCompatibleDC(0);

	if(hdc == 0)
	{
		return false;
	}

	// misma familia/charset que el cliente (Config.ini) pero al tamano final
	HFONT hf = CreateFontA(size,0,0,0,FW_NORMAL,0,0,0,g_TextRawFontCharSet,
		OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,g_TextRawFontName);

	if(hf == 0)
	{
		DeleteDC(hdc);
		return false;
	}

	HGDIOBJ oldFont = SelectObject(hdc,hf);

	char ch[2] = { (char)c, 0 };
	ABC abc;
	TEXTMETRIC tm;
	bool ok = false;

	if(GetCharABCWidthsA(hdc,c,c,&abc) != 0 && GetTextMetricsA(hdc,&tm) != 0)
	{
		int gw = abc.abcB;      // ancho del trazo
		int gh = tm.tmHeight;   // alto de celda (ascent + descent)

		slot->w = gw;
		slot->h = gh;
		slot->offsetX = abc.abcA;
		slot->advance = abc.abcA + abc.abcB + abc.abcC;
		slot->size = size;

		if(gh > 0)
		{
			g_TextRawLineHeight = gh;
		}

		if(gw > 0 && gh > 0)
		{
			int texW = TextRawNextPow2(gw);
			int texH = TextRawNextPow2(gh);

			// DIB 32bpp top-down: fondo negro, trazo blanco -> canal alpha
			BITMAPINFO bi;
			memset(&bi,0,sizeof(bi));
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = texW;
			bi.bmiHeader.biHeight = -texH; // top-down
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = 32;
			bi.bmiHeader.biCompression = BI_RGB;

			void* bits = 0;
			HBITMAP hb = CreateDIBSection(hdc,&bi,DIB_RGB_COLORS,&bits,0,0);

			if(hb != 0 && bits != 0)
			{
				HGDIOBJ oldBmp = SelectObject(hdc,hb);

				RECT rc = { 0,0,texW,texH };
				FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));

				SetBkMode(hdc,TRANSPARENT);
				SetTextColor(hdc,RGB(255,255,255));

				// dibujar con el offset del bearing izquierdo: el trazo queda
				// dentro de [0, gw] del bitmap
				TextOutA(hdc,-abc.abcA,0,ch,1);

				unsigned char* rgba = (unsigned char*)malloc(texW * texH * 4);

				if(rgba)
				{
					for(int y = 0; y < texH; y++)
					{
						for(int x = 0; x < texW; x++)
						{
							unsigned char* s = (unsigned char*)bits + (y * texW + x) * 4;
							unsigned char* d = rgba + (y * texW + x) * 4;

							d[0] = 255; d[1] = 255; d[2] = 255; // tint con glColor
							d[3] = s[0]; // blanco sobre negro: R = intensidad
						}
					}

					GLuint tex = 0;

					__try
					{
						glGenTextures(1,&tex);

						if(tex != 0)
						{
							glBindTexture(GL_TEXTURE_2D,tex);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
							glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,texW,texH,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
							glBindTexture(GL_TEXTURE_2D,0);

							slot->tex = tex;
							slot->texW = texW;
							slot->texH = texH;
							slot->valid = true;
							ok = true;
						}
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						// sin contexto GL (login/select): se reintenta el proximo frame
						ok = false;
					}

					free(rgba);
				}

				SelectObject(hdc,oldBmp);
			}

			if(hb != 0)
			{
				DeleteObject(hb);
			}
		}
		else
		{
			// char sin bitmap (espacio, etc.): solo avance
			slot->valid = true;
			ok = true;
		}
	}

	if(oldFont != 0)
	{
		SelectObject(hdc,oldFont);
	}

	DeleteObject(hf);
	DeleteDC(hdc);

	return ok;
}

// Orientacion vertical de los glifos en el quad. El DIB de los glifos es
// top-down (fila 0 = tope) y glTexImage2D coloca la fila 0 en v=0: el tope
// del glifo se muestra con el vertice superior del quad muestreando v=0
// (verificado en vivo: con v=1 arriba los textos salian espejados).
static const float g_TextRawVTop    = 0.0f;
static const float g_TextRawVBottom = 1.0f;

static int LuaDrawTextRaw(lua_State* L) // Draw.TextRaw(x, y, texto, r, g, b, [a])
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	const char* text = luaL_checkstring(L,3);
	float r = (float)luaL_optnumber(L,4,255.0)/255.0f;
	float g = (float)luaL_optnumber(L,5,255.0)/255.0f;
	float b = (float)luaL_optnumber(L,6,255.0)/255.0f;
	float a = (float)luaL_optnumber(L,7,255.0)/255.0f;

	int size = TextRawFontSize();

	// Pre-cache de los glifos del string (medida GDI + rasterizado solo la
	// primera vez). Si falla (sin contexto GL) no se dibuja este frame.
	size_t len = strlen(text);

	for(size_t i = 0; i < len; i++)
	{
		unsigned char c = (unsigned char)text[i];

		if(c == '\r')
		{
			continue;
		}

		if(c == '\n' || c < 32)
		{
			continue; // se manejan en el dibujado (\n) o se ignoran
		}

		if(TextRawEnsureGlyph(c,size) == false)
		{
			return 0;
		}
	}

	// --- dibujar con save/restore del estado GL (mismo patron que Draw.Image)
	GLboolean texEnabled = 0;
	GLboolean blendEnabled = 0;
	GLint prevTex = 0;
	GLint prevMatrixMode = 0;
	GLfloat prevProjection[16];
	GLfloat prevModelview[16];
	GLfloat prevColor[4];
	bool pushed = false;

	__try
	{
		GLint vp[4];

		texEnabled   = glIsEnabled(GL_TEXTURE_2D);
		blendEnabled = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_TEXTURE_BINDING_2D,&prevTex);
		glGetIntegerv(GL_MATRIX_MODE,&prevMatrixMode);
		glGetFloatv(GL_PROJECTION_MATRIX,prevProjection);
		glGetFloatv(GL_MODELVIEW_MATRIX,prevModelview);
		glGetFloatv(GL_CURRENT_COLOR,prevColor);
		glGetIntegerv(GL_VIEWPORT,vp);

		// proyeccion ortografica en pixels reales (coordenadas de la UI)
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,(GLdouble)vp[2],(GLdouble)vp[3],0,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		pushed = true;

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(r,g,b,a);

		// cursor en pixels REALES enteros: los glifos estan rasterizados al
		// tamano final -> cero re-muestreo, cero jitter, cero engorde
		int cursor = x;

		for(size_t i = 0; i < len; i++)
		{
			unsigned char c = (unsigned char)text[i];

			if(c == '\r')
			{
				continue;
			}

			if(c == '\n')
			{
				cursor = x;
				y += (g_TextRawLineHeight > 0) ? g_TextRawLineHeight : size;
				continue;
			}

			if(c == '\t')
			{
				// tab = 4 espacios (avance del espacio, cacheado)
				TEXTTRAW_GLYPH* sp = &g_TextRawGlyphs[32];

				if(sp->valid == false || sp->size != size)
				{
					TextRawEnsureGlyph(32,size);
				}

				cursor += 4 * sp->advance;
				continue;
			}

			TEXTTRAW_GLYPH* slot = &g_TextRawGlyphs[c];

			if(slot->w > 0 && slot->tex != 0)
			{
				float u1 = (float)slot->w / (float)slot->texW;
				float v1 = (float)slot->h / (float)slot->texH;
				float qx = (float)(cursor + slot->offsetX);
				float qy = (float)y;

				glBindTexture(GL_TEXTURE_2D,slot->tex);

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f,g_TextRawVTop);    glVertex2f(qx,qy);
				glTexCoord2f(u1,g_TextRawVTop);      glVertex2f(qx + slot->w,qy);
				glTexCoord2f(u1,g_TextRawVBottom);   glVertex2f(qx + slot->w,qy + slot->h);
				glTexCoord2f(0.0f,g_TextRawVBottom); glVertex2f(qx,qy + slot->h);
				glEnd();
			}

			cursor += slot->advance;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		LuaLog("[LuaPlugin] Draw.TextRaw: excepcion GL dibujando texto, ignorado");
	}

	// --- restaurar SIEMPRE (un fallo GL no debe dejar el contexto corrupto) ---
	__try
	{
		glBindTexture(GL_TEXTURE_2D,prevTex);

		if(texEnabled == 0)   glDisable(GL_TEXTURE_2D);
		if(blendEnabled == 0) glDisable(GL_BLEND);

		glColor4f(prevColor[0],prevColor[1],prevColor[2],prevColor[3]);

		if(pushed)
		{
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(prevProjection);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(prevModelview);
		glMatrixMode(prevMatrixMode);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// si hasta restaurar falla, no hay nada mas que hacer: el cliente sigue
	}

	return 0;
}

// ---------------------------------------------------------------------------
// API Lua: Input (RegisterKey para on_key; KeyPressed consulta directa)
// ---------------------------------------------------------------------------
static int LuaRegisterKey(lua_State* L) // Input.RegisterKey(vk)
{
	int vk = (int)luaL_checkinteger(L,1);

	if(vk < 0 || vk > 255)
	{
		lua_pushboolean(L,0);
		return 1;
	}

	for(int n=0;n<g_SubscribedKeyCount;n++)
	{
		if(g_SubscribedKeys[n] == (BYTE)vk)
		{
			lua_pushboolean(L,1);
			return 1;
		}
	}

	if(g_SubscribedKeyCount < (int)sizeof(g_SubscribedKeys))
	{
		g_SubscribedKeys[g_SubscribedKeyCount++] = (BYTE)vk;

		lua_pushboolean(L,1);
	}
	else
	{
		LuaLog("[LuaPlugin] RegisterKey: sin slots libres (max 32)");
		lua_pushboolean(L,0);
	}

	return 1;
}

static int LuaKeyPressed(lua_State* L) // Input.KeyPressed(vk) -> bool (estado actual)
{
	int vk = (int)luaL_checkinteger(L,1);

	lua_pushboolean(L,(GetAsyncKeyState(vk) & 0x8000) ? 1 : 0);

	return 1;
}

// ---------------------------------------------------------------------------
// Router de opcodes: RegisterPacketHandler(head, sub|nil, fn) -> id
// ---------------------------------------------------------------------------
static PACKET_HANDLER* FindPacketHandler(int head,int sub)
{
	PACKET_HANDLER* wildcard = 0;

	for(int n=0;n<g_PacketHandlerCount;n++)
	{
		if(g_PacketHandlers[n].luaRef == LUA_NOREF)
		{
			continue; // slot libre (tras un Unregister)
		}

		if(g_PacketHandlers[n].head == (BYTE)head)
		{
			if(g_PacketHandlers[n].sub == sub)
			{
				return &g_PacketHandlers[n];
			}

			if(g_PacketHandlers[n].sub == -1 && wildcard == 0)
			{
				wildcard = &g_PacketHandlers[n];
			}
		}
	}

	return wildcard;
}

static int LuaRegisterPacketHandler(lua_State* L) // RegisterPacketHandler(head, sub|nil, fn)
{
	int head = (int)luaL_checkinteger(L,1);

	int sub = -1;

	if(lua_isnil(L,2) == 0)
	{
		sub = (int)luaL_checkinteger(L,2);
	}

	luaL_checktype(L,3,LUA_TFUNCTION);

	// Si ya hay un handler EXACTO (head,sub), devolver su id (no duplicar:
	// el duplicado nunca se dispararia y confundiria al desregistrar)
	PACKET_HANDLER* existing = FindPacketHandler(head,sub);

	if(existing != 0 && existing->sub == sub)
	{
		lua_pop(L,1); // quitar la funcion que no se va a registrar
		lua_pushinteger(L,(int)(existing - g_PacketHandlers) + 1);
		return 1;
	}

	// Buscar slot libre (los ids SON estables: un Unregister deja un hueco,
	// no compacta la tabla)
	int slot = -1;

	for(int n=0;n<g_PacketHandlerCount;n++)
	{
		if(g_PacketHandlers[n].luaRef == LUA_NOREF)
		{
			slot = n;
			break;
		}
	}

	if(slot == -1)
	{
		if(g_PacketHandlerCount >= (int)(sizeof(g_PacketHandlers)/sizeof(g_PacketHandlers[0])))
		{
			LuaLog("[LuaPlugin] RegisterPacketHandler: sin slots libres (max 32)");
			lua_pop(L,1);
			lua_pushinteger(L,0);
			return 1;
		}

		slot = g_PacketHandlerCount;
		g_PacketHandlerCount++;
	}

	int ref = luaL_ref(L,LUA_REGISTRYINDEX);

	if(ref == LUA_REFNIL || ref == LUA_NOREF)
	{
		lua_pushinteger(L,0);
		return 1;
	}

	g_PacketHandlers[slot].head = (BYTE)head;
	g_PacketHandlers[slot].sub = sub;
	g_PacketHandlers[slot].luaRef = ref;

	lua_pushinteger(L,slot + 1); // id = slot+1 (estable)

	return 1;
}

static int LuaUnregisterPacketHandler(lua_State* L) // UnregisterPacketHandler(id)
{
	int id = (int)luaL_checkinteger(L,1);

	if(id >= 1 && id <= g_PacketHandlerCount)
	{
		if(g_PacketHandlers[id-1].luaRef != LUA_NOREF)
		{
			luaL_unref(g_Lua,LUA_REGISTRYINDEX,g_PacketHandlers[id-1].luaRef);

			g_PacketHandlers[id-1].luaRef = LUA_NOREF; // hueco: los ids no se desplazan
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------
// API Lua: SendPacket(hex)
// ---------------------------------------------------------------------------
static int LuaSendPacket(lua_State* L) // SendPacket("C1 05 BF 51 00")
{
	const char* hex = luaL_checkstring(L,1);

	BYTE buffer[2048];
	int len = 0;
	bool truncated = false;

	for(int n=0; hex[n] != 0; n++)
	{
		if(len >= (int)sizeof(buffer))
		{
			truncated = true;
			break;
		}
		
		char c = hex[n];

		if(c == ' ' || c == '\t' || c == ',' || c == '-')
		{
			continue;
		}

		int hi = -1;
		int lo = -1;

		if(c >= '0' && c <= '9') { hi = c-'0'; }
		else if(c >= 'A' && c <= 'F') { hi = c-'A'+10; }
		else if(c >= 'a' && c <= 'f') { hi = c-'a'+10; }
		else
		{
			LuaLog("[LuaPlugin] SendPacket: caracter invalido '%c' en '%s'",c,hex);
			lua_pushboolean(L,0);
			return 1;
		}

		char d = hex[n+1];

		if(d >= '0' && d <= '9') { lo = d-'0'; }
		else if(d >= 'A' && d <= 'F') { lo = d-'A'+10; }
		else if(d >= 'a' && d <= 'f') { lo = d-'a'+10; }
		else if(d == 0)
		{
			LuaLog("[LuaPlugin] SendPacket: numero impar de digitos hex en '%s'",hex);
			lua_pushboolean(L,0);
			return 1;
		}
		else
		{
			LuaLog("[LuaPlugin] SendPacket: caracter invalido '%c' en '%s'",d,hex);
			lua_pushboolean(L,0);
			return 1;
		}

		buffer[len++] = (BYTE)((hi<<4)|lo);
		n++;
	}

	if(truncated)
	{
		LuaLog("[LuaPlugin] SendPacket: packet truncado a %d bytes (max 2048)",len);
	}

	if(len < 3)
	{
		LuaLog("[LuaPlugin] SendPacket: el paquete necesita al menos type+size+head");
		lua_pushboolean(L,0);
		return 1;
	}

	// Auto-ajustar el campo de tamano de la cabecera
	switch(buffer[0])
	{
		case 0xC1:
		case 0xC3:
			buffer[1] = (BYTE)len;
			break;
		case 0xC2:
		case 0xC4:
			buffer[1] = HIBYTE(len);
			buffer[2] = LOBYTE(len);
			break;
		default:
			LuaLog("[LuaPlugin] SendPacket: tipo de cabecera invalido 0x%02X",buffer[0]);
			lua_pushboolean(L,0);
			return 1;
	}

	bool result = SendPacket(buffer,len);

	if(result == false)
	{
		LuaLog("[LuaPlugin] SendPacket: fallo al enviar %s",hex);
	}

	lua_pushboolean(L,result ? 1 : 0);
	return 1;
}

// ---------------------------------------------------------------------------
// Registro de funciones Lua
// ---------------------------------------------------------------------------
static void RegisterLuaFunctions(lua_State* L)
{
	// Tabla Draw
	lua_createtable(L,0,7);
	lua_pushcfunction(L,LuaDrawText);     lua_setfield(L,-2,"Text");
	lua_pushcfunction(L,LuaDrawTextRaw);  lua_setfield(L,-2,"TextRaw");
	lua_pushcfunction(L,LuaDrawBar);      lua_setfield(L,-2,"Bar");
	lua_pushcfunction(L,LuaDrawMessage);  lua_setfield(L,-2,"Message");
	lua_pushcfunction(L,LuaDrawImage);    lua_setfield(L,-2,"Image");
	lua_pushcfunction(L,LuaLoadImage);    lua_setfield(L,-2,"LoadImage");
	lua_pushcfunction(L,LuaDrawTooltip);  lua_setfield(L,-2,"Tooltip");
	lua_setglobal(L,"Draw");

	// Tabla Input
	lua_createtable(L,0,4);
	lua_pushcfunction(L,LuaCursorX);      lua_setfield(L,-2,"CursorX");
	lua_pushcfunction(L,LuaCursorY);      lua_setfield(L,-2,"CursorY");
	lua_pushcfunction(L,LuaRegisterKey);  lua_setfield(L,-2,"RegisterKey");
	lua_pushcfunction(L,LuaKeyPressed);   lua_setfield(L,-2,"KeyPressed");
	lua_setglobal(L,"Input");

	// Tabla Interface (ventanas nativas del cliente)
	lua_createtable(L,0,5);
	lua_pushcfunction(L,LuaInterfaceOpen);       lua_setfield(L,-2,"Open");
	lua_pushcfunction(L,LuaInterfaceClose);      lua_setfield(L,-2,"Close");
	lua_pushcfunction(L,LuaInterfaceIsOpen);     lua_setfield(L,-2,"IsOpen");
	lua_pushcfunction(L,LuaInterfaceGetOpenWindows); lua_setfield(L,-2,"GetOpenWindows");
	lua_pushcfunction(L,LuaInterfaceGetActiveWindow); lua_setfield(L,-2,"GetActiveWindow");
	lua_setglobal(L,"Interface");

	// Tabla UI (bloqueo de clicks sobre UI custom)
	lua_createtable(L,0,12);
	lua_pushcfunction(L,LuaBlockMouse);            lua_setfield(L,-2,"BlockMouse");
	lua_pushcfunction(L,LuaClearBlockedRects);     lua_setfield(L,-2,"ClearBlockedRects");
	lua_pushcfunction(L,LuaSetMouseBlockEnabled);  lua_setfield(L,-2,"SetMouseBlockEnabled");
	lua_pushcfunction(L,LuaIsMouseInside);         lua_setfield(L,-2,"IsMouseInside");
	lua_pushcfunction(L,LuaConsumeClick);          lua_setfield(L,-2,"ConsumeClick");
	lua_pushcfunction(L,LuaIsMouseBlockHooked);    lua_setfield(L,-2,"IsMouseBlockHooked");
	lua_pushcfunction(L,LuaMouseClickCallSite);    lua_setfield(L,-2,"MouseClickCallSite");
	// Estado por capa (diagnostico en vivo de FASE 6)
	lua_pushcfunction(L,LuaWndProcInstalled);     lua_setfield(L,-2,"WndProcInstalled");
	lua_pushcfunction(L,LuaSendGuardInstalled);   lua_setfield(L,-2,"SendGuardInstalled");
	lua_pushcfunction(L,LuaBlockedByHook);        lua_setfield(L,-2,"BlockedByHook");
	lua_pushcfunction(L,LuaBlockedByWndProc);     lua_setfield(L,-2,"BlockedByWndProc");
	lua_pushcfunction(L,LuaBlockedBySend);        lua_setfield(L,-2,"BlockedBySend");
	lua_pushcfunction(L,LuaCursorInsideUI);       lua_setfield(L,-2,"CursorInsideUI");
	lua_setglobal(L,"UI");

	// Tabla Client (datos del personaje desde MAIN_CHARACTER_STRUCT + cache)
	lua_createtable(L,0,24);
	lua_pushcfunction(L,LuaScreenState);      lua_setfield(L,-2,"ScreenState");
	lua_pushcfunction(L,LuaInGame);           lua_setfield(L,-2,"InGame");
	lua_pushcfunction(L,LuaResolutionX);      lua_setfield(L,-2,"ResolutionX");
	lua_pushcfunction(L,LuaResolutionY);      lua_setfield(L,-2,"ResolutionY");
	lua_pushcfunction(L,LuaCharacterName);    lua_setfield(L,-2,"CharacterName");
	lua_pushcfunction(L,LuaCharacterLevel);   lua_setfield(L,-2,"Level");
	lua_pushcfunction(L,LuaCharacterClass);   lua_setfield(L,-2,"Class");
	lua_pushcfunction(L,LuaCharacterHP);      lua_setfield(L,-2,"HP");
	lua_pushcfunction(L,LuaCharacterMaxHP);   lua_setfield(L,-2,"MaxHP");
	lua_pushcfunction(L,LuaCharacterMP);      lua_setfield(L,-2,"MP");
	lua_pushcfunction(L,LuaCharacterMaxMP);   lua_setfield(L,-2,"MaxMP");
	lua_pushcfunction(L,LuaCharacterShield);  lua_setfield(L,-2,"Shield");
	lua_pushcfunction(L,LuaCharacterMaxShield);lua_setfield(L,-2,"MaxShield");
	lua_pushcfunction(L,LuaCharacterBP);      lua_setfield(L,-2,"BP");
	lua_pushcfunction(L,LuaCharacterMaxBP);   lua_setfield(L,-2,"MaxBP");
	lua_pushcfunction(L,LuaCharacterStrength);lua_setfield(L,-2,"Strength");
	lua_pushcfunction(L,LuaCharacterDexterity);lua_setfield(L,-2,"Dexterity");
	lua_pushcfunction(L,LuaCharacterVitality);lua_setfield(L,-2,"Vitality");
	lua_pushcfunction(L,LuaCharacterEnergy);  lua_setfield(L,-2,"Energy");
	lua_pushcfunction(L,LuaCharacterLeadership);lua_setfield(L,-2,"Leadership");
	lua_pushcfunction(L,LuaCharacterLevelUpPoint);lua_setfield(L,-2,"LevelUpPoint");
	lua_pushcfunction(L,LuaCharacterExperience);lua_setfield(L,-2,"Experience");
	lua_pushcfunction(L,LuaCharacterNextExperience);lua_setfield(L,-2,"NextExperience");
	lua_pushcfunction(L,LuaCharacterMoney);   lua_setfield(L,-2,"Money");
	lua_pushcfunction(L,LuaCharacterMap);     lua_setfield(L,-2,"Map");
	// Estados cacheados por packets (FASE 7)
	lua_pushcfunction(L,LuaClientIsTradeOpen);       lua_setfield(L,-2,"IsTradeOpen");
	lua_pushcfunction(L,LuaClientIsTradeAccepted);   lua_setfield(L,-2,"IsTradeAccepted");
	lua_pushcfunction(L,LuaClientGetTradeMoney);     lua_setfield(L,-2,"GetTradeMoney");
	lua_pushcfunction(L,LuaClientGetTradeItem);      lua_setfield(L,-2,"GetTradeItem");
	lua_pushcfunction(L,LuaClientIsShopOpen);        lua_setfield(L,-2,"IsShopOpen");
	lua_pushcfunction(L,LuaClientIsChaosBoxOpen);    lua_setfield(L,-2,"IsChaosBoxOpen");
	// NO CONFIRMADOS (devuelven nil/false y loguean una vez)
	lua_pushcfunction(L,LuaClientIsNpcDialogOpen);   lua_setfield(L,-2,"IsNpcDialogOpen");
	lua_pushcfunction(L,LuaClientGetNpcIndex);       lua_setfield(L,-2,"GetNpcIndex");
	lua_pushcfunction(L,LuaClientGetNpcName);        lua_setfield(L,-2,"GetNpcName");
	lua_pushcfunction(L,LuaClientGetInventoryItem);  lua_setfield(L,-2,"GetInventoryItem");
	lua_pushcfunction(L,LuaClientIsInventorySlotEmpty); lua_setfield(L,-2,"IsInventorySlotEmpty");
	lua_pushcfunction(L,LuaClientGetWearItem);       lua_setfield(L,-2,"GetWearItem");
	lua_pushcfunction(L,LuaClientGetShopItem);       lua_setfield(L,-2,"GetShopItem");
	lua_pushcfunction(L,LuaClientGetShopPrice);      lua_setfield(L,-2,"GetShopPrice");
	lua_pushcfunction(L,LuaClientGetChaosItem);      lua_setfield(L,-2,"GetChaosItem");
	lua_setglobal(L,"Client");

	// Log global
	lua_register(L,"Log",LuaLogMsg);

	// Packets: SendPacket("C1 05 BF 51 00") -> devuelve true/false
	lua_register(L,"SendPacket",LuaSendPacket);

	// Router de opcodes: RegisterPacketHandler(head, sub|nil, fn) -> id
	lua_register(L,"RegisterPacketHandler",LuaRegisterPacketHandler);
	lua_register(L,"UnregisterPacketHandler",LuaUnregisterPacketHandler);
}

// ---------------------------------------------------------------------------
// Inicializacion de Lua: estado + API + script
// ---------------------------------------------------------------------------
void InitLua()
{
	CreateDirectory("Lua",0);

	g_Lua = luaL_newstate();

	if(g_Lua == 0)
	{
		LuaLog("[LuaPlugin] Error: no se pudo crear el estado Lua");
		return;
	}

	luaL_openlibs(g_Lua);

	RegisterLuaFunctions(g_Lua);

	if(luaL_loadfile(g_Lua,LUA_SCRIPT_PATH) != 0 || lua_pcall(g_Lua,0,0,0) != 0)
	{
		LuaLog("[LuaPlugin] Error al cargar %s: %s",LUA_SCRIPT_PATH,lua_tostring(g_Lua,-1));
		lua_pop(g_Lua,1);
		g_LuaScriptError = true;
		return;
	}

	g_LuaReady = true;

	LuaLog("[LuaPlugin] Lua listo. Script: %s",LUA_SCRIPT_PATH);
}

// ---------------------------------------------------------------------------
// Render por frame: se llama desde el hook de dibujo
// ---------------------------------------------------------------------------
static void RenderLuaUI()
{
	if(g_Lua == 0 || g_LuaReady == 0 || g_LuaScriptError != 0 || g_LuaRenderError != 0)
	{
		return;
	}

	// Solo dibujar dentro del juego
	if(*(DWORD*)MAIN_SCREEN_STATE != 5)
	{
		return;
	}

	lua_getglobal(g_Lua,"on_draw");

	if(lua_isfunction(g_Lua,-1) == 0)
	{
		lua_pop(g_Lua,1);
		return;
	}

	lua_pushinteger(g_Lua,pCursorX);
	lua_pushinteger(g_Lua,pCursorY);

	if(lua_pcall(g_Lua,2,0,0) != 0)
	{
		LuaLog("[LuaPlugin] Error en on_draw: %s",lua_tostring(g_Lua,-1));
		lua_pop(g_Lua,1);

		// Desactivar solo el dibujo para no spamear el log cada frame
		g_LuaRenderError = true;
	}
}

// ---------------------------------------------------------------------------
// Input polling por frame: on_key(vk, pressed) para teclas suscritas con
// Input.RegisterKey, y on_click(x, y, boton, pressed) para el raton (botones
// 1=izq, 2=der, 4=medio). Solo dispara en TRANSICIONES de estado, dentro del
// juego y con el cliente en primer plano.
// ---------------------------------------------------------------------------
static void PollLuaInput()
{
	if(g_Lua == 0 || g_LuaReady == 0 || g_LuaScriptError != 0 || g_LuaInputError != 0)
	{
		return;
	}

	if(*(DWORD*)MAIN_SCREEN_STATE != 5)
	{
		return;
	}

	// Gate de foco SOLO si el handle de la ventana es valido (IsWindow): si en
	// un build el offset MAIN_WINDOW no contuviera el handle correcto, no se
	// bloquea el input (eso dejaba on_click mudo mientras el juego si recibia
	// los clicks: botones sin responder + personaje moviendose).
	HWND gameHwnd = *(HWND*)MAIN_WINDOW;

	if(gameHwnd != 0 && IsWindow(gameHwnd) != 0 && GetForegroundWindow() != gameHwnd)
	{
		// Sin foco: sincronizar el estado previo con el actual para que al
		// recuperar el foco no se disparen transiciones espurias (pulsar y
		// soltar una tecla con el cliente en segundo plano).
		for(int n=0;n<g_SubscribedKeyCount;n++)
		{
			BYTE vk = g_SubscribedKeys[n];
			g_PrevKeyState[vk] = ((GetAsyncKeyState(vk) & 0x8000) != 0) ? 1 : 0;
		}

		g_PrevKeyState[VK_LBUTTON] = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) ? 1 : 0;
		g_PrevKeyState[VK_RBUTTON] = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) ? 1 : 0;
		g_PrevKeyState[VK_MBUTTON] = ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) ? 1 : 0;

		return;
	}

	// Un error puntual en un callback NO debe dejar el input mudo para siempre:
	// se cuentan fallos CONSECUTIVOS y solo tras 5 se desactiva (script roto).
	// Con un error transitorio (p.ej. una API no disponible un frame) se sigue
	// sondeando y los botones vuelven a responder solos.
	bool hadError = false;

	// --- Teclas suscritas (on_key) ---
	for(int n=0;n<g_SubscribedKeyCount;n++)
	{
		BYTE vk = g_SubscribedKeys[n];

		bool down = ((GetAsyncKeyState(vk) & 0x8000) != 0);

		if(down == (g_PrevKeyState[vk] != 0))
		{
			continue; // sin transicion
		}

		g_PrevKeyState[vk] = down ? 1 : 0;

		lua_getglobal(g_Lua,"on_key");

		if(lua_isfunction(g_Lua,-1) == 0)
		{
			lua_pop(g_Lua,1);
			continue;
		}

		lua_pushinteger(g_Lua,vk);
		lua_pushboolean(g_Lua,down ? 1 : 0);

		if(lua_pcall(g_Lua,2,0,0) != 0)
		{
			LuaLog("[LuaPlugin] Error en on_key: %s",lua_tostring(g_Lua,-1));
			lua_pop(g_Lua,1);

			hadError = true;

			if(++g_ConsecutiveInputErrors >= 5)
			{
				g_LuaInputError = true;
			}
		}
	}

	// --- Botones del raton (on_click), siempre activos ---
	static const BYTE mouseButtons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };

	for(int n=0;n<3;n++)
	{
		BYTE vk = mouseButtons[n];

		bool down = ((GetAsyncKeyState(vk) & 0x8000) != 0);

		if(down == (g_PrevKeyState[vk] != 0))
		{
			continue;
		}

		g_PrevKeyState[vk] = down ? 1 : 0;

		lua_getglobal(g_Lua,"on_click");

		if(lua_isfunction(g_Lua,-1) == 0)
		{
			lua_pop(g_Lua,1);
			continue;
		}

		lua_pushinteger(g_Lua,pCursorX);
		lua_pushinteger(g_Lua,pCursorY);
		lua_pushinteger(g_Lua,vk);
		lua_pushboolean(g_Lua,down ? 1 : 0);

		if(lua_pcall(g_Lua,4,0,0) != 0)
		{
			LuaLog("[LuaPlugin] Error en on_click: %s",lua_tostring(g_Lua,-1));
			lua_pop(g_Lua,1);

			hadError = true;

			if(++g_ConsecutiveInputErrors >= 5)
			{
				g_LuaInputError = true;
			}
		}
	}

	if(hadError == false)
	{
		g_ConsecutiveInputErrors = 0;
	}
}

// ---------------------------------------------------------------------------
// Hook de render (patron de HealthBar.cpp del Main_EX603) CON CHAINING:
// si main.dll ya hookeo RENDER_HOOK_OFFSET, encadenamos su funcion en vez de
// romperla. Si no hay hook previo, llamamos a la funcion original del cliente.
// ---------------------------------------------------------------------------
void DrawLuaUI()
{
	// Capa 2 (WndProc): se instala en el primer frame de render, cuando la
	// ventana del cliente ya existe y los hooks de main.dll estan activos.
	InstallWndProcGuard();

	// Capa 3: si main.dll re-escribe CLIENT_SEND_POINTER con MySend despues de
	// cargar los plugins, el guard quedaria saltado (el personaje se moveria al
	// clickear la UI). Este chequeo barato (un DWORD por frame) lo re-instala.
	MaintainSendGuard();

	// 1) cadena de hooks: primero lo que habia antes
	((void(*)())g_OriginalRenderFunction)();

	// 2) UI dibujada desde Lua
	RenderLuaUI();

	// 3) input (on_key / on_click) sondeado por frame
	PollLuaInput();

	// 4) restaurar estado GL
	glColor3f(1.0f,1.0f,1.0f);
}

static void InstallRenderHook()
{
	BYTE opcode = *(BYTE*)RENDER_HOOK_OFFSET;

	if(opcode == 0xE8) // call rel32 ya presente (hook de main.dll) -> encadenar
	{
		g_OriginalRenderFunction = RENDER_HOOK_OFFSET + 5 + (*(DWORD*)(RENDER_HOOK_OFFSET+1));

		LuaLog("[LuaPlugin] Render hook encadenado al destino 0x%08X",g_OriginalRenderFunction);
	}
	else
	{
		g_OriginalRenderFunction = RENDER_ORIGINAL;

		LuaLog("[LuaPlugin] Render hook sin hook previo, original = 0x%08X",RENDER_ORIGINAL);
	}

	SetCompleteHook(0xE8,RENDER_HOOK_OFFSET,&DrawLuaUI);

	LuaLog("[LuaPlugin] Render hook instalado en 0x%08X",RENDER_HOOK_OFFSET);
}

// ---------------------------------------------------------------------------
// Verifica que una direccion apunta a memoria de codigo ejecutable dentro del
// proceso (acepta cualquier base de modulo: main.dll, DLLs reubicadas por ASLR,
// el main.exe a 0x00400000, etc.). Sustituye al antiguo sanity-check por rango
// fijo (0x00400000-0x10000000), que RECHAZABA el ProtocolCoreEx de main.dll
// (base por defecto del linker = 0x10000000, funciones en 0x1000xxxx) y rompia
// la cadena de packets al entrar al mundo (F3:0E0, F1:00, etc.).
// ---------------------------------------------------------------------------
static bool IsExecutableCode(DWORD addr)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(VirtualQuery((LPCVOID)addr,&mbi,sizeof(mbi)) == 0)
	{
		return false;
	}

	if(mbi.State != MEM_COMMIT)
	{
		return false;
	}

	switch(mbi.Protect & 0xFF)
	{
		case PAGE_EXECUTE:
		case PAGE_EXECUTE_READ:
		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// Packets: claves + envio (replica del DataSend de main.dll)
// ---------------------------------------------------------------------------
void InitPacketManager()
{
	bool enc = gPacketManager.LoadEncryptionKey("Data\\Enc1.dat");
	bool dec = gPacketManager.LoadDecryptionKey("Data\\Dec2.dat");

	if(enc == 0)
	{
		LuaLog("[LuaPlugin] Error: no se pudo cargar Data\\Enc1.dat (clave de envio)");
	}

	if(dec == 0)
	{
		LuaLog("[LuaPlugin] Error: no se pudo cargar Data\\Dec2.dat (clave de recepcion)");
	}

	if(enc != 0 && dec != 0)
	{
		LuaLog("[LuaPlugin] PacketManager listo (Enc1.dat / Dec2.dat)");
	}
	else
	{
		LuaLog("[LuaPlugin] ADVERTENCIA: los packets C3/C4 fallaran sin las claves de cifrado");
	}
}

bool SendPacket(BYTE* lpMsg,DWORD size)
{
	if(lpMsg == 0 || size < 3 || size >= 2048)
	{
		return false;
	}

	BYTE EncBuff[2048];

	if(gPacketManager.AddData(lpMsg,size) == 0 || gPacketManager.ExtractPacket(EncBuff) == 0)
	{
		LuaLog("[LuaPlugin] SendPacket: cabecera invalida (primer byte 0x%02X)",lpMsg[0]);
		return false;
	}

	BYTE send[2048];

	memcpy(send,EncBuff,size);

	// C3/C4: inyectar serial + cifrar (igual que DataSend de main.dll)
	if(EncBuff[0] == 0xC3 || EncBuff[0] == 0xC4)
	{
		if(EncBuff[0] == 0xC3)
		{
			BYTE save = EncBuff[1];

			EncBuff[1] = (*(BYTE*)(MAIN_PACKET_SERIAL))++;

			size = gPacketManager.Encrypt(&send[2],&EncBuff[1],(size-1))+2;

			EncBuff[1] = save;

			send[0] = 0xC3;
			send[1] = LOBYTE(size);
		}
		else
		{
			BYTE save = EncBuff[2];

			EncBuff[2] = (*(BYTE*)(MAIN_PACKET_SERIAL))++;

			size = gPacketManager.Encrypt(&send[3],&EncBuff[2],(size-2))+3;

			EncBuff[2] = save;

			send[0] = 0xC4;
			send[1] = HIBYTE(size);
			send[2] = LOBYTE(size);
		}
	}

	SOCKET s = *(SOCKET*)(MAIN_ACTIVE_SOCKET+0x0C);

	if(s == 0 || s == INVALID_SOCKET)
	{
		LuaLog("[LuaPlugin] SendPacket: socket inactivo");
		return false;
	}

	// Puntero de envio del cliente: main.dll ya lo sustituyo por MySend, que
	// aplica el cifrado de stream (EncryptData) y llama al send() real.
	WSSEND ClientSend = *(WSSEND*)(CLIENT_SEND_POINTER);

	if(ClientSend == 0)
	{
		LuaLog("[LuaPlugin] SendPacket: puntero de envio no disponible");
		return false;
	}

	return (ClientSend(s,(char*)send,size,0) != SOCKET_ERROR);
}

// ---------------------------------------------------------------------------
// Recepcion de packets: on_packet(head, sub, data_hex)
// ---------------------------------------------------------------------------
static void PushPacketHex(lua_State* L,BYTE* lpMsg,int size)
{
	static const char digits[] = "0123456789ABCDEF";

	luaL_Buffer b;

	luaL_buffinit(L,&b);

	for(int n=0;n<size;n++)
	{
		if(n > 0)
		{
			luaL_addchar(&b,' ');
		}

		luaL_addchar(&b,digits[lpMsg[n] >> 4]);
		luaL_addchar(&b,digits[lpMsg[n] & 0x0F]);
	}

	luaL_pushresult(&b);
}

// Dispatcher de recepcion. Se instala en PROTOCOL_HOOK_OFFSET con chaining:
//   cliente -> LuaProtocolCoreEx -> ProtocolCoreEx de main.dll -> ProtocolCore
// El script define on_packet(head, sub, data_hex):
//   - Devuelve true  -> el packet se CONSUME (no llega al cliente original)
//   - Devuelve false/nil -> el packet sigue su flujo normal
static BOOL LuaProtocolCoreEx(BYTE head,BYTE* lpMsg,int size,int key)
{
	// Sub segun tipo: C1/C3 -> byte[3]; C2/C4 -> byte[4]
	int msgSize = 0;
	int sub = -1;

	if(lpMsg != 0 && size >= 3)
	{
		msgSize = (size > 2048) ? 2048 : size;

		if(lpMsg[0] == 0xC1 || lpMsg[0] == 0xC3)
		{
			if(msgSize >= 4) { sub = lpMsg[3]; }
		}
		else
		{
			if(msgSize >= 5) { sub = lpMsg[4]; }
		}

		// Cache del dinero del personaje (no vive en MAIN_CHARACTER_STRUCT):
		// parser compartido con el test harness (MoneyCache.h) para que el test
		// ejercite el mismo codigo real que corre aca dentro del cliente.
		// Se actualiza SIEMPRE, aunque el script Lua tenga errores: el dinero
		// solo llega por estos packets y no debe congelarse si falla un script.
		DWORD money = 0;

		if(ParseMoneyFromPacket(head,sub,lpMsg,msgSize,&money))
		{
			g_MoneyCache = money;
		}

		// Cache de estados (trade / personal shop / caja del caos): mismo
		// criterio, se actualiza SIEMPRE y antes de tocar Lua (ClientStateCache.h
		// es compartido con el test harness).
		ParseClientStatePacket(head,sub,lpMsg,msgSize,&g_ClientState);
	}

	if(g_Lua != 0 && g_LuaReady != 0 && g_LuaScriptError == 0 && g_LuaPacketError == 0 && lpMsg != 0 && size >= 3)
	{
		// Copia local: no mutamos 'size' porque se reenvia intacto a la cadena
		// (ProtocolCoreEx de main.dll) al final de la funcion.

		// 1) ROUTER: handler exacto [head][sub] o wildcard [head][nil]
		PACKET_HANDLER* handler = FindPacketHandler(head,sub);

		if(handler != 0)
		{
			lua_rawgeti(g_Lua,LUA_REGISTRYINDEX,handler->luaRef);

			lua_pushinteger(g_Lua,head);
			lua_pushinteger(g_Lua,sub);
			PushPacketHex(g_Lua,lpMsg,msgSize);

			if(lua_pcall(g_Lua,3,1,0) == 0)
			{
				bool handled = (lua_toboolean(g_Lua,-1) != 0);

				lua_pop(g_Lua,1);

				if(handled != 0)
				{
					return 1; // consumido por el handler
				}
			}
			else
			{
				LuaLog("[LuaPlugin] Error en handler de packet: %s",lua_tostring(g_Lua,-1));
				lua_pop(g_Lua,1);

				// Desactivar solo los callbacks de packets (el packet se re-envia)
				g_LuaPacketError = true;
			}

			// Handler registrado pero devolvio false: el packet sigue la cadena
			// (NO se llama al on_packet global: cada packet tiene UN consumidor)
			return g_OriginalPacketHandler(head,lpMsg,size,key);
		}

		// 2) Fallback: on_packet global (backward compatible)
		lua_getglobal(g_Lua,"on_packet");

		if(lua_isfunction(g_Lua,-1) != 0)
		{
			lua_pushinteger(g_Lua,head);
			lua_pushinteger(g_Lua,sub);
			PushPacketHex(g_Lua,lpMsg,msgSize);

			if(lua_pcall(g_Lua,3,1,0) == 0)
			{
				bool handled = (lua_toboolean(g_Lua,-1) != 0);

				lua_pop(g_Lua,1);

				if(handled != 0)
				{
					return 1; // consumido por Lua
				}
			}
			else
			{
				LuaLog("[LuaPlugin] Error en on_packet: %s",lua_tostring(g_Lua,-1));
				lua_pop(g_Lua,1);

				// Desactivar solo los callbacks de packets (el packet se re-envia)
				g_LuaPacketError = true;
			}
		}
		else
		{
			lua_pop(g_Lua,1);
		}
	}

	// Cadena de hooks: dispatcher previo (ProtocolCoreEx de main.dll o el original)
	return g_OriginalPacketHandler(head,lpMsg,size,key);
}

static void InstallPacketHook()
{
	BYTE opcode = *(BYTE*)PROTOCOL_HOOK_OFFSET;

	// main.dll ya redirige PROTOCOL_HOOK_OFFSET a su ProtocolCoreEx.
	// Guardamos ese destino ANTES de sobrescribir y encadenamos.
	if(opcode == 0xE8 || opcode == 0xE9) // call/jmp rel32: el destino es valido
	{
		DWORD target = PROTOCOL_HOOK_OFFSET + 5 + (*(DWORD*)(PROTOCOL_HOOK_OFFSET+1));

		// Sanity-check: el destino debe ser memoria de codigo EJECUTABLE en el
		// proceso. NO usamos un rango fijo porque main.dll (que ya hookeo este
		// call con su ProtocolCoreEx) se carga con base por defecto 0x10000000
		// o reubicada por ASLR -> sus funciones estan SIEMPRE por encima de
		// 0x10000000. Con VirtualQuery aceptamos cualquier base de modulo.
		if(IsExecutableCode(target))
		{
			g_OriginalPacketHandler = (BOOL(*)(BYTE,BYTE*,int,int))target;

			LuaLog("[LuaPlugin] Packet hook encadenado al dispatcher 0x%08X (codigo ejecutable)",target);
		}
		else
		{
			g_OriginalPacketHandler = (BOOL(*)(BYTE,BYTE*,int,int))ProtocolCore;

			LuaLog("[LuaPlugin] Packet hook: destino 0x%08X no ejecutable, usando ProtocolCore 0x%08X",target,(DWORD)ProtocolCore);
		}
	}
	else
	{
		// Sin hook previo (main.dll no instalo ProtocolCoreEx): encadenar al
		// dispatcher original del cliente.
		g_OriginalPacketHandler = (BOOL(*)(BYTE,BYTE*,int,int))ProtocolCore;

		LuaLog("[LuaPlugin] Packet hook sin hook previo (opcode 0x%02X), original = 0x%08X",opcode,(DWORD)ProtocolCore);
	}

	SetCompleteHook(0xFF,PROTOCOL_HOOK_OFFSET,&LuaProtocolCoreEx);

	LuaLog("[LuaPlugin] Packet hook instalado en 0x%08X",PROTOCOL_HOOK_OFFSET);
}

// ---------------------------------------------------------------------------
// PUNTO DE ENTRADA DEL PLUGIN (lo llama main.dll via GetProcAddress("EntryProc"))
// ---------------------------------------------------------------------------
extern "C" _declspec(dllexport) void EntryProc()
{
	// Carpeta del log/scripts (relativa al main.exe del cliente)
	CreateDirectory("Lua",0);

	LuaLog("======================================================");
	LuaLog("[LuaPlugin] %s v%s",PLUGIN_NAME,PLUGIN_VERSION);
	LuaLog("[LuaPlugin] Proceso: %u Hilo: %u",GetCurrentProcessId(),GetCurrentThreadId());

	// Nota: aqui los hooks base de main.dll YA estan instalados
	// (gProtect.CheckPluginFile() se ejecuta dentro del EntryProc de main.dll,
	// antes de sus SetWindowsHookEx globales).

	InstallPacketHook();

	InstallRenderHook();

	InstallMouseClickHook();

	InstallSendGuard();

	InitPacketManager();

	InitLua();

	LuaLog("[LuaPlugin] Inicializacion terminada.");
}

// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_PluginInstance = (HINSTANCE)hModule;
			break;
	}

	return 1;
}
