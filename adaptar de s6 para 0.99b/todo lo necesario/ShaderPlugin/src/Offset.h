#pragma once

// ============================================================================
// Offset.h - Direcciones del cliente main.exe EX603 (Season 6 Episode 3)
// Mismas direcciones que usa LuaPlugin/src/Offset.h (verificadas en el source
// del Main_EX603). El plugin de shaders solo necesita 4:
//   - HWND de la ventana (para filtrar el SwapBuffers)
//   - ScreenState (5 = dentro del juego)
//   - Struct del personaje (+0x22 Life / +0x26 MaxLife) para el efecto HP bajo
//   - Mapa actual (para el clima por mapa de Shader.ini)
// ============================================================================

#define MAIN_WINDOW             0x00E8C578   // HWND del cliente (puntero)
#define MAIN_SCREEN_STATE       0x00E609E8   // 5 = dentro del juego
#define MAIN_CHARACTER_STRUCT   0x08128AC8   // puntero al struct del personaje
#define MAIN_CURRENT_MAP        0x00E61E18   // mapa actual
