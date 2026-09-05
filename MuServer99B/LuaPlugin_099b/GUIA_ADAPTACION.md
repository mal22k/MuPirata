# LuaPlugin 0.99b - Guía de Adaptación desde S6 EX603

## Resumen
Este plugin porta TODAS las funciones del LuaPlugin de Season 6 EX603 al cliente 0.99b (Season 2.1.7).

## Método de Inyección
El sistema GetMainInfo ya está implementado igual que en S6:
1. main.exe 0.99b parcheado carga main.dll
2. main.dll lee ServerInfo.sse y ejecuta gProtect.CheckPluginFile()
3. CheckPluginFile hace LoadLibrary("Lua.dll") y llama a EntryProc()

**Registro:** MainInfo.ini -> [MainInfo] PluginName1=Lua.dll

## Offsets Clave 0.99b vs S6 EX603

| Función | S6 EX603 | 0.99b |
|---------|----------|-------|
| MAIN_SCREEN_STATE | 0x00E609E8 | 0x00610660 |
| MAIN_CHARACTER_STRUCT | 0x08128AC8 | 0x07B5ECCC |
| pCursorX/Y | 0x0879340C/10 | 0x081B9560/5C |
| ProtocolCore | 0x00663B20 | 0x004A3C70 |
| PROTOCOL_HOOK_OFFSET | 0x0065FD79 | 0x004A3B8B |
| RENDER_HOOK_OFFSET | 0x005B96E8 | 0x005443AF |
| RENDER_ORIGINAL | 0x005BA770 | 0x005443B4 |
| MOUSE_CLICK_OFFSET | 0x007D2920 | 0x00435680 (a confirmar) |
| CLIENT_SEND_POINTER | 0x00D227F8 | 0x005FA460 |
| CLIENT_RECV_POINTER | 0x00D227B0 | 0x005FA48C |

## Funciones Implementadas

### 1. Sistema de UI Custom
- DrawLuaUI(): Hook de render para dibujar interfaces personalizadas
- Funciones de dibujo: Textos, barras, imágenes, sprites
- Alpha blending y efectos OpenGL

### 2. Sistema de Packets
- RegisterPacketHandler(head, sub, fn): Registro de handlers por opcode
- on_packet: Handler global de packets
- Envío de packets con cifrado compatible con main.dll

### 3. Bloqueo de Mouse/UI Custom (3 Capas)
- **Capa 1:** Hook del call site de MouseClick (consumo de clicks)
- **Capa 2:** WndProc subclaseado (traga mensajes WM_LBUTTONDOWN)
- **Capa 3:** Guard de envio (bloquea packets C1:04/05/06)

### 4. Input Polling
- on_key(key, state): Detección de teclas
- on_click(button, x, y): Detección de clicks

### 5. Funciones de Personaje/Estado
- CharacterBase(): Puntero al struct del personaje
- MoneyCache: Dinero parseado de packets
- ClientState: Trade, personal shop, chaos box

### 6. Funciones de Lua Bridges
Todos los bridges del servidor 0.99b están disponibles:
- SendPacket(head, data)
- UI.DrawText, UI.DrawBar, UI.DrawImage
- UI.BlockMouse(rects)
- Character.GetMoney, Character.GetMap
- etc.

## Archivos del Proyecto

```
LuaPlugin_099b/
├── src/
│   ├── LuaPlugin.cpp       # Código principal (2987 líneas)
│   ├── LuaPlugin.h         # Headers principales
│   ├── Offset.h            # Offsets específicos 0.99b
│   ├── PacketManager.cpp/h # Gestión de packets
│   ├── ClientStateCache.h  # Cache de estados
│   ├── MoneyCache.h        # Cache de dinero
│   ├── MouseBlock.h        # Lógica de bloqueo de mouse
│   ├── Util.cpp/h          # Utilidades
│   └── stdafx.cpp/h        # Precompiled headers
├── lua/                    # Scripts Lua de ejemplo
├── LuaPlugin_099b.vcxproj  # Proyecto Visual Studio
└── build.bat               # Script de compilación
```

## Compilación

1. Abrir `LuaPlugin_099b.vcxproj` en Visual Studio
2. Configurar para Win32 (el cliente 0.99b es 32-bit)
3. Asegurar que las librerías de Lua estén linkadas
4. Compilar en Release -> genera `Lua.dll`

## Instalación

1. Copiar `Lua.dll` a la carpeta del cliente 0.99b
2. Editar `MainInfo.ini`:
   ```ini
   [MainInfo]
   PluginName1=Lua.dll
   ```
3. Ejecutar `GetMainInfo.exe` para generar `ServerInfo.sse`
4. Iniciar el cliente

## Notas Importantes

### Offsets a Verificar con Debugger
Algunos offsets pueden necesitar ajuste según el build exacto del main.exe 0.99b:
- **MOUSE_CLICK_OFFSET (0x00435680)**: Buscar la función que procesa clicks
- **MOUSE_CLICK_CALLSITE_REF (0x00435700)**: Call site que invoca MouseClick
- **pCheckWindow/pOpenWindow/pClosekWindow**: Window manager del cliente

### Bridges del Servidor
Los bridges del lado servidor están en:
`/workspace/MuServer99B/Tools/Guides/Script Lua BridgeFunctions.rtf`

Estos definen las funciones Lua disponibles desde el servidor.

### Compatibilidad con main.dll 0.99b
- El hook de protocolo usa PROTOCOL_HOOK_OFFSET (0x004A3B8B)
- El hook de render usa RENDER_HOOK_OFFSET (0x005443AF)
- Los punteros CLIENT_SEND/RECV_POINTER son sustituidos por MySend/MyRecv de main.dll

## Próximos Pasos

1. **Verificar offsets con debugger** (OllyDbg/x64dbg):
   - Localizar MOUSE_CLICK_OFFSET exacto
   - Confirmar RENDER_ORIGINAL
   - Validar window manager functions

2. **Probar el plugin**:
   - Compilar y cargar en el cliente 0.99b
   - Verificar que los hooks se instalan correctamente
   - Testear cada función desde Lua

3. **Agregar funciones adicionales**:
   - Si hay funciones específicas de 0.99b que no están en S6
   - Optimizaciones propias de la versión

## Referencias

- Source original 0.99b: `/workspace/Source/Source/Emulator 0.99 (2.1.7)/Main/`
- LuaPlugin S6: `/workspace/adaptar de s6 para 0.99b/todo lo necesario/LuaPlugin/`
- Bridges servidor: `/workspace/MuServer99B/Tools/Guides/Script Lua BridgeFunctions.rtf`
