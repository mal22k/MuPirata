// ============================================================================
// ShaderPlugin.cpp
// DLL de POST-PROCESADO para el cliente MU Online EX603 (SSeMU Season 6).
//
// INDEPENDIENTE del sistema LuaPlugin: no usa Lua en absoluto.
//
// METODO DE INYECCION (flujo GetMainInfo, igual que Lua.dll):
//   1) main.exe parcheado carga main.dll y llama su EntryProc()
//   2) main.dll lee ServerInfo.sse y ejecuta gProtect.CheckPluginFile()
//   3) CheckPluginFile hace LoadLibrary() de cada PluginName y llama EntryProc()
//      -> este codigo corre DENTRO del proceso del cliente.
//
// Registro: MainInfo.ini -> [MainInfo]
//              PluginName1 = Lua.dll
//              PluginName2 = Shader.dll
//           + GetMainInfo.exe -> regenera ServerInfo.sse (con CRC de la DLL)
//
// QUE HACE:
//   - Hookea SwapBuffers (gdi32) CON TRAMPOLIN: justo antes del flip, copia el
//     backbuffer a una textura y dibuja un quad fullscreen con el shader
//     Shader\post.fs (bloom, anamorphic flare, god rays, sharpen, vineta,
//     HP bajo, clima...).
//   - Solo procesa el frame de la ventana del cliente y (por defecto) solo
//     dentro del juego (MAIN_SCREEN_STATE == 5).
//   - Tecla de toggle configurable (Shader.ini, por defecto VK_HOME = 0x24).
//   - RED DE SEGURIDAD: toda la pasada GL corre dentro de __try/__except. Si
//     algo falla (contexto raro, driver, offsets), se loguea, el efecto se
//     apaga solo y EL CLIENTE NUNCA CRASHEA por el post-procesado.
//
// FIX DEL CRASH (recursion infinita):
//   El SetCompleteHook clasico de SSeMU NO salva los bytes originales: solo
//   escribe el JMP. Antes, el hook hacia "return g_OriginalSwapBuffers(hdc)"
//   donde g_OriginalSwapBuffers era la MISMA direccion parcheada -> el JMP
//   volvia al hook -> recursion hasta desbordar la pila -> crash del main en
//   el primer frame. Ahora se copian los 5 bytes originales a un trampolin en
//   memoria ejecutable (VirtualAlloc) y se llama a ESE, que ejecuta el prologo
//   original y salta de vuelta a SwapBuffers+5.
// ============================================================================

#include "ShaderPlugin.h"
#include "Offset.h"
#include "Util.h"
#include "PostProcess.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// SwapBuffers original (gdi32) -> trampolin con el prologo original
// ---------------------------------------------------------------------------
typedef BOOL(WINAPI*SwapBuffersProc)(HDC);

static SwapBuffersProc g_OriginalSwapBuffers = 0;
static BYTE*           g_Trampoline = 0;

// ---------------------------------------------------------------------------
// Lectura SEGURA de memoria del cliente: si el offset apunta a memoria no
// mapeada (por ejemplo en un build distinto), devolvemos 0 en vez de crashear.
// ---------------------------------------------------------------------------
static bool IsReadable(DWORD addr, DWORD size)
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

static DWORD SafeReadDword(DWORD addr)
{
	if(IsReadable(addr,4) == false)
	{
		return 0;
	}

	return *(DWORD*)addr;
}

static WORD SafeReadWord(DWORD addr)
{
	if(IsReadable(addr,2) == false)
	{
		return 0;
	}

	return *(WORD*)addr;
}

// ---------------------------------------------------------------------------
// Estado del cliente (memoria, sin puente con Lua)
// ---------------------------------------------------------------------------
static bool InGame()
{
	return (SafeReadDword(MAIN_SCREEN_STATE) == 5);
}

static float ReadPlayerHP()
{
	float hp = 1.0f;

	DWORD base = SafeReadDword(MAIN_CHARACTER_STRUCT);

	if(base != 0 && IsReadable(base,0x30))
	{
		WORD life    = SafeReadWord(base+0x22);
		WORD maxLife = SafeReadWord(base+0x26);

		if(maxLife > 0)
		{
			hp = (float)life/(float)maxLife;

			if(hp < 0.0f) hp = 0.0f;
			if(hp > 1.0f) hp = 1.0f;
		}
	}

	return hp;
}

static int ReadCurrentMap()
{
	DWORD map = SafeReadDword(MAIN_CURRENT_MAP);

	// Rango ampliado: el sistema de shaders por mapa soporta hasta el mapa 137
	// (Kanturu Underground) del cliente original. > 137 = no valido.
	if(map > 137)
	{
		return -1; // mapa no valido
	}

	return (int)map;
}

// Log de diagnostico temporal: imprime el mapa leido cuando cambia, para
// verificar que el offset MAIN_CURRENT_MAP es correcto en este cliente.
static void LogCurrentMapDebug(int map)
{
	static int s_lastLoggedMap = -2;

	if(map != s_lastLoggedMap)
	{
		s_lastLoggedMap = map;
		ShaderLog("[Shader] DEBUG mapa leido = %d",map);
	}
}

// ---------------------------------------------------------------------------
// Hook de SwapBuffers: aplica el post-procesado justo antes del flip.
// Todo dentro de __try/__except: si algo falla, el filtro apaga el efecto y
// el cliente sigue vivo.
// ---------------------------------------------------------------------------
static BOOL WINAPI HookSwapBuffers(HDC hdc)
{
	if(g_OriginalSwapBuffers == 0)
	{
		return SwapBuffers(hdc);
	}

	__try
	{
		// Poll de la tecla de toggle SIEMPRE (tambien con el efecto apagado;
		// el reload de shaders al re-activar solo toca GL si hay contexto)
		PostProcess_CheckToggle();

		if(PostProcess_IsEnabled())
		{
			// Solo la ventana del cliente (el hook ve TODOS los SwapBuffers
			// del proceso, pero solo procesamos el frame del juego)
			HWND wnd = WindowFromDC(hdc);
			HWND client = (HWND)SafeReadDword(MAIN_WINDOW);

			if(client != 0 && wnd == client)
			{
				// Lazy-init con reintento: se intenta cada frame del cliente
				// hasta que haya contexto GL actual (LoadGLFunctions falla
				// sin contexto y no marca fallo permanente)
				if(PostProcess_GLCanTry())
				{
					PostProcess_LoadShader();
				}

				if(PostProcess_IsEnabled() && PostProcess_IsReady())
				{
					bool apply = true;

					if(ShaderConfig_Get().inGameOnly && InGame() == false)
					{
						apply = false;
					}

					if(apply)
					{
						int curMap = ReadCurrentMap();
						LogCurrentMapDebug(curMap);
						PostProcess_Frame(ReadPlayerHP(),curMap);
					}
				}
			}
		}
	}
	__except(PostProcess_ExceptionFilter(GetExceptionCode()))
	{
		// Nada: el filtro ya logueo y desactivo el efecto.
		// El cliente nunca crashea por el post-procesado.
	}

	return g_OriginalSwapBuffers(hdc);
}

// ---------------------------------------------------------------------------
// Mini-decoder x86: longitud de una instruccion de prologo (o -1 si es muy
// compleja -> no hookeamos). Cubre los prologos reales de SwapBuffers:
//   - Windows 10/11:  FF 25 <disp32>   = jmp dword ptr [disp32] (thunk a win32u)
//   - Hotpatch clasico: 8B FF 55 8B EC  = mov edi,edi; push ebp; mov ebp,esp
//   - Clasico:          55 8B EC 83 EC xx = push ebp; mov ebp,esp; sub esp,imm8
// ---------------------------------------------------------------------------
static int PrologueInsnLen(BYTE* p)
{
	switch(p[0])
	{
		case 0x90: return 1;                            // nop
		case 0xCC: return 1;                            // int3
		case 0x50: case 0x51: case 0x52: case 0x53:
		case 0x54: case 0x56: case 0x57: return 1;      // push reg
		case 0x58: case 0x59: case 0x5A: case 0x5B:
		case 0x5C: case 0x5E: case 0x5F: return 1;      // pop reg
		case 0x55: return 1;                            // push ebp
		case 0x5D: return 1;                            // pop ebp
		case 0x6A: return 2;                            // push imm8
		case 0x68: return 5;                            // push imm32
		case 0xEB: return 2;                            // jmp short
		case 0xE9: return 5;                            // jmp rel32 (ya hookeado)
		case 0x8B:
			if((p[1] & 0xC0) == 0xC0) return 2;         // mov reg,reg (mov edi,edi...)
			return -1;
		case 0x33:
			if((p[1] & 0xC0) == 0xC0) return 2;         // xor reg,reg
			return -1;
		case 0x83: return 3;                            // sub/add/and/or/... esp,imm8
		case 0x81: return 6;                            // sub/add/... esp,imm32
		case 0xFF:
			if(p[1] == 0x25) return 6;                  // jmp dword ptr [disp32] (thunk)
			if(p[1] == 0x15) return 6;                  // call dword ptr [disp32]
			return -1;
		default:
			return -1;
	}
}

// ---------------------------------------------------------------------------
// Instalacion del hook con TRAMPOLIN (fix del crash por recursion).
// ---------------------------------------------------------------------------
static bool InstallSwapBuffersHook()
{
	HMODULE gdi = GetModuleHandleA("gdi32.dll");

	if(gdi == 0)
	{
		ShaderLog("[%s] ERROR: gdi32.dll no esta cargada.",SHADER_PLUGIN_NAME);
		return false;
	}

	DWORD swapAddr = (DWORD)GetProcAddress(gdi,"SwapBuffers");

	if(swapAddr == 0)
	{
		ShaderLog("[%s] ERROR: no se encontro SwapBuffers en gdi32.dll.",SHADER_PLUGIN_NAME);
		return false;
	}

	BYTE* p = (BYTE*)swapAddr;

	// Si otro modulo (p.ej. main.dll o un anti-cheat) ya lo hookeo con un JMP,
	// NO parcheamos encima: nos omitimos para no romper su cadena.
	if(p[0] == 0xE9 || p[0] == 0xCC)
	{
		ShaderLog("[%s] SwapBuffers YA hookeado por otro modulo (primer byte %02X). Se omite el hook.",SHADER_PLUGIN_NAME,p[0]);
		return false;
	}

	// Copiar instrucciones COMPLETAS del prologo hasta cubrir >= 5 bytes
	// (max 12). Solo asi el trampolin es seguro: ninguna instruccion rota.
	BYTE copy[12];
	int n = 0;

	while(n < 5)
	{
		int len = PrologueInsnLen(p+n);

		if(len <= 0 || n+len > 12)
		{
			ShaderLog("[%s] Prologo de SwapBuffers no decodificable (%02X %02X %02X %02X...): se omite el hook.",
				SHADER_PLUGIN_NAME,p[0],p[1],p[2],p[3]);
			return false;
		}

		memcpy(copy+n,p+n,len);
		n += len;
	}

	// Trampolin: copia de los bytes ORIGINALES + JMP de vuelta a swapAddr+n.
	// Memoria EJECUTABLE (DEP no permite ejecutar .data).
	g_Trampoline = (BYTE*)VirtualAlloc(0,32,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);

	if(g_Trampoline == 0)
	{
		ShaderLog("[%s] ERROR: VirtualAlloc del trampolin fallo.",SHADER_PLUGIN_NAME);
		return false;
	}

	memcpy(g_Trampoline,copy,n);

	g_Trampoline[n] = 0xE9;
	*(DWORD*)(g_Trampoline+n+1) = (swapAddr+n) - (DWORD)(g_Trampoline+n+5);

	// Hook: E9 rel32 en gdi32!SwapBuffers -> HookSwapBuffers
	SetCompleteHook(0xE9,swapAddr,&HookSwapBuffers);

	g_OriginalSwapBuffers = (SwapBuffersProc)g_Trampoline;

	ShaderLog("[%s] SwapBuffers hookeado en 0x%08X (trampolin 0x%08X, %d bytes: %02X %02X %02X %02X %02X...)",
		SHADER_PLUGIN_NAME,swapAddr,(DWORD)g_Trampoline,n,
		g_Trampoline[0],g_Trampoline[1],g_Trampoline[2],g_Trampoline[3],g_Trampoline[4]);

	return true;
}

// ---------------------------------------------------------------------------
// PUNTO DE ENTRADA DEL PLUGIN (lo llama main.dll via GetProcAddress("EntryProc"))
// ---------------------------------------------------------------------------
extern "C" _declspec(dllexport) void EntryProc()
{
	// Carpeta de config/shaders/log (relativa al main.exe del cliente)
	CreateDirectoryA("Shader",0);

	ShaderLog("======================================================");
	ShaderLog("[%s] v%s",SHADER_PLUGIN_NAME,SHADER_PLUGIN_VERSION);
	ShaderLog("[%s] Proceso: %u Hilo: %u",SHADER_PLUGIN_NAME,GetCurrentProcessId(),GetCurrentThreadId());

	// Config (crea Shader.ini con valores por defecto si no existe)
	PostProcess_Init();

	// Hook de SwapBuffers (gdi32) con trampolin. Las funciones GL y los
	// shaders se cargan de forma perezosa en el primer frame del cliente
	// (cuando el contexto GL del cliente ya existe).
	if(InstallSwapBuffersHook() == false)
	{
		ShaderLog("[%s] Hook de SwapBuffers NO instalado: el post-procesado queda desactivado.",SHADER_PLUGIN_NAME);
		return;
	}

	ShaderLog("[%s] Inicializacion terminada.",SHADER_PLUGIN_NAME);
}

// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule,DWORD ul_reason_for_call,LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
	}

	return 1;
}
