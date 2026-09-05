#pragma once

// ============================================================================
// Offset.h - Direcciones del cliente main.exe EX603 (Season 6 Episode 3)
// Referencia: SOURCE viejos/Source/Main_EX603/Main/Offset.h
//
// IMPORTANTE: estas direcciones son del build EX603 original. Si tu main.exe
// es de otra version/season, hay que re-localizar cada direccion (las tablas
// AddressTable*.bin + Offset.h del proyecto Main del emulador son la referencia
// autoritativa para tu build).
// ============================================================================

// --- Punteros/estado globales del cliente ---
#define MAIN_WINDOW             0x00E8C578   // HWND del cliente (puntero)
#define MAIN_SCREEN_STATE       0x00E609E8   // 5 = dentro del juego
#define MAIN_CHARACTER_STRUCT   0x08128AC8   // puntero al struct del personaje
#define MAIN_PACKET_SERIAL      0x08793700   // serial para packets C3/C4
#define MAIN_ACTIVE_SOCKET      0x08793750   // socket activo (+0x0C = SOCKET)
#define MAIN_CURRENT_MAP        0x00E61E18   // mapa actual
#define MAIN_RESOLUTION_X       0x00E61E58
#define MAIN_RESOLUTION_Y       0x00E61E5C

// --- Cursor en pantalla ---
#define pCursorX                (*(int*)0x0879340C)
#define pCursorY                (*(int*)0x08793410)

// --- Texto ---
#define pTextThis               ((LPVOID(*)())0x0041FE10)
#define pSetTextColor           ((void(__thiscall*)(LPVOID,BYTE,BYTE,BYTE,BYTE))0x00420040)
#define pDrawText               ((int(__thiscall*)(LPVOID,int,int,char*,int,int,int*,int))0x00420150)
#define pDrawMessage            ((int(__cdecl*)(LPCSTR,int))0x00597630)

// --- Primitivas graficas (OpenGL) ---
#define pDrawToolTip            ((int(__cdecl*)(int,int,LPCSTR))0x00597220)   // tooltip inmediato
#define pSetBlend               ((void(__cdecl*)(BYTE))0x00635FD0)
#define pGLSwitchBlend          ((void(__cdecl*)())0x00636070)
#define pGLSwitch               ((void(__cdecl*)())0x00635F50)
#define pDrawBarForm            ((void(__cdecl*)(float,float,float,float,float,int))0x006378A0)
#define pLoadImage              ((int(__cdecl*)(char*,int,int,int,int,int))0x00772330)
#define pDrawImage              ((void(*)(DWORD,float,float,float,float,float,float,float,float,int,int,GLfloat))0x00637C60)

// --- Window manager del cliente (ventanas nativas) ---
#define pWindowThis             ((LPVOID(*)())0x00861110)
#define pCheckWindow            ((bool(__thiscall*)(LPVOID,int))0x0085EC20)
#define pOpenWindow             ((int(__thiscall*)(LPVOID,int))0x0085EC50)
#define pClosekWindow           ((int(__thiscall*)(LPVOID,int))0x0085F9A0)

// --- Protocolo ---
// ProtocolCore: dispatcher original de packets del cliente (0x00663B20).
// PROTOCOL_HOOK_OFFSET: call del dispatcher que main.dll ya redirige a su
// ProtocolCoreEx. El plugin encadena: LuaProtocolCoreEx -> ProtocolCoreEx de
// main.dll -> ProtocolCore (el cliente nunca se queda sin dispatcher).
#define ProtocolCore            ((BOOL(*)(DWORD,BYTE*,DWORD,DWORD))0x00663B20)
#define PROTOCOL_HOOK_OFFSET    0x0065FD79

// --- Punteros de red del cliente ---
// main.dll sustituye estos punteros por MySend/MyRecv (HackCheck.cpp), que
// aplican el cifrado de stream (EncryptData/DecryptData) y llaman al send/recv
// real. Usar CLIENT_SEND_POINTER para enviar packets con el mismo canal que
// main.dll (DataSend).
#define CLIENT_SEND_POINTER     0x00D227F8
#define CLIENT_RECV_POINTER     0x00D227B0   // reservado para el hook de recepcion (on_packet)

// --- Render hook: punto del loop de dibujo + funcion original ---
#define RENDER_HOOK_OFFSET      0x005B96E8   // call que dibuja barras de vida (por frame)
#define RENDER_ORIGINAL         0x005BA770   // funcion original de barras de vida

// --- MouseClick del juego (patron HelperMouseClick del Main_EX603) ---
// El source viejo (Common.cpp) la llama como ((char(__thiscall*)(char*))0x007D2920)(This)
// y la usa para procesar clicks; si la ventana del helper estaba abierta
// devolvia 0 (click consumido). Nosotros la hookeamos para consumir clicks
// que caen sobre rectangulos de UI custom registrados desde Lua (UI.BlockMouse).
#define MOUSE_CLICK_OFFSET      0x007D2920   // funcion MouseClick (validada en .text del main.exe)
