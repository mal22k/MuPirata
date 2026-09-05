#pragma once

#define PLUGIN_NAME       "LuaPlugin"
#define PLUGIN_VERSION    "0.3.5"

#define LUA_SCRIPT_PATH   "Lua\\main.lua"
#define LUA_LOG_PATH      "Lua\\lua_plugin.log"

// Carga las claves de packets (Data\Enc1.dat / Data\Dec2.dat)
void InitPacketManager();

// Envia un packet al servidor activo (replica del DataSend de main.dll)
bool SendPacket(BYTE* lpMsg, DWORD size);

// Hook de recepcion: LuaProtocolCoreEx + encadenado al dispatcher de main.dll
void InstallPacketHook();

// Inicializa el estado Lua, registra las funciones del cliente y carga el script
void InitLua();

// Hook de render instalado en RENDER_HOOK_OFFSET (con chaining al hook previo)
void DrawLuaUI();

// Escribe una linea en Lua\lua_plugin.log (log de depuracion)
void LuaLog(const char* fmt, ...);
