#pragma once

// ============================================================================
// Offset.h - Direcciones del cliente main.exe 0.99b (Season 2.1.7)
// Fuente: /workspace/Source/Source/Emulator 0.99 (2.1.7)/Main/Offset.h
// ADAPTADO PARA LUAPLUGIN - SOLO OFFSETS VERIFICADOS
// ============================================================================

// --- Punteros/estado globales del cliente ---
#define MAIN_WINDOW             0x05688160   // HWND del cliente
#define MAIN_SCREEN_STATE       0x00610660   // Estado de pantalla (5 = dentro del juego)
#define MAIN_CHARACTER_STRUCT   0x07B5ECCC   // Puntero al struct del personaje
#define MAIN_VIEWPORT_STRUCT    0x07924F08   // Estructura del viewport
#define MAIN_PACKET_SERIAL      0x0568C543   // Serial para packets
#define MAIN_ACTIVE_SOCKET      0x05688310   // Socket activo
#define MAIN_CURRENT_MAP        0x006081C0   // Mapa actual
#define MAIN_RESOLUTION_X       0x006105F8   // Resolución X
#define MAIN_RESOLUTION_Y       0x006105FC   // Resolución Y

// --- Cursor en pantalla ---
#define pCursorX                (*(int*)0x081B9560)
#define pCursorY                (*(int*)0x081B955C)

// --- Texto y dibujado ---
#define DrawInterfaceText       ((void(*)(int,int,char*))0x005B4210)
#define pDrawText               ((char*(__cdecl*)(int,int,char*,int,int,int))0x00500100)
#define pDrawBarForm            ((void(__cdecl*)(float,float,float,float,float,int))0x005B2460)
#define pDrawBigText            ((void(*)(float,float,DWORD,float,float))0x005B22B0)
#define pDrawMessage            ((int(__cdecl*)(char*,int))0x00500570)
#define pRenderTipText          ((void(*)(int,int,char*))0x005002A0)

// --- Imágenes y blend ---
#define pDrawImage              ((void(*)(DWORD,float,float,float,float,float,float,float,float,int,int,GLfloat))0x005B2520)
#define pLoadImageJPG           ((int(*)(char*,int,GLint,GLint,int,int))0x005CBE50)
#define pLoadImageTGA           ((int(*)(char*,int,GLint,GLint,int,int))0x005CC2E0)
#define EnableAlphaBlend        ((void(*)())0x005B1450)
#define EnableAlphaBlend2       ((void(*)())0x005B1550)
#define EnableAlphaTest         ((void(*)(bool))0x005B13C0)
#define DisableAlphaBlend       ((void(*)())0x005B1340)

// --- Protocolo ---
#define ProtocolCore            ((BOOL(*)(DWORD,BYTE*,DWORD,DWORD))0x004A3C70)
#define MAIN_HOOK_RECV          0x005FA48C   // Hook de recepción
#define MAIN_HOOK_SEND          0x005FA460   // Hook de envío

// --- Render hook ---
#define RENDER_HOOK_OFFSET      0x005443AF   // Call en el loop de render (VERIFICAR CON DEBUGGER)
#define RENDER_ORIGINAL         0x005BA770   // Función original (ajustar si difiere)

// --- Mouse ---
#define pMouseLButton           0x081B95A8
#define pMouseRButton           0x081B9590
#define pCheckMouseIn           ((BOOL(__cdecl*)(int,int,int,int,int))0x004354C0)

// --- Utilidades ---
#define pGetPosFromAngle        ((void(__cdecl*)(float*,int*,int*))0x005B0F70)
#define pTransformPosition      ((int(__thiscall*)(DWORD,DWORD,float*,float*,bool))0x004AB360)
#define pCreateParticle         ((int(__cdecl*)(DWORD,float*,DWORD,float*,DWORD,float,DWORD))0x004F0150)
#define pCreateEffect           ((void(__cdecl*)(int,float*,DWORD,float*,int,DWORD,short,BYTE,float,BYTE,float*))0x004D23C0)
#define pPlayBuffer             ((int(__cdecl*)(int,int,int))0x00414480)

// --- Items ---
#define ITEM_BASE_MODEL         441
#define pLoadItemModel          ((void(*)(int,char*,char*,int))0x005A2710)
#define pLoadItemTexture        ((void(*)(int,char*,int,int,int))0x005A22D0)
#define pGetItemName(x)         ((char*)(*(DWORD*)(0x0773D118)+0x4C*x))

// --- Fuentes ---
#define pFontHDC                *(HDC*)0x05688124
#define pFontNormal             *(HFONT*)0x05688170
#define pFontBold               *(HFONT*)0x05688174
#define pFontBig                *(HFONT*)0x05688178

// --- Macros útiles ---
#define GET_ITEM(x,y)           (((x)*32)+(y))
#define GET_ITEM_MODEL(x,y)     ((((x)*32)+(y))+ITEM_BASE_MODEL)
#define GET_MAX_WORD_VALUE(x)   (((x)>65000)?65000:((WORD)(x)))

// NOTA IMPORTANTE: Los siguientes offsets necesitan verificación con debugger
// El bloqueo de mouse se hace via pCheckMouseIn en 0.99b
#define MOUSE_CLICK_OFFSET      0x004354C0   // Alternativa: usar pCheckMouseIn directamente
