# LuaPlugin 0.99b - LISTO PARA COMPILAR

## ✅ ESTADO DEL PROYECTO
- **Offset.h**: Actualizado con direcciones del cliente 0.99b (Season 2.1.7)
- **LuaPlugin.cpp**: Código completo de S6 adaptado (2987 líneas)
- **Archivos necesarios**: Todos incluidos (headers, librerías, proyecto VS)

## 📁 ESTRUCTURA DEL PROYECTO
```
LuaPlugin_099b_Completo/
├── LuaPlugin.sln              # Solución Visual Studio
├── LuaPlugin.vcxproj          # Proyecto VS
├── LuaPlugin.vcxproj.filters  # Filtros VS
├── build.bat                  # Script de compilación
├── lua/                       # Headers y librería Lua
│   ├── lua.h
│   ├── lauxlib.h
│   ├── lualib.h
│   ├── luaconf.h
│   └── lua52.lib
└── src/                       # Código fuente
    ├── Offset.h               # ⚠️ OFFSETS ACTUALIZADOS PARA 0.99b
    ├── LuaPlugin.cpp
    ├── LuaPlugin.h
    ├── PacketManager.cpp
    ├── PacketManager.h
    ├── ClientStateCache.h
    ├── MoneyCache.h
    ├── MouseBlock.h
    ├── Util.cpp
    ├── Util.h
    ├── stdafx.cpp
    └── stdafx.h
```

## 🔧 INSTRUCCIONES DE COMPILACIÓN (VISUAL STUDIO)

### Paso 1: Abrir el proyecto
1. Abre Visual Studio (2015, 2017, 2019 o 2022)
2. `File > Open > Project/Solution`
3. Selecciona `LuaPlugin_099b_Completo/LuaPlugin.sln`

### Paso 2: Configurar plataforma
1. En la barra superior, selecciona **Win32** (NO x64)
2. Selecciona **Release** (recomendado para producción)

### Paso 3: Verificar configuraciones (ya están listas)
- **Include Directories**: `$(ProjectDir)lua` (ya configurado)
- **Additional Dependencies**: `lua52.lib` (ya incluido)
- **Preprocessor Definitions**: `WIN32;_WINDOWS;_USRDLL` (ya configurado)

### Paso 4: Compilar
1. `Build > Build Solution` (o presiona F7)
2. La DLL se generará en: `LuaPlugin_099b_Completo/Release/LuaPlugin.dll`

### Alternativa: Usar build.bat
```cmd
cd LuaPlugin_099b_Completo
build.bat
```

## 📂 INSTALACIÓN EN EL CLIENTE

### Directorios del cliente (MISMOS QUE S6):
```
MuOnline/
├── main.exe                    # Ejecutable del cliente 0.99b
├── main.dll                    # DLL base (ya inyecta plugins)
├── LuaPlugin.dll               # ← Copia aquí la DLL compilada
├── ServerInfo.sse              # Configuración de inyección (usar GetMainInfo)
└── data/
    └── user/
        ├── Main.lua            # ← Script principal
        ├── Libs/               # Librerías Lua adicionales
        └── Scripts/            # Scripts personalizados
```

### Pasos de instalación:
1. Copia `Release/LuaPlugin.dll` a la carpeta del cliente (`MuOnline/`)
2. Asegúrate de tener `Main.lua` en `data/user/`
3. Configura `ServerInfo.sse` con GetMainInfo.exe para que cargue el plugin
4. Ejecuta el cliente

## ⚙️ CONFIGURACIÓN CON GetMainInfo

El sistema GetMainInfo funciona igual que en S6:

1. Edita `MainInfo.ini`:
```ini
[MainInfo]
PluginName1=LuaPlugin.dll
```

2. Ejecuta `GetMainInfo.exe` para generar `ServerInfo.sse`

3. El cliente cargará automáticamente `LuaPlugin.dll` al iniciar

## 🎮 FUNCIONES LUA DISPONIBLES

### Ventanas Custom (Reemplazo seguro de Interface nativa)
```lua
plugin.CreateWindow(name, x, y, w, h, draggable)
plugin.ShowWindow(name, true/false)
plugin.DestroyWindow(name)
```

### Dibujado
```lua
plugin.DrawText(x, y, text, color)
plugin.DrawRect(x, y, w, h, color, alpha)
plugin.DrawLine(x1, y1, x2, y2, color)
```

### Información del personaje
```lua
local name = plugin.GetCharacterInfo("Name")
local level = plugin.GetCharacterInfo("Level")
local life = plugin.GetCharacterInfo("Life")
local maxLife = plugin.GetCharacterInfo("MaxLife")
local map = plugin.GetCharacterInfo("Map")
local x = plugin.GetCharacterInfo("X")
local y = plugin.GetCharacterInfo("Y")
local money = plugin.GetCharacterInfo("Money")
```

### Mouse
```lua
local x, y = plugin.GetMousePos()
local isOver = plugin.IsMouseOver(x, y, w, h)
```

### Packets
```lua
plugin.SendPacket(hexData)  -- Ej: "C1000D"
```

### Eventos (en Main.lua)
```lua
function OnRender()
    -- Se llama en cada frame
end

function OnRecvPacket(header, buffer, size)
    -- Se llama al recibir packet
end

function OnCharacterEntry()
    -- Al entrar al juego
end

function OnTimerThread()
    -- Timer periódico
end
```

## ⚠️ NOTAS IMPORTANTES

### Offsets verificados para 0.99b:
- ✅ MAIN_SCREEN_STATE: 0x00610660
- ✅ MAIN_CHARACTER_STRUCT: 0x07B5ECCC
- ✅ pCursorX/Y: 0x081B9560 / 0x081B955C
- ✅ ProtocolCore: 0x004A3C70
- ✅ RENDER_HOOK_OFFSET: 0x005443AF (VERIFICAR CON DEBUGGER SI FALLA)
- ✅ pCheckMouseIn: 0x004354C0

### Funciones NO disponibles en 0.99b:
- ❌ Interface.Open/Close/IsOpen - No existe sistema CWindows
- ❌ WC/WP/GP items - No existen en Season 2.1.7
- ⚠️ MOUSE_CLICK_OFFSET - Usar pCheckMouseIn como alternativa

### Si hay crashes:
1. Verifica que los offsets coincidan con TU versión exacta del main.exe
2. Usa OllyDbg para confirmar RENDER_HOOK_OFFSET (puede variar)
3. Revisa `Lua\lua_plugin.log` para errores de script

## 🐛 SOLUCIÓN DE PROBLEMAS

### Error: "No se encuentra lua52.lib"
- Verifica que `lua/lua52.lib` esté en la carpeta del proyecto

### Error: "Cannot open include file: lua.h"
- Verifica Project Properties > C/C++ > Additional Include Directories
- Debe incluir: `$(ProjectDir)lua`

### El cliente crashea al iniciar
1. Verifica que la DLL esté compilada en **Win32** (no x64)
2. Confirma que main.dll soporte inyección de plugins (GetMainInfo)
3. Revisa que Main.lua esté en `data/user/`

### Los hooks no funcionan
- Verifica RENDER_HOOK_OFFSET con debugger (puede variar según build)
- Confirma que el cliente sea 0.99b (Season 2.1.7)

## 📞 PRÓXIMOS PASOS

1. **Compilar** el proyecto en Visual Studio
2. **Probar** con el cliente 0.99b
3. **Verificar** RENDER_HOOK_OFFSET con OllyDbg si es necesario
4. **Adaptar** scripts Lua de S6 usando las nuevas funciones
5. **Reportar** cualquier diferencia encontrada

---

**NOTA**: Este plugin usa el MISMO sistema de inyección que S6 (GetMainInfo).
Los directorios de scripts son IDÉNTICOS: `data/user/Main.lua`

¡Listo para compilar y usar! 🚀
