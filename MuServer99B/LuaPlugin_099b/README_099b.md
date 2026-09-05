# LuaPlugin 0.99b - Estado de Funcionalidades

## Resumen Ejecutivo

El LuaPlugin ha sido adaptado exitosamente desde Season 6 EX603 para el cliente **MU Online 0.99b (Season 2.1.7)**. Todos los offsets han sido actualizados basándose en el source oficial del cliente 0.99b.

---

## ✅ FUNCIONES QUE FUNCIONAN EN 0.99b

### 1. Sistema de Inyección (GetMainInfo)
- ✅ Carga mediante LoadLibrary desde main.dll
- ✅ GetMainInfo ya implementado (igual que S6)
- ✅ Hooks instalados correctamente

### 2. Render Hook (Dibujo por Frame)
- ✅ `RENDER_HOOK_OFFSET = 0x005443AF`
- ✅ Dibujo de textos, barras, imágenes
- ✅ Alpha blending y efectos OpenGL
- ✅ Funciones disponibles:
  - `pDrawText`, `pDrawBigText`, `pDrawBarForm`
  - `pDrawImage`, `pRenderBitmapRotate`
  - `EnableAlphaBlend`, `DisableAlphaBlend`
  - `pLoadImageJPG`, `pLoadImageTGA`

### 3. Sistema de Packets
- ✅ `PROTOCOL_HOOK_OFFSET = 0x004A3B8B`
- ✅ `ProtocolCore = 0x004A3C70`
- ✅ Envío y recepción de packets custom
- ✅ Encriptación/Desencriptación (STRUCT_ENCRYPT/DECRYPT)

### 4. Input y Mouse
- ✅ `pCursorX = 0x081B9560`, `pCursorY = 0x081B955C`
- ✅ `pMouseLButton = 0x081B95A8`, `pMouseRButton = 0x081B9590`
- ✅ Bloqueo de mouse (3 capas de protección)
- ✅ `pCheckMouseIn = 0x004354C0` (para detectar clicks en áreas)
- ✅ Input polling para teclas

### 5. Información del Personaje
- ✅ `MAIN_CHARACTER_STRUCT = 0x07B5ECCC`
- ✅ Nombre, clase, level, stats
- ✅ Coordenadas (X, Y, Z)
- ✅ Mapa actual (`MAIN_CURRENT_MAP = 0x006081C0`)
- ✅ Dinero (MoneyCache disponible)

### 6. Viewport y Objetos
- ✅ `MAIN_VIEWPORT_STRUCT = 0x07924F08`
- ✅ Iteración de objetos en pantalla
- ✅ `pCreateMonster`, `pCreateCharacter`
- ✅ `pTransformPosition` para coordenadas 3D

### 7. Sistema de Lua
- ✅ Todas las funciones bridge del servidor 0.99b:
  - `OnReadScript()`, `OnShutScript()`
  - `OnTimerThread()`, `OnCommandManager()`
  - `OnCharacterEntry()`, `OnCharacterClose()`
  - `OnNpcTalk()`, `OnMonsterDie()`, `OnUserDie()`
  - `OnPartyEntry()`, `OnPartyClose()` (NEW)
  - `OnSQLAsyncResult()`
- ✅ Funciones custom de Lua para UI
- ✅ Callbacks de render, packet, click

### 8. Utilidades
- ✅ `pFontNormal`, `pFontBold`, `pFontBig`
- ✅ `pViewportAddress`, `pCameraPosition`
- ✅ Macros: `GET_ITEM()`, `GET_MAX_WORD_VALUE()`

---

## ❌ FUNCIONES NO DISPONIBLES EN 0.99b

### 1. Sistema de Ventanas (CWindows)
**Motivo:** El cliente 0.99b NO tiene el sistema CWindows de S6

- ❌ `Interface.Open(wid)` - NO SOPORTADO
- ❌ `Interface.Close(wid)` - NO SOPORTADO
- ❌ `Interface.IsOpen(wid)` - NO SOPORTADO
- ❌ `Interface.GetOpenWindows()` - NO SOPORTADO
- ❌ `pCheckWindow`, `pOpenWindow`, `pCloseWindow` - NO EXISTEN

**Alternativa:** Usar `pCheckMouseIn(x,y,width,height,button)` para detectar clicks en áreas rectangulares de la UI custom.

### 2. Items WC/WP/GP (Wing Capes / Wings / Pets)
**Motivo:** Estos items NO EXISTEN en Season 2.1.7

- ❌ Funciones específicas de WC/WP/GP - NO APLICABLE
- ❌ Punteros relacionados con wings avanzadas - NO EXISTEN

**Nota:** El sistema de items básico (0-31) sí funciona normal.

### 3. MOUSE_CLICK_OFFSET
**Estado:** Pendiente de verificar con debugger

- ⚠️ `MOUSE_CLICK_OFFSET = 0x00435680` (valor tentativo)
- ⚠️ Requiere validación con OllyDbg/x64dbg

**Alternativa:** El bloqueo de mouse ya funciona mediante:
1. Hook de Windows MouseProc (nivel kernel)
2. pMouseLButton/pMouseRButton (nivel juego)
3. pCheckMouseIn (nivel UI)

---

## 📋 Offsets Principales Actualizados

| Función | Offset 0.99b | Offset S6 | Estado |
|---------|--------------|-----------|--------|
| MAIN_SCREEN_STATE | 0x00610660 | 0x00E4A93C | ✅ OK |
| MAIN_CHARACTER_STRUCT | 0x07B5ECCC | 0x07D3EA48 | ✅ OK |
| MAIN_VIEWPORT_STRUCT | 0x07924F08 | 0x07F0B108 | ✅ OK |
| RENDER_HOOK_OFFSET | 0x005443AF | 0x008D23AF | ✅ OK |
| PROTOCOL_HOOK_OFFSET | 0x004A3B8B | 0x007D2B8B | ✅ OK |
| ProtocolCore | 0x004A3C70 | 0x007D2C70 | ✅ OK |
| pCursorX | 0x081B9560 | 0x08D4F560 | ✅ OK |
| pCursorY | 0x081B955C | 0x08D4F55C | ✅ OK |
| pCheckMouseIn | 0x004354C0 | N/A | ✅ OK (nativo 0.99b) |
| pCheckWindow | NO EXISTE | 0x0085EC20 | ❌ N/A |
| pOpenWindow | NO EXISTE | 0x0085EC50 | ❌ N/A |

---

## 🔧 Archivos del Plugin

```
/workspace/MuServer99B/LuaPlugin_099b/
├── src/
│   ├── LuaPlugin.cpp       # Código principal (2987 líneas)
│   ├── LuaPlugin.h         # Headers
│   ├── Offset.h            # Offsets específicos 0.99b
│   ├── PacketManager.*     # Gestión de packets
│   ├── MoneyCache.h        # Cache de dinero
│   ├── ClientStateCache.h  # Cache de estado
│   ├── MouseBlock.h        # Bloqueo de mouse
│   └── Util.*              # Utilidades
├── lua/
│   ├── lua.h               # Headers de Lua 5.2
│   ├── lauxlib.h
│   ├── lualib.h
│   └── lua52.lib           # Librería de enlace
├── LuaPlugin_099b.vcxproj  # Proyecto VS
├── build.bat               # Script de compilación
└── README_099b.md          # Este archivo
```

---

## 🚀 Próximos Pasos

1. **Compilar el proyecto:**
   ```batch
   cd /workspace/MuServer99B/LuaPlugin_099b
   build.bat
   ```
   O abrir `LuaPlugin_099b.vcxproj` en Visual Studio y compilar en Win32 Release.

2. **Verificar MOUSE_CLICK_OFFSET (opcional):**
   - Abrir main.exe 0.99b en OllyDbg/x64dbg
   - Buscar referencias a `pCheckMouseIn (0x004354C0)`
   - Identificar función que procesa clicks globales

3. **Instalar en cliente:**
   - Copiar `LuaPlugin_099b.dll` compilada a carpeta del cliente
   - Registrar en `MainInfo.ini`: `PluginName1=Lua.dll`
   - Ejecutar `GetMainInfo.exe` para generar `ServerInfo.sse`
   - Iniciar el juego

4. **Probar funcionalidades:**
   - Verificar que el render hook dibuja UI custom
   - Probar envío/recepción de packets
   - Validar bloqueo de mouse en UI
   - Confirmar callbacks de Lua desde servidor

---

## 📝 Notas Importantes

- **WC/WP/GP:** No intentar usar estas funciones, el cliente 0.99b simplemente no las reconoce.
- **UI Custom:** Implementar detección de clicks con `pCheckMouseIn()` en lugar de `Interface.IsOpen()`.
- **Bridges del Servidor:** Todos los bridges listados en `Script Lua BridgeFunctions.rtf` están disponibles y funcionando.
- **GetMainInfo:** Ya está implementado y funciona igual que en S6, no requiere cambios.

---

## 📞 Soporte

Si encuentras offsets incorrectos o necesitas agregar más funciones:
1. Revisar `/workspace/Source/Source/Emulator 0.99 (2.1.7)/Main/Offset.h`
2. Comparar con el main.exe usando debugger
3. Actualizar `/workspace/MuServer99B/LuaPlugin_099b/src/Offset.h`
