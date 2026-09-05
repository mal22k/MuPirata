# 🎮 GUÍA COMPLETA DEL ECOSISTEMA — MU Online S6 (SSeMU)

> **Proyecto:** Cliente MU EX603 (Season 6 Episode 3) + GameServer SSeMU con Lua embebido.
> **Fecha:** Agosto 2026 · **Versiones:** LuaPlugin v0.2.0 · ShaderPlugin v0.1.2
> **Stack:** C++ (VS2010, toolset v100) · Lua 5.2 · OpenGL · Win32

---

## ÍNDICE

1. [Arquitectura general](#1-arquitectura-general)
2. [Componentes del proyecto](#2-componentes-del-proyecto)
3. [LuaPlugin (cliente) — API completa](#3-luaplugin-cliente--api-completa)
4. [GameServer — Lua del servidor](#4-gameserver--lua-del-servidor)
5. [Canal cliente ↔ servidor (3 vías)](#5-canal-cliente--servidor-3-vías)
6. [Ejemplos prácticos](#6-ejemplos-prácticos)
7. [Build y compilación](#7-build-y-compilación)
8. [Despliegue en el cliente](#8-despliegue-en-el-cliente)
9. [Solución de problemas](#9-solución-de-problemas)
10. [Ruta de trabajo (brechas conocidas)](#10-ruta-de-trabajo-brechas-conocidas)

---

## 1. ARQUITECTURA GENERAL

```
┌─────────────────────────── CLIENTE (main.exe EX603) ───────────────────────────┐
│                                                                                │
│  main.exe (parcheado, con VMProtect)                                           │
│   └─ main.dll  (hooks base del emulador: health bar, ProtocolCoreEx, MySend)  │
│        └─ gProtect.CheckPluginFile()  ← sistema de PLUGINS                    │
│             ├─ Lua.dll      (LuaPlugin v0.2.0)  ← EMBEBE LUA 5.2              │
│             │    └─ hooks: render (0x005B96E8) + packets (0x0065FD79)         │
│             │    └─ carga Lua\main.lua  (script del usuario)                  │
│             └─ Shader.dll   (ShaderPlugin v0.1.2)  ← post-process OpenGL      │
│                  └─ Shader\Shader.ini  (Preset=Retro|Cine|Limpio|Off)         │
└────────────────────────────────────────────────────────────────────────────────┘
                                    │  red (packets cifrados C1-C4)
┌─────────────────────────── SERVIDOR (GameServer SSeMU S6) ─────────────────────┐
│                                                                                │
│  GameServer.exe                                                                │
│   └─ Lua embebido 5.2 (la MISMA version que el cliente)                        │
│        └─ Data\Script\ScriptMain.lua  → require() de todos los scripts         │
│             ├─ System\ScriptCore.lua     (registro de bridges)                 │
│             ├─ Script\WelcomeMessage.lua (bienvenida)                          │
│             ├─ Script\LuaPluginDemo.lua  (canal con el cliente)                │
│             └─ Character\CalcCharacter.lua (formulas de daño por clase)        │
└────────────────────────────────────────────────────────────────────────────────┘
```

**Punto clave:** tanto el cliente como el servidor usan **Lua 5.2**. El mismo lenguaje en ambos lados, con APIs distintas (el cliente expone el juego al script; el servidor expone los objetos del mundo).

---

## 2. COMPONENTES DEL PROYECTO

| Carpeta | Qué es | Estado |
|---|---|---|
| `LuaPlugin/` | DLL inyectable con Lua 5.2 para el cliente | ✅ v0.2.0 compilada y probada en vivo |
| `ShaderPlugin/` | DLL de post-process GL (presets de look) | ✅ v0.1.2 compilada y probada |
| `MuServerS6Evercion maxima/` | Server completo S6 (GameServer, ConnectServer, Data, DB SQL) | ✅ operativo, Lua activo |
| `SOURCE viejos/` | Source viejo EX603 (Main_EX603) + Emulator/GameServer — **referencia de offsets** | 📚 referencia (NO es el binario actual) |
| `respaldo/SOURCE-2.0.7.rar` | Backup del source | 📦 |
| `REPORTE_CLIENTE_EX603.txt` | Reporte técnico del source viejo | 📄 |
| `GUIA_ECOSISTEMA.md` | Este documento | 📄 |

**Importante:** el `main.exe` actual está **protegido con VMProtect** (`.text` cifrado en archivo, se descifra en runtime). Los offsets del source viejo sirven de referencia pero solo se confirman probando en vivo. El verificador `Tools/LuaClientCheck/check_offsets.ps1` valida cualquier offset nuevo contra el binario real.

---

## 3. LUA PLUGIN (CLIENTE) — API COMPLETA

### 3.1 Callbacks (los define el script, los llama el plugin)

| Callback | Firma | Cuándo se llama |
|---|---|---|
| `on_draw` | `on_draw(cursor_x, cursor_y)` | Cada frame, solo en juego (screen state 5) |
| `on_key` | `on_key(vk, pressed)` | En transiciones de teclas **suscritas** (max 32) |
| `on_click` | `on_click(x, y, boton, pressed)` | Transiciones de mouse: 1=izq, 2=der, 4=medio |
| `on_packet` | `on_packet(head, sub, data_hex)` | Por cada packet del server (fallback global) |

**Reglas de input:** solo dentro del juego, cliente en primer plano, solo en *transiciones* (sin spam). Si un callback lanza error, se loguea **una vez** y se desactiva hasta reiniciar (anti-spam). `on_draw` y `on_packet` tienen flags independientes.

### 3.2 Tabla `Draw`

| Función | Descripción |
|---|---|
| `Draw.Text(x, y, texto, r, g, b)` | Texto con color (RGB 0-255), espacio VIRTUAL 640x480 (delega en el renderer nativo) |
| `Draw.TextRaw(x, y, texto, r, g, b, [a])` | Texto en espacio RAW de pixels (v0.3.6): glifos rasterizados por GDI al tamaño final y dibujados con GL en pixels enteros → sin jitter con escalas no enteras. El script escala/snapea x,y (mismo helper que `Draw.Image`). Con `\n` hace salto de línea |
| `Draw.Bar(x, y, w, h, r, g, b, a)` | Rectángulo con alpha (usa OpenGL) |
| `Draw.Message(texto, tipo)` | Mensaje del cliente (chat) |
| `Draw.Image(id, x, y, w, h, u0, v0, u1, v1, alpha)` | Sprite (UV 0..1) |
| `Draw.LoadImage(path, w, h)` | Carga textura → devuelve `id` |

### 3.3 Tabla `Input`

| Función | Descripción |
|---|---|
| `Input.CursorX()` / `Input.CursorY()` | Posición del cursor |
| `Input.RegisterKey(vk)` | Suscribe tecla para `on_key` (bool) |
| `Input.KeyPressed(vk)` | Estado actual de la tecla (bool) |

⚠️ **Teclas ocupadas en este cliente:** F9 = health bar (main.dll), F8/F10/F11/F12 = autoattack/cámara/tray. **Insert (0x2D) está libre** — es la del panel demo.

### 3.4 Tabla `Client` (datos del personaje en vivo)

Leídos de `MAIN_CHARACTER_STRUCT` (0x08128AC8 → puntero al struct) con **lectura segura** (VirtualQuery: si no hay personaje o el build difiere, devuelven 0 sin crashear).

| Función | Origen | Función | Origen |
|---|---|---|---|
| `CharacterName()` | struct +0x00 | `Shield()` / `MaxShield()` | +0x2A / +0x2C |
| `Level()` | +0x0E (WORD) | `BP()` / `MaxBP()` | +0x40 / +0x42 |
| `Class()` | +0x0B (BYTE) | `Strength()` | +0x18 |
| `HP()` / `MaxHP()` | +0x22 / +0x26 | `Dexterity()` | +0x1A |
| `MP()` / `MaxMP()` | +0x24 / +0x28 | `Vitality()` | +0x1C |
| `LevelUpPoint()` | +0x74 | `Energy()` | +0x1E |
| `Experience()` | +0x10 (DWORD) | `Leadership()` | +0x20 |
| `NextExperience()` | +0x14 (DWORD) | `Map()` | MAIN_CURRENT_MAP |
| `Money()` | **cache de packets** (ver abajo) | `ScreenState()` / `InGame()` | MAIN_SCREEN_STATE (5=en juego) |
| `ResolutionX()` / `ResolutionY()` | — | | |

**Sobre `Money()`:** el dinero **no vive en el struct del personaje** — se cachea de los packets del servidor en el hook de protocolo:
- `C3:F3:03` (entrar al mundo): DWORD little-endian en bytes 50..53
- `C3:22` result=0xFE (ganar/gastar zen): big-endian en bytes 4..7

El parser vive en `src/MoneyCache.h` y está **cubierto por 10 unit tests** (test_lua.exe) que arman los packets exactamente como el GameServer (verificado contra `DSProtocol.cpp:1313` y `GCMoneySend`). El cache se actualiza **siempre**, aunque el script falle.

### 3.5 Tabla `Interface` (ventanas nativas)

| Función | Descripción |
|---|---|
| `Interface.Open(wid)` | Abre ventana nativa (pOpenWindow 0x0085EC50) |
| `Interface.Close(wid)` | Cierra ventana (pClosekWindow 0x0085F9A0) |
| `Interface.IsOpen(wid)` | ¿Está abierta? (pCheckWindow 0x0085EC20) |

⚠️ **Los window IDs varían según el build.** En muchos EX603: `0x02` = MoveList (NO inventario), inventario = otro (común 0x0B). Por eso el demo usa **calibración automática** (ver §6.3).

### 3.6 Packets

| Función | Descripción |
|---|---|
| `SendPacket(hex)` | Envía packet al server (C1/C2 plano, C3/C4 cifrado con serial). Auto-ajusta el tamaño. Devuelve true/false |
| `RegisterPacketHandler(head, sub\|nil, fn)` → id | Router de opcodes. `sub=nil` = wildcard |
| `UnregisterPacketHandler(id)` | Quita un handler |

**Router (prioridad):** handler exacto `[head][sub]` > wildcard `[head][nil]` > `on_packet`. Solo **un** consumidor por packet: si el handler devuelve `false`, el packet sigue la cadena y `on_packet` NO se llama. Si devuelve `true`, se consume (el cliente original no lo ve).

```lua
-- Ejemplo router: responder al server y consumir
local id = RegisterPacketHandler(0xFC, 0x01, function(head, sub, data)
    SendPacket("C1 07 FC 02 01 00 00")  -- respuesta
    return true                          -- consumido
end)
```

### 3.7 Otros

| Función | Descripción |
|---|---|
| `Log(mensaje)` | Escribe en `Lua\lua_plugin.log` |

**Formato de `data_hex` en `on_packet`:** string hex completo del packet (`data[1]`=tipo, `data[2]`=size, `data[3]`=head para C1/C3). El sub se extrae: C1/C3 → byte[3], C2/C4 → byte[4]. Sin sub → `-1`.

---

## 4. GAMESERVER — LUA DEL SERVIDOR

### 4.1 Sistema de scripts

- **`Data\Script\ScriptMain.lua`** — SOLO hace `require()` de otros scripts (no agregar código aquí).
- **`Data\Script\System\ScriptCore.lua`** — el motor de bridges (`BridgeFunctionAttach` + `BridgeFunction_*`). **No borrar**: sin él ningún script funciona.
- **Registro de un script nuevo:**
  1. Crear `Data\Script\Script\MiScript.lua`
  2. Añadir `require('Script\MiScript')` en `ScriptMain.lua`
  3. Recargar con `/reload` (GM) o reiniciar el GS

### 4.2 Bridges — 21 eventos capturables

Para escuchar un evento, se registra la función con `BridgeFunctionAttach('NombreBridge', 'MiFuncion')`:

```lua
BridgeFunctionAttach('OnCharacterEntry', 'MiScript_OnCharacterEntry')

function MiScript_OnCharacterEntry(aIndex)
    -- aIndex = índice del jugador en el GS
end
```

| Bridge | Args típicos | Devolver |
|---|---|---|
| `OnReadScript` | — | — |
| `OnShutScript` | — | — |
| `OnTimerThread` | — | — (cada ~1s) |
| `OnCommandManager` | aIndex, code, arg | `1` = comando manejado |
| `OnCommandDone` | aIndex, code | — |
| `OnCharacterEntry` | aIndex | — |
| `OnCharacterClose` | aIndex | — |
| `OnNpcTalk` | aIndex, NpcIndex | `1` = manejado |
| `OnMonsterDamaged` | aIndex, MonsterIndex | — |
| `OnMonsterDie` | aIndex, MonsterIndex | — |
| `OnUserDie` | aIndex | — |
| `OnUserMove` | aIndex | — |
| `OnUserRespawn` | aIndex | — |
| `OnCheckUserTarget` | aIndex, TargetIndex | `0` = bloquear |
| `OnCheckUserKiller` | aIndex, KillerIndex | `0` = bloquear |
| `OnUserItemPick` | aIndex, ItemIndex | `0` = bloquear |
| `OnUserItemDrop` | aIndex, ItemIndex | `0` = bloquear |
| `OnUserItemMove` | aIndex | `0` = bloquear |
| `OnPartyEntry` | aIndex | — |
| `OnPartyClose` | aIndex | — |
| `OnSQLAsyncResult` | QueryIndex | — |

**Regla de devolución:** los bridges con `ret` devuelven `1` = permitido/manejado, `0` = bloqueado (los de "Check/Item" invierten la lógica: devolver 0 bloquea la acción). Si ningún script registra el bridge, el valor default del GS es `0` en CommandManager/NpcTalk (pasa) y `1` en los Check (permite).

### 4.3 Funciones de interfaz (~160, agrupadas)

**Objeto / jugador (GetObject\*):**
`GetObjectName, Account, AccountLevel, AccountExpireDate, Authority, Class, Connected, Live, Type, Lang, Level, MasterLevel, MasterPoint, MasterReset, Reset, LevelUpPoint, Life, MaxLife, Mana, MaxMana, Shield, MaxShield, BP, MaxBP, Money, Map, MapX, MapY, Strength, Dexterity, Vitality, Energy, Leadership, Default*/Extra* (stats base/extra), Total* (via CalcCharacter), PKLevel, PKCount, PKTimer, GensFamily, GensRank, GensSymbol, GensContribution, GuildName, GuildNumber, GuildStatus, GuildRelationship, GuildUnionName, GuildUnionNumber, CSGuildSide, PartyNumber, Interface, OfflineFlag, HardwareId, IpAddress, PersonalCode, Password, IndexByName, Change, ChangeUp`

**SetObject\* (modificar):**
`SetObjectLevel, LevelUpPoint, Money, Map, MapX, MapY, Strength, Dexterity, Vitality, Energy, Leadership, MasterLevel, MasterPoint, PKLevel, PKCount, PKTimer, ChatLimitTime` · `UserSetAccountLevel` · `ObjectAddCoin / ObjectSubCoin / ObjectGetCoin` · `UserActionSend, UserCalcAttribute, UserDisconnect, UserGameLogout, UserInfoSend`

**Items:**
`GetItemIndex, GetItemName, GetItemTable, GetItemCount, GetFreeSlotCount, CheckSpaceByItem, CheckSpaceBySize, DelItemCount, DelItemIndex, ItemGive, ItemGiveEx, ItemDrop, ItemDropEx, ItemBag, MapGetItemTable, GetFullSize, GetMainSize, GetWearSize`

**Mensajes (a jugador / a todos / globales):**
`NoticeSend, NoticeSendToAll, NoticeGlobalSend, MessageSend, MessageSendToAll, MessageGlobalSend, MessageGet, LevelSend, LevelUpSend, MoneySend`

**Mundo / monstruos:**
`MonsterCreate, MonsterDelete, MonsterSummonCreate, MonsterSummonDelete, MonsterCount, GetMonsterName, MoveUser, MoveUserEx, MapCheckAttr, GetMapName, SkillTreeRebuild, SkillTreeExtRebuild`

**Party / Quest:**
`PartyGetMemberCount, PartyGetMemberIndex, QuestStateCheck`

**SQL (síncrono y asíncrono):**
`SQLConnect, SQLDisconnect, SQLQuery, SQLFetch, SQLGetNumber, SQLGetSingle, SQLGetString, SQLGetResult, SQLCheck, SQLClose` · `SQLAsyncConnect, SQLAsyncDisconnect, SQLAsyncQuery, SQLAsyncGetNumber, SQLAsyncGetSingle, SQLAsyncGetString, SQLAsyncGetResult, SQLAsyncCheck, SQLAsyncResult`

**Server / utilidades:**
`GetGameServerCode, GetGameServerProtocol, GetGameServerVersion, GetGameServerCurUser, GetGameServerMaxUser, GetMinUserIndex, GetMaxUserIndex, GetMinMonsterIndex, GetMaxMonsterIndex, GetMaxIndex, CheckGameMasterLevel, SetOption, GetArgNumber, GetArgString, GetNumber`

> 📄 Documentación oficial completa en `Tools\Guides\Script Lua BridgeFunctions.rtf` y `Script Lua Interface Functions.rtf`.

---

## 5. CANAL CLIENTE ↔ SERVIDOR (3 VÍAS)

```
┌─────────┐  ① NoticeSend("[LUA:...]") → C1:0D       ┌─────────┐
│         │ ───────────────────────────────────────▶ │         │
│ CLIENTE │  ② SendChat("/luatest") → C1:00 chat      │ SERVIDOR│
│ (Lua)   │ ◀─────────────────────────────────────── │ (Lua)   │
│         │  ③ SendPacket("C1 FC ...") → opcode custom│         │
│         │ ───────────────────────────────────────▶ │         │
└─────────┘                                          └─────────┘
```

| Vía | Dirección | Mecanismo | Estado |
|---|---|---|---|
| ① **Notices** | Server → Cliente | `NoticeSend(aIndex, 1, "[LUA:...]")` → packet `C1:0D`. El cliente filtra el prefijo `[LUA` en `on_packet`, lo muestra y lo consume | ✅ **Funciona** |
| ② **Chat** | Cliente → Server | `SendChat("/luatest")` arma un packet `C1:00` con el nombre real del personaje (relleno con `\0`, NO espacios — el server hace strcmp) → `OnCommandManager` | ✅ **Funciona** |
| ③ **Custom puro** | Bidireccional | Opcode `0xFC` libre: el botón ALQ envía `C1:FC:02`; el router cliente responde a `C1:FC:01/03` | ⚠️ Falta handler C++ en el GS (brecha #1, ver §10) |

**Round-trip demo:** botón PING → `/luatest` → server responde `[LUA:CMD] nombre nivel=.. mapa=.. zen=..` → el panel lo muestra. Heartbeat cada 10s: `[LUA:HEARTBEAT] tick=.. online=..`.

---

## 6. EJEMPLOS PRÁCTICOS

### 6.1 Server: responder un comando custom

```lua
-- Data\Script\Script\MiScript.lua  +  require('Script\MiScript') en ScriptMain.lua
BridgeFunctionAttach('OnCommandManager', 'MiScript_OnCommand')

function MiScript_OnCommand(aIndex, code, arg)
    if code ~= 201 then
        return 0  -- no es mi comando, que lo maneje otro
    end

    local name = GetObjectName(aIndex)
    local money = GetObjectMoney(aIndex)

    -- Dar 1.000.000 de zen
    ObjectAddCoin(aIndex, 1000000)

    NoticeSend(aIndex, 1, "[LUA:CMD201] " .. name .. " ahora tienes " .. GetObjectMoney(aIndex) .. " zen")
    return 1  -- manejado
end
```

### 6.2 Server: evento de muerte con recompensa

```lua
BridgeFunctionAttach('OnMonsterDie', 'MiScript_OnMonsterDie')

function MiScript_OnMonsterDie(aIndex, MonsterIndex)
    local monsterName = GetMonsterName(MonsterIndex)

    if monsterName == "Balrog" then
        ItemGive(aIndex, GET_ITEM(14, 13), 1)  -- ojo: GET_ITEM es macro del server
        NoticeSend(aIndex, 1, "[LUA:BOSS] Mataste al Balrog!")
    end
end
```

> ⚠️ `GET_ITEM` puede no estar disponible en Lua directo; usar `ItemGive(aIndex, indiceNumerico, cant)` con el índice numérico del item (ver `GetItemTable`).

### 6.3 Cliente: calibración del inventario (2 pasos, ya en el demo)

```lua
-- Paso 1: pulsar INV -> guardar ventanas abiertas
local snapshot = {}
for id = 1, 0x1F do
    if Interface.IsOpen(id) then snapshot[#snapshot+1] = string.format("0x%02X", id) end
end

-- El usuario abre el inventario con la tecla I...
-- Paso 2: pulsar INV otra vez -> detectar la ventana NUEVA
for id = 1, 0x1F do
    if Interface.IsOpen(id) then
        local cur = string.format("0x%02X", id)
        local ya = false
        for _, prev in ipairs(snapshot) do if prev == cur then ya = true break end end
        if not ya then INV_WINDOW = tonumber(cur) end  -- ¡calibrado!
    end
end
```

### 6.4 Cliente: panel con stats (on_draw)

```lua
function on_draw(cx, cy)
    if not Client.InGame() then return end

    Draw.Bar(8, 8, 250, 140, 15, 15, 20, 190)
    Draw.Text(14, 44, "Char: " .. Client.CharacterName() .. "  Lv: " .. Client.Level(), 160, 200, 255)
    Draw.Text(14, 56, "HP: " .. Client.HP() .. "/" .. Client.MaxHP() .. "  MP: " .. Client.MP() .. "/" .. Client.MaxMP(), 160, 255, 160)
    Draw.Text(14, 68, "Map: " .. Client.Map() .. "  Money: " .. Client.Money(), 255, 255, 200)
end
```

### 6.5 Cliente: escuchar un packet del server y consumirlo

```lua
RegisterPacketHandler(0xFC, 0x03, function(head, sub, data)
    Log("Server pidio cerrar alquimia: " .. data)
    return true  -- el cliente original no lo procesa (opcode custom)
end)
```

---

## 7. BUILD Y COMPILACIÓN

**Requisitos:** Visual Studio 2010 (v100) o VS2022 con toolset v100 instalado. Windows + bash.

### LuaPlugin
```bash
cd LuaPlugin
cmd //c build.bat          # → Output/Lua.dll  (156 KB, Release Win32)
# Test standalone (sin el cliente):
cd test
cmd //c build_test.bat     # → test_lua.exe
./test_lua.exe             # → "parser de dinero: 10 tests OK" + "TEST OK"
```
⚠️ **Siempre Release.** La Debug usa /MDd y lanza LNK4098 (mismatch CRT). El warning `LNK4098` en Release es **benigno** (documentado).

### ShaderPlugin
```bash
cd ShaderPlugin
cmd //c build.bat          # → Output/Shader.dll
cd test
cmd //c build_test.bat     # → test_shader.exe (valida presets e INI)
```

### GameServer
No se recompila (binario cerrado). Se modifican solo scripts `.lua` en `Data\Script\` + `Data\CommandManager.txt`.

---

## 8. DESPLIEGUE EN EL CLIENTE

1. **Copiar DLLs** a la carpeta del cliente (junto a main.exe): `Lua.dll` + `Shader.dll` (+ `Lua\main.lua`, `Shader\Shader.ini`).
2. **Registrar plugins:** en `MainInfo.ini` → `[MainInfo] PluginName1=Lua.dll` (y `PluginName2=Shader.dll` si aplica).
3. **Ejecutar `GetMainInfo.exe`** → regenera `Path\ServerInfo.sse` con los CRC. **Obligatorio cada vez que cambie una DLL** — si el CRC no cuadra, el cliente se cierra al arrancar.
4. **Distribuir:** main.exe + main.dll + DLLs plugins + ServerInfo.sse + carpetas `Lua\` y `Shader\`.

### Despliegue del lado server
1. Copiar `LuaPluginDemo.lua` → `Data\Script\Script\`.
2. En `Data\Script\ScriptMain.lua`: `require('Script\LuaPluginDemo')`.
3. En `Data\CommandManager.txt`, comando `/luatest` (index 200):
   ```
   200  1  1  1  1  "/luatest"  0  *  *  *  *  *  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   ```
4. `/reload` (GM) o reiniciar GameServer.

---

## 9. SOLUCIÓN DE PROBLEMAS

| Síntoma | Causa | Fix |
|---|---|---|
| Cliente se cierra al arrancar | CRC de ServerInfo.sse desactualizado | Volver a ejecutar GetMainInfo |
| Desconexión al entrar al mundo | (bug ya resuelto del chaining de packets) | No tocar `InstallPacketHook`; el check es por VirtualQuery, no rango fijo |
| PING no responde ("esperando") | SendChat rellena el nombre con espacios | El relleno debe ser con `\0` (ya aplicado). Ver log `Lua\lua_plugin.log` |
| Botón INV abre otra ventana | Window ID varía según build (0x02 = MoveList) | Calibración automática en 2 pasos (ver §6.3) |
| Money() en 0 | No llegó F3:03 (recién entrando) o el struct no aplica | Entrar al mundo; el dinero se actualiza con cada C3:22:FE |
| Stats en 0 | Sin personaje cargado / build distinto | Los reads son seguros (VirtualQuery), no crashean; verificar offsets |
| Anti-cheat bloquea la DLL | MHPClient/MHPServer | Probar con anti-cheat desactivado o registrar el plugin |
| F1-F12 cambian el look del shader | (ya resuelto) | ShaderPlugin v0.1.2 no tiene hotkeys; solo `Shader.ini` |

**Logs:** cliente → `Lua\lua_plugin.log` · server → consola/logs del GS.

---

## 10. RUTA DE TRABAJO (BRECHAS CONOCIDAS)

| # | Tarea | Esfuerzo | Impacto |
|---|---|---|---|
| 1 | **Handler `C1:FC` en el GS** (Protocol.cpp + `OnCustomPacket`) | Bajo (C++ GS) | Cierra el canal custom puro (vía ③) |
| 2 | **Framework de UI en Lua puro** (ventanas custom: alquimia con trade, shops) | Medio (solo main.lua) | UI real sin tocar C++ |
| 3 | **Persistencia de config** (`Lua\config.txt` para recordar la calibración INV entre sesiones) | Bajo (solo main.lua) | Calibrar una sola vez |
| 4 | **Conectar los 18 bridges del server sin usar** (eventos, SQL, PK, items, party) | Bajo-Medio (scripts) | Contenido custom real |
| 5 | **Bloqueo de clicks sobre UI custom** (hook del mouse del cliente, patrón `HelperMouseClick` 0x007D2920) | Medio (C++ cliente) | Ventanas custom reales sin tocar el juego |

---

*Documento generado a partir del código real del proyecto. Para offsets del cliente: `LuaPlugin/src/Offset.h` + `SOURCE viejos/Source/Main_EX603/Main/Offset.h`. Para la API del server: `Tools/Guides/*.rtf`.*
