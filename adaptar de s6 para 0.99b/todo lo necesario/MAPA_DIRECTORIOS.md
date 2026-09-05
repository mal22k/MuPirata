# 🗺️ MAPA DE DIRECTORIOS — ssemu (proyecto MU SSeMU S6)

> Referencia rápida de dónde vive cada cosa. Actualizado: 08-08-2026.

```
C:\Users\Administrator\Desktop\ssemu\
│
├── salidas\              ★ ÚNICO destino de DLLs COMPILADAS
│   ├── Shader.dll        (plugin post-procesado, build.bat del ShaderPlugin)
│   ├── Lua.dll           (plugin scripting, build.bat del LuaPlugin)
│   ├── LEEME.txt         (guía del directorio)
│   └── obj\              (intermedios: obj\ShaderPlugin\ y obj\LuaPlugin\)
│
├── forja\                ★ Paquete COMPLETO de la Forja (Lua, no se compila)
│   ├── LEEME_INSTALACION.md   (guía de instalación)
│   ├── Forja_UI_preview.html  (preview visual del panel)
│   ├── cliente\
│   │   ├── main.lua           (núcleo del cliente v0.5.0)
│   │   ├── config.txt
│   │   └── forge\main.lua     (panel de la forja)
│   └── servidor\
│       ├── Forge.lua          (recetas + NPC Delgado, server)
│       ├── CommandManager_linea.txt
│       ├── ScriptMain_lua_require.txt
│       └── test\forge_selftest_runner.lua
│
├── ShaderPlugin\         ★ Proyecto C++ del Shader.dll
│   ├── ShaderPlugin.vcxproj   (OutDir → ..\salidas\)
│   ├── build.bat              (compila, NO copia — el usuario copia)
│   ├── src\                   (ShaderPlugin.cpp, PostProcess.cpp, ...)
│   ├── shaders\               (post.fs / post.vs)
│   └── test\                  (harness: test_shader.exe, load_dll.exe, ...)
│
├── LuaPlugin\            ★ Proyecto C++ del Lua.dll
│   ├── LuaPlugin.vcxproj      (OutDir → ..\salidas\)
│   ├── build.bat              (compila, NO copia — el usuario copia)
│   ├── src\                   (LuaPlugin.cpp, PacketManager.cpp, ...)
│   ├── lua\                   (intérprete Lua 5.2)
│   └── test\                  (harness: test_lua.exe + prod_check.lua, forge_ui_check.lua)
│       NOTA: tests apuntan a ..\..\forja\cliente (lua_scripts fue eliminado)
│
├── MuServerS6Evercion maxima\   ★ SERVIDOR (GameServer + Data)
│   └── Data\Script\
│       ├── ScriptMain.lua          (requires: System\ScriptCore, Script\Forge...)
│       └── Script\Forge.lua        (la forja del server — == forja\servidor\Forge.lua)
│
├── respaldo\              ★ Backups (backup_shaderplugin_*.zip, output_obsoleto\)
├── SOURCE viejos\         ★ Sources históricos (referencia, NO tocar)
└── *.md                   (GUIA_ECOSISTEMA, MAPA_DIRECTORIOS, PROGRESO_LUA, ...)
```

## 📌 Reglas de oro (las que pediste)

1. **DLLs compiladas** → SOLO en `salidas\`. Los `build.bat` NO copian a ningún lado.
2. **Forja (Lua)** → SOLO en `forja\` (paquete compartible completo).
3. **El copiado a destinos lo hace el USUARIO** (nunca el asistente mezcla archivos).
4. **Destinos de copia manual**:
   - `salidas\Lua.dll` → `Package_SSeMU_S6_ENG\Lua.dll` (junto al main.exe)
   - `salidas\Shader.dll` → `Package_SSeMU_S6_ENG\Shader.dll` (+ regenerar ServerInfo.sse)
   - `forja\cliente\*` → `Package_SSeMU_S6_ENG\Lua\*`
   - `forja\servidor\Forge.lua` → `MuServerS6Evercion maxima\Data\Script\Script\Forge.lua`

## 🔁 Flujo de trabajo

```
EDITAR  →  COMPILAR (build.bat)  →  salidas\  →  COPIAR MANUAL al cliente
   ↑                                       ↓
forja\ (Lua)  ←──── sincronizar ───────────┘
```
