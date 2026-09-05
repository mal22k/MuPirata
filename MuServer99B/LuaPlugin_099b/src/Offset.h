#pragma once

// ============================================================================
// Offset.h - Direcciones del cliente main.exe 0.99b (Season 2.1.7)
// Basado en: Source/Source/Emulator 0.99 (2.1.7)/Main/Offset.h
// Adaptado para LuaPlugin con las mismas funciones que S6 EX603
// ============================================================================

// --- Punteros/estado globales del cliente ---
#define MAIN_WINDOW             0x05688160   // HWND del cliente (puntero)
#define MAIN_CONNECTION_STATUS  0x0568C548   // Estado de conexion
#define MAIN_SCREEN_STATE       0x00610660   // 5 = dentro del juego
#define MAIN_CHARACTER_STRUCT   0x07B5ECCC   // puntero al struct del personaje
#define MAIN_VIEWPORT_STRUCT    0x07924F08   // viewport de objetos
#define MAIN_PACKET_SERIAL      0x0568C543   // serial para packets C3/C4
#define MAIN_FONT_SIZE          0x07C09B58   // tamaño de fuente
#define MAIN_RESOLUTION         0x05687F70   // resolucion base
#define MAIN_RESOLUTION_X       0x006105F8   // ancho de pantalla
#define MAIN_RESOLUTION_Y       0x006105FC   // alto de pantalla
#define MAIN_PARTY_MEMBER_COUNT 0x07D3EA48   // cantidad de miembros del party
#define MAIN_CURRENT_MAP        0x006081C0   // mapa actual
#define MAIN_ACTIVE_SOCKET      0x05688310   // socket activo

// --- Cursor en pantalla ---
#define pCursorX                (*(int*)0x081B9560)
#define pCursorY                (*(int*)0x081B955C)

// --- Texto y dibujo basico ---
#define DrawInterfaceText       ((void(*)(int,int,char*))0x005B4210)
#define pDrawText               ((char*(__cdecl*)(int,int,char*,int,int,int))0x00500100)
#define pDrawMessage            ((int(__cdecl*)(char*,int))0x00500570)
#define pDrawBarForm            ((void(__cdecl*)(float,float,float,float,float,int))0x005B2460)
#define pDrawBigText            ((void(*)(float,float,DWORD,float,float))0x005B22B0)
#define pDrawImage              ((void(*)(DWORD,float,float,float,float,float,float,float,float,int,int,GLfloat))0x005B2520)
#define pRenderTipText          ((void(*)(int,int,char*))0x005002A0)

// --- Primitivas graficas (OpenGL) ---
#define pLoadImageJPG           ((int(*)(char*,int,GLint,GLint,int,int))0x005CBE50)
#define pLoadImageTGA           ((int(*)(char*,int,GLint,GLint,int,int))0x005CC2E0)
#define pRenderBitmapRotate     ((void(*)(int,float,float,float,float,float))0x005B2630)
#define pBeginBitmap            ((void(*)())0x005B2370)

// --- Alpha blending y efectos ---
#define EnableAlphaBlend        ((void(*)())0x005B1450)
#define EnableAlphaBlend2       ((void(*)())0x005B1550)
#define EnableAlphaBlendMinus   ((void(*)())0x005B14D0)
#define EnableAlphaTest         ((void(*)(bool))0x005B13C0)
#define DisableAlphaBlend       ((void(*)())0x005B1340)
#define EnableLightMap          ((void(*)())0x005B1650)

// --- Window manager del cliente (ventanas nativas) ---
// NOTA: En 0.99b NO existe el sistema CWindows de S6
// El manejo de ventanas es hardcoded, usamos pCheckMouseIn como alternativa
#define pCheckMouseIn           ((BOOL(__cdecl*)(int,int,int,int,int))0x004354C0)
// pCheckWindow/pOpenWindow/pCloseWindow NO EXISTEN en 0.99b - usar pCheckMouseIn en su lugar

// --- Protocolo ---
// ProtocolCore: dispatcher original de packets del cliente
// PROTOCOL_HOOK_OFFSET: punto donde main.dll instala su hook
#define ProtocolCore            ((BOOL(*)(DWORD,BYTE*,DWORD,DWORD))0x004A3C70)
#define PROTOCOL_HOOK_OFFSET    0x004A3B8B   // call que main.dll parchea en Main.cpp linea 173

// --- Punteros de red del cliente ---
// main.dll sustituye estos punteros por MySend/MyRecv (HackCheck.cpp)
#define CLIENT_SEND_POINTER     0x005FA460   // Hook send (MAIN_HOOK_SEND)
#define CLIENT_RECV_POINTER     0x005FA48C   // Hook recv (MAIN_HOOK_RECV)

// --- Render hook: punto del loop de dibujo + funcion original ---
// En 0.99b, el hook de health bar se instala en Main.cpp linea 171
#define RENDER_HOOK_OFFSET      0x005443AF   // call que dibuja barras de vida (por frame)
#define RENDER_ORIGINAL         0x005443B4   // funcion original de barras de vida

// --- MouseClick del juego ---
// Funcion que procesa clicks en 0.99b - A LOCALIZAR con debugger si es necesario
// Por ahora usamos una direccion placeholder basada en el patron del 0.99b
#define MOUSE_CLICK_OFFSET      0x00435680   // Funcion MouseClick (a confirmar)
#define MOUSE_CLICK_CALLSITE_REF 0x00435700  // Call site que invoca MouseClick

// --- Funciones de camara y 3D ---
#define pTransformPosition      ((int(__thiscall*)(DWORD,DWORD,float*,float*,bool))0x004AB360)
#define pCreateSprite           ((int(*)(int,float*,float,float*,DWORD,float,int))0x004F63D0)
#define pCreateParticle         ((int(__cdecl*)(DWORD,float*,DWORD,float*,DWORD,float,DWORD))0x004F0150)
#define pCreateEffect           ((void(__cdecl*)(int,float*,DWORD,float*,int,DWORD,short,BYTE,float,BYTE,float*))0x004D23C0)
#define pCameraPosition         (GLfloat*)0x081B95B8
#define pCameraMatrix           (LPVOID*)0x081B9424

// --- Creacion de objetos ---
#define pCreateMonster          ((DWORD(*)(int,int,int,int))0x0041D5D0)
#define pCreateCharacter        ((DWORD(*)(int,int,int,int,float))0x004CD360)
#define pSetCharacterScale      ((void(*)(DWORD))0x004CD410)
#define pLoadItemModel          ((void(*)(int,char*,char*,int))0x005A2710)
#define pLoadItemTexture        ((void(*)(int,char*,int,int,int))0x005A22D0)
#define pRenderItem3D           ((void(*)(float,float,float,float,int,int,int,int,int))0x00577050)

// --- Fuentes ---
#define pFontHDC                *(HDC*)0x05688124
#define pFontNormal             *(HFONT*)0x05688170
#define pFontBold               *(HFONT*)0x05688174
#define pFontBig                *(HFONT*)0x05688178

// --- Utilidades ---
#define pGetPosFromAngle        ((void(__cdecl*)(float*,int*,int*))0x005B0F70)
#define pCheckMouseIn           ((BOOL(__cdecl*)(int,int,int,int,int))0x004354C0)
#define pViewportAddress        *(DWORD*)(0x07924F00)
#define pMouseLButton           0x081B95A8
#define pMouseRButton           0x081B9590

// --- Macros de utilidad ---
#define GET_ITEM(x,y)           (((x)*32)+(y))
#define GET_ITEM_MODEL(x,y)     ((((x)*32)+(y))+441)
#define GET_MAX_WORD_VALUE(x)   (((x)>65000)?65000:((WORD)(x)))

// --- Encrypt/Decrypt del cliente 0.99b ---
#define STRUCT_DECRYPT          ((void(__thiscall*)(void*,void*))0x004A7520)((void*)0x05687D00,*(void**)0x07B5ECD4);
#define STRUCT_ENCRYPT          ((void(__thiscall*)(void*,void*))0x0040A790)((void*)0x05687D00,*(void**)0x07B5ECD4);
