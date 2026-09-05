# BRIDGES CLIENTE <-> SERVIDOR (MU Online S6 EX603 / SSeMU)

**Documento de handoff para otra IA.** Objetivo: comparar el bridge Lua del **cliente** (funciones confirmadas y funcionando) contra el Lua del **servidor**, y diseñar **eventos y features custom** que usen ambos lados.

> Generado por: proyecto ssemu (carpeta de trabajo) + despliegue real `Package_SSeMU_S6_ENG`.
> Fecha: agosto 2026 · Cliente: MU EX603 Season 6 · Server: SSeMU S6 Everción maxima · Lua 5.2 ambos lados.

---

## 1. CONTEXTO Y ARQUITECTURA

```
┌──────────────────────────── CLIENTE (main.exe EX603) ────────────────────────────┐
│  Lua.dll v0.3.2  (plugin cargado via main.dll / MainInfo.ini)                     │
│  · Lua 5.2 embebido, script principal:  Lua\main.lua                              │
│  · Log:  Lua\lua_plugin.log  ·  Config:  Lua\config.txt                           │
│  · Callbacks invocadas por la DLL: on_draw / on_key / on_click / on_packet        │
│  · API global expuesta a Lua:  Draw.*  Input.*  Interface.*  UI.*  Client.*       │
│                             Log()  SendPacket()  RegisterPacketHandler()          │
└───────────────────────────────────────────────────────────────────────────────────┘
                                  │  Red (packets C1/C2/C3/C4, cifrados por MySend)
                                  ▼
┌──────────────────────────── SERVIDOR (GameServer SSeMU) ─────────────────────────┐
│  Lua 5.2 embebido, scripts en:  MuServerS6Evercion maxima\Data\Script\            │
│  · Entry:  ScriptMain.lua  (solo require) →  System\ScriptCore.lua (bridge hub)  │
│  · Scripts custom:  Script\*.lua  (base: Script\TemplateScript.lua)               │
│  · Bridges (eventos):  BridgeFunctionAttach('OnXxx','MiFunc')                     │
│  · API de interfaz:  GetObject*()  SetObject*()  NoticeSend()  ItemGive()  ...    │
│  · Docs oficiales:  Tools\Guides\Script Lua Interface Functions.rtf               │
│                     Tools\Guides\Script Lua BridgeFunctions.rtf                   │
└───────────────────────────────────────────────────────────────────────────────────┘
```

### Canal de comunicación YA PROBADO (referencia obligatoria)

Existe un demo funcional que conecta ambos lados: `MuServerS6Evercion maxima\Data\Script\Script\LuaPluginDemo.lua` + el panel del cliente.

- **Servidor → Cliente**: `NoticeSend(aIndex, tipo, "[LUA:ETIQUETA] mensaje")` → llega como packet **C1:0D** → el cliente lo intercepta en `on_packet(0x0D, sub, data_hex)` buscando el prefijo `[LUA`. También `MessageSend(aIndex, ...)` (chat).
- **Cliente → Servidor**: el cliente envía `/luatest` por chat (`SendPacket("C1 00 ...")`) → el server lo recibe en `OnCommandManager(aIndex, code, arg)` (code = índice del comando en `Data\CommandManager.txt`) → responde por NoticeSend.

⚠️ **Regla heredada**: NO depender del opcode custom 0xFC (falta handler en el GameServer). Usar el canal NoticeSend/CommandManager o packets estándar.

---

## 2. API DEL CLIENTE — FUNCIONES CONFIRMADAS (Lua.dll v0.3.2)

Estado: `CONFIRMADO` = verificado en vivo / `PARCIAL` = implementado y probado en harness, revisar en vivo / `NO CONFIRMADO` = stub seguro (falla suave: nil/false/-1/"" + log una vez, nunca crashea).

### 2.1 Callbacks (definidas por el script, invocadas por la DLL)

| Callback | Firma | Cuándo | Estado |
|---|---|---|---|
| `on_draw` | `on_draw(cursor_x, cursor_y)` | Cada frame (dentro del juego, screen state 5) | CONFIRMADO |
| `on_key` | `on_key(vk, pressed)` | Solo teclas suscritas con `Input.RegisterKey(vk)` | CONFIRMADO |
| `on_click` | `on_click(x, y, boton, pressed)` | Botones del ratón (1=izq, 2=der; pressed 1/0) | CONFIRMADO |
| `on_packet` | `on_packet(head, sub, data_hex)` | Cada packet recibido (head=0xC1/C2..., sub=2º byte, data_hex=resto en hex) | CONFIRMADO |

### 2.2 Tabla `Draw` (overlay 2D)

| Función | Firma | Estado |
|---|---|---|
| `Draw.Text` | `(x, y, texto, r, g, b)` | CONFIRMADO |
| `Draw.Bar` | `(x, y, w, h, r, g, b, a)` | CONFIRMADO |
| `Draw.Message` | `(texto, tipo)` → mensaje en el chat del cliente | CONFIRMADO |
| `Draw.Image` | `(id, x, y, w, h, u0, v0, u1, v1, alpha)` | CONFIRMADO |
| `Draw.LoadImage` | `(path, w, h)` → id | CONFIRMADO |
| `Draw.Tooltip` | (tooltip nativo; usa pDrawToolTip 0x00597220) | PARCIAL |

### 2.3 Tabla `Input`

| Función | Firma | Estado |
|---|---|---|
| `Input.CursorX` / `Input.CursorY` | `()` → número | CONFIRMADO |
| `Input.RegisterKey` | `(vk)` → suscribe tecla para `on_key` | CONFIRMADO |
| `Input.KeyPressed` | `(vk)` → bool (GetAsyncKeyState) | CONFIRMADO |

### 2.4 Tabla `Interface` (ventanas nativas del cliente)

| Función | Firma | Estado |
|---|---|---|
| `Interface.Open` | `(wid)` | CONFIRMADO (pOpenWindow 0x0085EC50) |
| `Interface.Close` | `(wid)` | CONFIRMADO (pCloseWindow 0x0085F9A0) |
| `Interface.IsOpen` | `(wid)` → bool | CONFIRMADO (pCheckWindow 0x0085EC20) |
| `Interface.GetOpenWindows` | `()` → tabla con los wid abiertos | PARCIAL |
| `Interface.GetActiveWindow` | `()` → wid o -1 | NO CONFIRMADO (-1 + log) |

> Nota heredada: en EX603 `0x02` puede ser MoveList (no inventario). El inventario se **calibra** en runtime (`Lua\config.txt`, clave `inv_window`).

### 2.5 Tabla `UI` (bloqueo de clicks sobre UI custom — 3 capas)

| Función | Firma | Estado |
|---|---|---|
| `UI.BlockMouse` | `(x, y, w, h)` → registra rectángulo de bloqueo | CONFIRMADO |
| `UI.ClearBlockedRects` | `()` | CONFIRMADO |
| `UI.SetMouseBlockEnabled` | `(bool)` → activa/desactiva el sistema | CONFIRMADO |
| `UI.IsMouseInside` | `(x, y, w, h)` → bool | CONFIRMADO |
| `UI.ConsumeClick` | `()` → consume el click actual | CONFIRMADO |
| `UI.CursorInsideUI` | `()` → bool (cursor dentro de algún rect registrado) | CONFIRMADO |
| `UI.IsMouseBlockHooked` / `UI.MouseClickCallSite` | diagnóstico | CONFIRMADO |
| `UI.WndProcInstalled` / `UI.SendGuardInstalled` | diagnóstico por capa | CONFIRMADO |
| `UI.BlockedByHook` / `UI.BlockedByWndProc` / `UI.BlockedBySend` | contadores por capa | CONFIRMADO |

**Las 3 capas (verificadas en vivo con el log real del cliente):**
1. **Hook MouseClick** — call site `0x007D2B0C` encadenado a `HelperMouseClick (0x007D2920)`: devuelve "consumido" al juego.
2. **WndProc guard** — subclase de todas las ventanas del proceso (EnumWindows, chaining con CallWindowProc): traga WM_LBUTTONDOWN/UP/RBUTTON/MBUTTON/DBLCLK sobre la UI.
3. **SendGuard** — envuelve `CLIENT_SEND_POINTER (0x00D227F8, MySend)`: traga C1:04 (ataque/click), C1:05 (mover) y C1:06 mientras el cursor está sobre la UI. **Es la garantía de que el personaje no se mueve ni ataca al clickear una ventana custom.**

### 2.6 Tabla `Client` (personaje + estados)

**Datos del personaje — CONFIRMADO** (desde `MAIN_CHARACTER_STRUCT 0x08128AC8`):

`ScreenState()`, `InGame()`, `ResolutionX()`, `ResolutionY()`, `CharacterName()`, `Level()`, `Class()`, `HP()`, `MaxHP()`, `MP()`, `MaxMP()`, `Shield()`, `MaxShield()`, `BP()`, `MaxBP()`, `Strength()`, `Dexterity()`, `Vitality()`, `Energy()`, `Leadership()`, `LevelUpPoint()`, `Experience()`, `NextExperience()`, `Money()`, `Map()`.

**Estados cacheados por packets (FASE 7) — PARCIAL:**

| Función | Estado |
|---|---|
| `Client.IsTradeOpen()` / `IsTradeAccepted()` / `GetTradeMoney()` / `GetTradeItem(slot)` | PARCIAL (cache por packets C1:3E/40...) |
| `Client.IsShopOpen()` | PARCIAL |
| `Client.IsChaosBoxOpen()` | PARCIAL |

**Stubs seguros — NO CONFIRMADO (nil/false/-1/"" + log único, sin crashear):**

`IsNpcDialogOpen()`, `GetNpcIndex()`, `GetNpcName()`, `GetInventoryItem(slot)`, `IsInventorySlotEmpty(slot)`, `GetWearItem(slot)`, `GetShopItem(slot)`, `GetShopPrice(slot)`, `GetChaosItem(slot)`.

### 2.7 Funciones globales

| Función | Firma | Estado |
|---|---|---|
| `Log` | `Log(msg)` → escribe en `Lua\lua_plugin.log` | CONFIRMADO |
| `SendPacket` | `SendPacket("C1 05 BF 51 00")` → bool. Acepta C1/C2/C3/C4, ajusta el tamaño de cabecera solo | CONFIRMADO |
| `RegisterPacketHandler` | `(head, sub|nil, fn)` → id. `head`=0xC1..., `sub`=2º byte o nil=wildcard | CONFIRMADO |
| `UnregisterPacketHandler` | `(id)` | CONFIRMADO |

### 2.8 Offsets confirmados del cliente (check_offsets.ps1, 32/32)

| Nombre | Offset | Estado |
|---|---|---|
| Render hook | `0x005B96E8` | CONFIRMADO |
| Packet hook (dispatcher) | `0x0065FD79` | CONFIRMADO |
| `MAIN_CHARACTER_STRUCT` | `0x08128AC8` (.data) | CONFIRMADO |
| `ITEM_TABLE_PTR` | `0x08128AC0` (.data) | CONFIRMADO |
| `pOpenWindow` / `pCloseWindow` / `pCheckWindow` | `0x0085EC50` / `0x0085F9A0` / `0x0085EC20` | CONFIRMADO |
| `HelperMouseClick` | `0x007D2920` (call site `0x007D2B0C`) | CONFIRMADO |
| `pDrawToolTip` | `0x00597220` | CONFIRMADO |
| `CLIENT_SEND_POINTER` (MySend) | `0x00D227F8` | CONFIRMADO |
| `MAIN_WINDOW` | `0x00E8C578` (fallback; el WndProc usa EnumWindows) | CONFIRMADO |

---

## 3. API DEL SERVIDOR — BRIDGES (eventos) SSeMU

Registro: `BridgeFunctionAttach('NombrePuente','NombreFuncion')` (una vez, al cargar el script). Implementación: `System\ScriptCore.lua` (`BridgeFunctionTable`). Plantilla completa con comentarios: `Script\TemplateScript.lua`.

| Bridge | Firma | Retorno esperado | Cuándo |
|---|---|---|---|
| `OnReadScript` | `()` | — | Inicialización del GameServer |
| `OnShutScript` | `()` | — | Antes de recargar scripts |
| `OnTimerThread` | `()` | — | Cada 1 segundo (timer global) |
| `OnCommandManager` | `(aIndex, code, arg)` | 1 = comando válido / 0 = no | Usuario escribe comando (`code` = índice en CommandManager.txt, `arg` = parámetros) |
| `OnCommandDone` | `(aIndex, code)` | — | Tras ejecutar comando válido |
| `OnCharacterEntry` | `(aIndex)` | — | Jugador entra al juego con personaje |
| `OnCharacterClose` | `(aIndex)` | — | Jugador sale con personaje |
| `OnNpcTalk` | `(aIndex, bIndex)` | 1 = permitir / 0 = bloquear | Jugador habla con NPC (aIndex=NPC, bIndex=usuario) |
| `OnMonsterDamaged` | `(aIndex, bIndex, damage, finalDamage)` | — | Monstruo recibe daño |
| `OnMonsterDie` | `(aIndex, bIndex)` | — | Monstruo muere (bIndex=matador) |
| `OnUserDie` | `(aIndex, bIndex)` | — | Jugador muere |
| `OnUserMove` | `(aIndex, MapIndex)` | — | Jugador se mueve |
| `OnUserRespawn` | `(aIndex, KillerType)` | — | Respawn (KillerType: 0=Monster,1=User,2=GuildWar,3=Duel) |
| `OnCheckUserTarget` | `(aIndex, bIndex)` | 1 = se puede atacar / 0 = no | Verificación de target de ataque |
| `OnCheckUserKiller` | `(aIndex, bIndex)` | 1 = dar PK / 0 = no | Verificación de estado PK |
| `OnUserItemPick` | `(aIndex, slot)` | 1 = permitir / 0 = bloquear | Recoger item |
| `OnUserItemDrop` | `(aIndex, slot, x, y)` | 1 = permitir / 0 = bloquear | Tirar item |
| `OnUserItemMove` | `(aIndex, aFlag, aSlot, bFlag, bSlot)` | 1 = permitir / 0 = bloquear | Mover item. Flags: 0=Inventario, 1=Trade, 2=Almacén, 3=Chaos Box, 4=Personal Shop, 5~20=Chaos Box, 21=Event Inv, 22=Muun Inv |
| `OnPartyEntry` | `(aIndex, index)` | — | Entra a party (index=slot) |
| `OnPartyClose` | `(aIndex, index)` | — | Sale de party |
| `OnSQLAsyncResult` | `(label, param, result)` | — | Resultado de query async (0=Fail/1=Success) |

> ⚠️ Semántica de retorno (importante para no invertirla): los bridges "OnCheck*" y "OnUserItem*" retornan **1 = permitir**. `OnNpcTalk` y `OnCommandManager` retornan **1 = manejado/válido** (en OnCommandManager, `ScriptCore` devuelve 1 si *alguna* función registrada devolvió ≠ 0). `OnCommandManager` se invoca solo con comandos válidos registrados en `Data\CommandManager.txt`.

---

## 4. API DEL SERVIDOR — FUNCIONES DE INTERFAZ (llamables desde tus scripts)

Lista completa extraída de `Tools\Guides\Script Lua Interface Functions.rtf` (SSeMU). Agrupada por dominio:

**Índices / servidor:** `GetMaxIndex()`, `GetMinUserIndex()`, `GetMaxUserIndex()`, `GetMinMonsterIndex()`, `GetMaxMonsterIndex()`, `GetGameServerCode()`, `GetGameServerVersion()`, `GetGameServerProtocol()` (0=Kor/1=Eng/2=Jpn/3=Chs/4=Tai/5=Phi), `GetGameServerCurUser()`, `GetGameServerMaxUser()`.

**Getters de objeto** (todas `(aIndex)`): `GetObjectConnected`, `GetObjectIpAddress`, `GetObjectHardwareId`, `GetObjectLang` (NEW), `GetObjectType`, `GetObjectAccount`, `GetObjectPassword`, `GetObjectName`, `GetObjectPersonalCode`, `GetObjectClass`, `GetObjectChangeUp`, `GetObjectLevel`, `GetObjectLevelUpPoint`, `GetObjectMoney`, `GetObjectStrength`, `GetObjectDexterity`, `GetObjectVitality`, `GetObjectEnergy`, `GetObjectLeadership`, `GetObjectExtra*`, `GetObjectDefault*`, `GetObjectLive`, `GetObjectLife`, `GetObjectMaxLife`, `GetObjectMana`, `GetObjectMaxMana`, `GetObjectBP`, `GetObjectMaxBP`, `GetObjectShield`, `GetObjectMaxShield`, `GetObjectPKCount`, `GetObjectPKLevel`, `GetObjectPKTimer`, `GetObjectMap`, `GetObjectMapX`, `GetObjectMapY`, `GetObjectAuthority`, `GetObjectPartyNumber`, `GetObjectGuildNumber`, `GetObjectGuildStatus`, `GetObjectGuildName`, `GetObjectGuildRelationship`, `GetObjectGuildUnionNumber`, `GetObjectGuildUnionName`, `GetObjectChange`, `GetObjectInterface`, `GetObjectMasterLevel`, `GetObjectMasterPoint`, `GetObjectAccountLevel`, `GetObjectAccountExpireDate`, `GetObjectReset`, `GetObjectMasterReset`, `GetObjectGensRank`, `GetObjectGensSymbol`, `GetObjectGensFamily`, `GetObjectGensContribution`, `GetObjectCSGuildSide`, `GetObjectOfflineFlag`, `GetObjectIndexByName(name)`.

**Setters de objeto** (`(aIndex, valor)`): `SetObjectLevel`, `SetObjectLevelUpPoint`, `SetObjectMoney`, `SetObjectStrength`, `SetObjectDexterity`, `SetObjectVitality`, `SetObjectEnergy`, `SetObjectLeadership`, `SetObjectChatLimitTime`, `SetObjectPKCount`, `SetObjectPKLevel`, `SetObjectPKTimer`, `SetObjectMap`, `SetObjectMapX`, `SetObjectMapY`, `SetObjectMasterLevel`, `SetObjectMasterPoint`.

**Items:** `GetItemName()`, `InventoryGetWearSize`, `InventoryGetMainSize`, `InventoryGetFullSize`, `InventoryGetItemTable`, `InventoryGetItemIndex`, `InventoryGetItemCount`, `InventoryDelItemIndex`, `InventoryDelItemCount`, `InventoryGetFreeSlotCount`, `InventoryCheckSpaceByItem`, `InventoryCheckSpaceBySize`, `ItemDrop`, `ItemDropEx`, `ItemGive`, `ItemGiveEx`, `MapGetItemTable`.

**Mensajes / notificaciones:** `ChatTargetSend`, `MessageSend`, `MessageSendToAll`, `MessageGlobalSend`, `NoticeSend`, `NoticeSendToAll`, `NoticeGlobalSend`, `MessageGet` (MOD), `PostSend`, `FireworksSend`, `LevelUpSend`, `MasterLevelUpSend`, `MoneySend`, `PKLevelSend`, `SkinChangeSend`, `UserInfoSend`, `UserActionSend`, `UserSetAccountLevel`, `UserDisconnect`, `UserGameLogout`, `UserCalcAttribute`.

**Monstruos / mapa:** `MonsterCount`, `MonsterCreate`, `MonsterDelete`, `MonsterSummonCreate`, `MonsterSummonDelete`, `MoveUser`, `MoveUserEx`, `MapCheckAttr`, `GetMapName`, `GetMonsterName`.

**Party / guild:** `PartyGetMemberCount`, `PartyGetMemberIndex`.

**Comandos / permisos / config:** `CommandCheckGameMasterLevel`, `CommandGetArgNumber`, `CommandGetArgString`, `CommandSend`, `ConfigReadNumber`, `ConfigReadString`, `ConfigSaveString`, `PermissionCheck`, `PermissionInsert`, `PermissionRemove`.

**Efectos / estados:** `EffectAdd`, `EffectDel`, `EffectCheck`, `EffectClear`, `QuestStateCheck`, `RandomGetNumber`, `GetObjectCoin`, `ObjectAddCoin`, `ObjectSubCoin`.

**SQL (síncrono y async):** `SQLConnect`, `SQLDisconnect`, `SQLCheck`, `SQLQuery`, `SQLClose`, `SQLFetch`, `SQLGetResult`, `SQLGetNumber`, `SQLGetSingle`, `SQLGetString` · `SQLAsyncConnect`, `SQLAsyncDisconnect`, `SQLAsyncCheck`, `SQLAsyncQuery`, `SQLAsyncGetResult`, `SQLAsyncGetNumber`, `SQLAsyncGetSingle`, `SQLAsyncGetString` (resultado en `OnSQLAsyncResult`).

---

## 5. MAPEO CLIENTE ↔ SERVIDOR (qué hay en cada lado por dominio)

| Dominio | Cliente (confirmado) | Servidor (disponible) | Observación |
|---|---|---|---|
| Personaje | `Client.Level/HP/MP/Money/Map/...` | `GetObjectLevel/Life/Mana/Money/Map/...` | 1:1 de datos |
| Chat / avisos | `Draw.Message`, `on_packet` (C1:0D) | `NoticeSend`, `MessageSend` | Canal [LUA:...] ya probado |
| Comandos | `SendPacket` (chat C1:00) | `OnCommandManager` + `CommandManager.txt` | `/luatest` ya probado |
| Movimiento/ataque | `UI.*` (bloqueo), `on_click` | `OnUserMove`, `OnCheckUserTarget` | El bloqueo de UI es 100% cliente |
| Muerte/PK | (no directo) | `OnUserDie`, `OnMonsterDie`, `OnUserRespawn`, `OnCheckUserKiller` | El cliente puede reaccionar a C1:0D/estado |
| Items | `IsTradeOpen`, cache parcial; **inventario NO CONFIRMADO en cliente** | `OnUserItemPick/Drop/Move`, `Inventory*`, `ItemGive` | **El server es la fuente de verdad de items** |
| Trade | `IsTradeOpen/Accepted`, `GetTradeItem/Money` (parcial) | `OnUserItemMove` flag 1, `MoneySend` | El server valida todo el trade |
| NPC | `GetNpcIndex/Name` NO CONFIRMADO en cliente | `OnNpcTalk`, `GetObjectName` del NPC | Diálogos y eventos de NPC **en server** |
| Party | (no directo) | `OnPartyEntry/Close`, `PartyGetMember*` | Eventos de party en server |
| Timer | `on_draw` por frame (60fps) | `OnTimerThread` (1s) | Dos relojes distintos disponibles |
| Persistencia | `Lua\config.txt` (cliente) | `ConfigRead/Write`, `SQLAsync*` | DB del server para datos compartidos |
| Monstruos | (el cliente los ve por packets) | `MonsterCreate/Delete`, `OnMonsterDie` | Spawns y eventos de caza en server |

---

## 6. PATRONES DE EVENTOS CUSTOM (ejemplos en ambos lados)

Regla de oro: **el server es la autoridad** (valida y premia). El cliente solo **detecta y muestra** (feedback visual + datos en pantalla).

### Evento A — "Botín legendario al matar monstruo X"

```lua
-- SERVER (Script\EventoX.lua, registrado en ScriptMain.lua)
BridgeFunctionAttach('OnMonsterDie','EventoX_OnMonsterDie')

function EventoX_OnMonsterDie(mIdx, uIdx)
    if GetObjectType(mIdx) ~= 0 then return end          -- 0 = monstruo
    if GetMonsterName(mIdx) == 'Dark Lord' then          -- matar al boss
        ItemGive(uIdx, 14, 13, 0, 0, 0, 1, 0)            -- dar item (Ej: Sworn/Exc)
        NoticeSend(uIdx, 1, "[LUA:LOOT] Has obtenido el botin legendario!")
        FireworksSend(uIdx)                              -- celebración visible
    end
end
```

```lua
-- CLIENTE (en on_packet) -> feedback local al recibir el aviso del server
function on_packet(head, sub, data_hex)
    if head == 0x0D and sub == 0x00 then
        -- data_hex contiene "LUA:LOOT ..." -> mostrar overlay/banner
    end
end
```

### Evento B — Comando custom /top (top 3 por nivel, desde DB)

```lua
-- SERVER: añadir /top en Data\CommandManager.txt con code=201
BridgeFunctionAttach('OnCommandManager','Top_OnCommandManager')

function Top_OnCommandManager(aIndex, code, arg)
    if code ~= 201 then return 0 end
    NoticeSend(aIndex, 1, "[LUA:TOP] El top lo consulta el server via SQL")
    SQLAsyncQuery("SELECT Name,Level FROM Character ORDER BY Level DESC LIMIT 3", "TOP")
    return 1
end

function Top_OnSQLAsyncResult(label, param, result)
    if label ~= "TOP" then return end
    -- result: filas de la consulta -> enviar a cada jugador con NoticeSend
end
```

### Evento C — Veto de recoger items raros sin requisito (evento mundo)

```lua
-- SERVER: bloquear drop de items prohibidos (inventario lleno de duplicados)
BridgeFunctionAttach('OnUserItemPick','AntiFarm_OnUserItemPick')

function AntiFarm_OnUserItemPick(aIndex, slot)
    local idx = InventoryGetItemIndex(aIndex, slot)
    if idx == 14 and InventoryGetItemCount(aIndex, slot) > 20 then
        NoticeSend(aIndex, 1, "[LUA:FARM] Limite de ese item alcanzado")
        return 0   -- 0 = bloqueado
    end
    return 1
end
```

### Evento D — HUD del cliente con datos del server (estado de evento)

```lua
-- SERVER: cada 5 segundos, estado del evento a los jugadores (canal C1:0D)
-- CLIENTE: on_packet(0x0D) filtra "[LUA:EVT]" -> actualiza la ventana custom
--   (el framework UI del cliente ya existe: Lua\main.lua, abrir con Insert)
```

### Evento E — Aviso visual local en el cliente (sin server)

```lua
-- CLIENTE: cuando el jugador entra al mapa 7 (Lorencia) con HP bajo
function on_draw(cx, cy)
    if Client.InGame() and Client.Map() == 7 and Client.HP() < Client.MaxHP() * 0.3 then
        Draw.Bar(10, 10, 200, 22, 255, 0, 0, 160)
        Draw.Text(14, 14, "HECHIZO DE VIDA BAJO - CUIDADO", 255, 200, 0)
    end
end
```

---

## 7. REGLAS PARA LA IA RECEPTORA (no romper nada)

1. **No inventar offsets** del cliente. Si falta algo, usar `RegisterPacketHandler`/`on_packet` o el canal `[LUA:...]` del server.
2. **No modificar** ShaderPlugin, Shader.ini, post.fs, ni el binario del GameServer (no hay source completo).
3. **No usar F1-F12** como hotkeys del cliente (ocupadas por skills). Insert/Home/PgUp libres.
4. **Todo callback del cliente debe ir en `pcall`** (un error no debe congelar el input ni crashear).
5. **Todo lo NO CONFIRMADO del cliente devuelve nil/false/-1** y loguea una vez — no asumirlo como funcional.
6. **Server**: respetar la semántica de retorno de cada bridge (1=permitir en OnCheck*/OnUserItem*; 1=manejado en OnCommandManager/OnNpcTalk).
7. **Despliegue server**: crear `Script\MiScript.lua` + `require('Script\MiScript')` en `ScriptMain.lua` + recargar con `/reload` (GM) o reiniciar el GameServer.
8. **Despliegue cliente**: copiar `Lua.dll` junto a main.exe (y ejecutar `GetMainInfo.exe` para el CRC) y `Lua\main.lua` a la carpeta `Lua\`; log en `Lua\lua_plugin.log`.
9. **Validación**: probar por pasos, con evidencia (log + pantalla). Si algo falla → falla suave: log + feature desactivada, nunca crash.
10. Este documento es la fuente de verdad de la API confirmada; los detalles finos de implementación están en `PROGRESO_LUA.txt` y `REPORTE_FUNCIONES_CLIENTE.txt`.

---

## 8. ENTREGABLES DE REFERENCIA (rutas)

| Archivo | Ruta |
|---|---|
| Script del cliente (UI framework + demo) | `Lua\main.lua` (repo) · `Package_SSeMU_S6_ENG\Lua\main.lua` (despliegue) |
| Plugin cliente | `LuaPlugin\Output\Lua.dll` (v0.3.2) |
| Log cliente en vivo | `Package_SSeMU_S6_ENG\Lua\lua_plugin.log` |
| Config cliente | `Lua\config.txt` |
| Harness de tests del cliente | `LuaPlugin\test\test_lua.cpp` (TEST OK v0.3.2, 48 checks) |
| Scripts del server | `MuServerS6Evercion maxima\Data\Script\` (`ScriptMain.lua`, `System\ScriptCore.lua`, `Script\TemplateScript.lua`, `Script\LuaPluginDemo.lua`) |
| Docs oficiales del server | `MuServerS6Evercion maxima\Tools\Guides\Script Lua Interface Functions.rtf` · `Script Lua BridgeFunctions.rtf` |
| Progreso / reporte | `PROGRESO_LUA.txt` · `REPORTE_FUNCIONES_CLIENTE.txt` |
