================================================================================
 LuaPlugin - DLL plugin con Lua 5.2 embebido para cliente MU EX603 (SSeMU)
 v0.3.2
================================================================================

NOVEDADES v0.3.2 (sobre la v0.3.1): CAUSAS RAIZ DEL PROBLEMA EN VIVO
  El problema en vivo persista (algunos clicks seguian moviendo al personaje y
  botones sin feedback). Causas raiz atacadas en v0.3.2:

  1) SENDGUARD AUTO-REPARABLE: main.dll puede escribir MySend en
     CLIENT_SEND_POINTER DESPUES de cargar los plugins (CheckPluginFile corre
     dentro de su EntryProc, antes de HackCheck init). Si eso pasa, el guard de
     v0.3.1 quedaba saltado y los packets de movimiento volvian a salir. Ahora
     un chequeo por frame (1 DWORD, coste nulo) re-instala el guard en cuanto
     el puntero cambia o cuando MySend aparece por primera vez (instalacion
     tardia). Verificado en el source viejo: HackCheck.cpp:98 hace
     SetDword(0x00D227F8,&MySend) y MySend cifra el buffer EN EL PUNTERO
     (EncryptData in-place) -> en el guard los packets C1 llegan PLANOS
     (buff[0]==0xC1), el criterio de bloqueo es correcto.
  2) WNDPROC GUARD POR PROCESO: ahora se subclasean TODAS las ventanas
     top-level visibles del proceso via EnumWindows (normalmente una sola, la
     del juego), con chaining por ventana. MAIN_WINDOW queda solo como fallback:
     si ese offset no contuviera el HWND real en tu build, la capa 2 ya no
     queda muerta.
  3) DIAGNOSTICO DE COORDENADAS: UI.CursorInsideUI() -> bool y la linea
     "Cursor: (x,y) dentro=SI/NO" del panel SYS. Si al clickear la ventana
     dice dentro=NO, los rects no coinciden con el cursor y el bloqueo no
     puede funcionar (ahi esta el fallo).
  4) FEEDBACK VISIBLE: todos los botones usan Notify() = LogUI + Draw.Message
     -> cada pulsacion muestra un mensaje [LUA] en el chat del juego (ningun
     boton vuelve a parecer "mudo").
  OJO: NO hay re-assert del WndProc (a diferencia del puntero de envio): si
  otro modulo subclasea ENCIMA, su CallWindowProc encadena a nuestro
  LuaWndProc (la capa sigue activa); re-afirmar crearia recursion infinita.

NOVEDADES v0.3.1 (sobre la v0.3.0): BLOQUEO DE CLICKS EN 3 CAPAS
  Problema en vivo reportado: al clickear las ventanas custom el personaje se
  movia y los botones no respondian (el click llegaba al juego y/o el input de
  Lua quedaba mudo). Solucion aplicada en 3 capas independientes + robustez:

  CAPA 1 - hook del call site de MouseClick (ya existia en v0.3.0): consume el
           click si el cursor esta sobre un rect de UI. Se conserva.
  CAPA 2 - WndProc guard (NUEVO): subclase del WndProc del cliente (patron
           TrayMode del Main_EX603, con chaining via CallWindowProc al wndproc
           previo de main.dll). Traga WM_LBUTTONDOWN/UP/DBLCLK, WM_RBUTTONDOWN/UP
           y WM_MBUTTONDOWN/UP cuando el cursor esta sobre un rect de UI: el
           juego NI SIQUIERA procesa el click (no camina, no ataca, no toca las
           ventanas nativas de detras). El input de Lua no depende del WndProc
           (se sondea con GetAsyncKeyState), asi que los botones siguen
           respondiendo. Se instala LAZY desde DrawLuaUI (primer frame de
           render, cuando la ventana ya existe y main.dll instalo sus hooks).
  CAPA 3 - SendGuard (NUEVO): intercepta el puntero de envio del cliente
           (CLIENT_SEND_POINTER, donde main.dll pone su MySend). Si el cursor
           esta sobre un rect de UI y el packet es C1:04 (ataque) / C1:05
           (mover) / C1:06 (cancelar), lo TRAGA y devuelve exito simulado:
           el personaje NO se mueve ni ataca, sin importar como proceso el
           click el juego. Solo actua cuando buff[0]==0xC1 y op en {04,05,06}
           (buffer plano): si el buffer viniera ya cifrado no bloquea nada
           (cero riesgo). Se instala solo si el puntero actual es ejecutable y
           NO es el send crudo de ws2_32.
  ROBUSTEZ del input - on_key/on_click: un error puntual YA NO deja el input
           mudo para siempre: se cuentan fallos CONSECUTIVOS y solo tras 5 se
           desactiva (script roto). El gate de foco ahora usa IsWindow: si el
           handle de MAIN_WINDOW no es valido en un build, el polling sigue
           activo en vez de bloquear on_click.
  DIAGNOSTICO - la DLL expone el estado real de cada capa:
           UI.WndProcInstalled() / UI.SendGuardInstalled() /
           UI.BlockedByHook() / UI.BlockedByWndProc() / UI.BlockedBySend()
           El panel Sistema Custom Demo las muestra en vivo (si una capa no
           quedo instalada se ve "hook NO | wnd NO | send NO" y sabes que el
           click llega al juego por esa via).

NOVEDADES v0.3.0 (sobre la v0.2.0)
  - UI.BlockMouse(x,y,w,h) / ClearBlockedRects() / SetMouseBlockEnabled(bool) /
    IsMouseInside(x,y,w,h) / ConsumeClick() / IsMouseBlockHooked() /
    MouseClickCallSite(): bloqueo de clicks sobre UI custom
    (hook del CALL SITE que invoca MouseClick 0x007D2920, mismo patron que los
    hooks de render/packets: localizacion por 0x007D2B0C o escaneo del .text,
    chaining si main.dll ya lo hookeo, __try/__except, y desactivacion elegante
    si no se encuentra el call site. La logica de rects esta en MouseBlock.h,
    compartida y testeada por el harness).
  - Interface.GetOpenWindows() (tabla de ids abiertos) / GetActiveWindow()
    (NO CONFIRMADO -> -1 + log una vez).
  - Draw.Tooltip(x, y, texto) -> tooltip inmediato del cliente (0x00597220).
  - Client.IsTradeOpen() / IsTradeAccepted() / GetTradeMoney() / GetTradeItem(slot):
    estado y items del trade cacheados de los packets del servidor
    (C3:36, C1:37/39/3B/3C/3A/3D) con el parser compartido ClientStateCache.h.
  - Client.IsShopOpen() (C1:3F:02/03/12) e IsChaosBoxOpen() (evidencia C1:88).
  - Funciones NO CONFIRMADAS (slots de inventario/wear/shop/chaos, NPC dialog):
    registradas pero devuelven nil/false/-1/"" y loguean UNA vez (offset de slots
    no documentado en el source viejo; ver REPORTE_FUNCIONES_CLIENTE.txt).
  - main.lua v0.3.0: framework de UI en Lua puro (ventanas arrastrables con
    botones/barras/checkboxes/tooltips/z-order/hotkeys), config persistente en
    Lua\config.txt (clave=valor versionada, se regenera con defaults),
    calibracion de ventanas nativas (inventario/NPC) en 2 pasos, cache de
    estados por packets y paneles demo (CustomUI Demo / Configuracion /
    Sistema Custom Demo). Hotkey del panel: Insert (0x2D).

QUE ES
  DLL que se inyecta en el cliente usando el SISTEMA DE PLUGINS del emulador
  (main.dll -> gProtect.CheckPluginFile() -> LoadLibrary + EntryProc).
  El plugin embele Lua 5.2 (la misma version que usa tu GameServer) y expone
  funciones del cliente a los scripts (dibujo, cursor, estado, etc.).

ESTRUCTURA
  LuaPlugin/
    LuaPlugin.sln / .vcxproj / .filters   <- proyecto (mismas convenciones que Main.vcxproj)
    src/
      LuaPlugin.cpp     <- DllMain, EntryProc, Lua, hook de render, API
      LuaPlugin.h
      Offset.h          <- direcciones del cliente EX603
      Util.h / Util.cpp <- SetByte/SetWord/SetDword/SetCompleteHook/MemoryCpy/MemorySet
      stdafx.h / stdafx.cpp
    lua/
      lua52.lib         <- copiado de SOURCE viejos/Util/lua/ (Lua 5.2)
      *.h               <- headers de Lua 5.2
    lua_scripts/
      main.lua          <- DEMO cliente (panel, botones, on_key/on_click, router)
    server_scripts/
      LuaPluginDemo.lua <- DEMO servidor (GameServer SSeMU): /luatest + heartbeats
    test/
      test_lua.cpp / build_test.bat / test_lua.exe
    Output/             <- aqui cae Lua.dll al compilar

COMO COMPILAR (PROBADO - COMPILA OK)
  Compilado y verificado en esta maquina: VS2022 MSBuild + toolset v100 (VS2010)
  -> Output\Lua.dll (147 KB) con EntryProc exportada.

  1) Abrir LuaPlugin.sln en Visual Studio (2013+ o VS2022; toolset v100, igual
     que el proyecto Main del emulador). Si tu VS no tiene el toolset v100,
     cambia PlatformToolset a v141/v142/v143 en las dos configuraciones.
     Tambien se puede compilar por linea de comandos:
       msbuild LuaPlugin.vcxproj -p:Configuration=Release -p:Platform=Win32
  2) Build Release|Win32. El resultado: Output\Lua.dll.
     NOTA: compila SIEMPRE en Release. La libreria lua52.lib del pack esta
     compilada con /MT; la config Debug usa /MDd y daria LNK4098 (mismatch de
     CRT). La config Debug existe solo como referencia.
     El warning LNK4098 (LIBCMT vs MSVCRT) al linkar lua52.lib en Release es
     BENIGNO: Lua resuelve sus llamadas al CRT dentro del propio DLL de forma
     consistente (el GameServer del pack enlaza la misma lib con el mismo /MD).
  3) La libreria Lua 5.2 ya viene incluida en lua/ (copiada del pack original).

TEST STANDALONE DEL SCRIPT (sin el cliente)
  test/ contiene un harness que simula la API del plugin con stubs (imprime en
  consola) y ejecuta main.lua de verdad con la misma lua52.lib:
    cd test
    build_test.bat      -> compila test_lua.exe (requiere VS2010/v100)
    test_lua.exe        -> carga main.lua y valida TODO:
      - on_draw (frames + rama mod 500)
      - router (C1:FC:01 consume y responde; C1:FC:03 consume; tras
        UnregisterPacketHandler el packet vuelve a pasar)
      - on_packet global (notice C1:0D con [LUA consume; notice normal y
        C1:26 pasan)
      - input (Insert -> on_key toggle; click en PING -> SendChat("/luatest")
        genera el packet C1:00 con name[10] correcto)
  Resultado de la prueba en esta maquina: TEST OK, 0 errores.

COMO REGISTRARLA COMO PLUGIN (via GetMainInfo)
  1) Copia Lua.dll a la carpeta .\Path\ de tu GetMainInfo (junto a main.exe y
     main.dll). Si GetMainInfo usa otra ruta, cambia OutDir del proyecto a esa.
  2) En MainInfo.ini (carpeta de GetMainInfo) anade:
       [MainInfo]
       ClientName=main.exe
       PluginName1=Lua.dll
       (PluginName2/3 disponibles si ya usas otros plugins)
  3) Ejecuta GetMainInfo.exe -> regenera .\Path\ServerInfo.sse (incluye el CRC
     de Lua.dll; si el CRC no cuadra, el cliente se cierra con ExitProcess).
  4) Distribuye: main.exe + main.dll + Lua.dll + ServerInfo.sse + la carpeta
     Lua\ con main.lua (y opcionalmente el log).

COMO SE EJECUTA
  main.exe (parcheado) -> carga main.dll -> EntryProc() de main.dll ->
  gProtect.CheckPluginFile() -> LoadLibrary("Lua.dll") -> GetProcAddress
  ("EntryProc") -> EntryProc() de ESTE plugin (hilo principal del cliente, con
  los hooks base de main.dll ya instalados).

  En EntryProc el plugin:
    - Instala el hook de render en RENDER_HOOK_OFFSET (0x005B96E8) CON CHAINING:
      si main.dll ya habia hookeado esa direccion, se encadena su funcion en vez
      de romperla (las barras de vida custom del emulador siguen funcionando).
    - Crea el estado Lua 5.2, registra la API y carga Lua\main.lua.
  Cada frame dentro del juego, el hook llama a on_draw(cursor_x, cursor_y) del
  script.

API LUA DISPONIBLE
  Draw.Text(x, y, texto, r, g, b)          -> pSetTextColor + pDrawText
  Draw.Bar(x, y, w, h, r, g, b, a)         -> pSetBlend + glColor4f + pDrawBarForm
  Draw.Message(texto, tipo)                -> pDrawMessage
  Draw.Image(id, x, y, w, h, u0, v0, u1, v1, alpha)
                                           -> pDrawImage (UV 0..1 por defecto)
  Draw.LoadImage(path, w, h)               -> pLoadImage, devuelve el id de imagen
  Input.CursorX() / Input.CursorY()        -> pCursorX / pCursorY
  Input.RegisterKey(vk)                    -> suscribe una tecla para on_key (max 32)
  Input.KeyPressed(vk)                     -> estado actual de una tecla (bool)
  Interface.Open(wid) / Close(wid)         -> ventanas nativas (pOpenWindow/ClosekWindow)
  Interface.IsOpen(wid)                    -> bool (pCheckWindow)
  Client.InGame() / ScreenState()          -> MAIN_SCREEN_STATE (5 = en juego)
  Client.ResolutionX() / ResolutionY()
  Client.CharacterName()                   -> nombre del personaje (struct +0x00)
  Client.Level() / Class()                 -> nivel y clase (struct +0x0E / +0x0B)
  Client.HP() / MaxHP() / MP() / MaxMP()   -> vida/mana actual y max (struct +0x22/+0x26/+0x24/+0x28)
  Client.Shield() / MaxShield() / BP() / MaxBP()
  Client.Strength() / Dexterity() / Vitality() / Energy() / Leadership()
  Client.LevelUpPoint() / Experience() / NextExperience()
  Client.Money()                           -> zen (cacheado de los packets C3:F3:03 / C3:22:FE)
  Client.Map()                             -> mapa actual (MAIN_CURRENT_MAP)
  Todos los reads del struct son SEGUROS (VirtualQuery): si el puntero no es
  valido (login, build distinto) devuelven 0 en vez de crashear.
  SendPacket(hex)                          -> envia un packet al servidor (true/false)
  RegisterPacketHandler(head, sub|nil, fn) -> router de opcodes; devuelve id
  UnregisterPacketHandler(id)              -> quita un handler del router
  Log(mensaje)                             -> escribe en Lua\lua_plugin.log

  Ventanas nativas (window IDs comunes en EX603; verifica con IsOpen):
    0x02 = inventario, 0x03 = personaje, 0x08 = almacen, 0x09 = shop NPC.

INPUT: on_key / on_click
  El plugin SONDEA el input en el hook de render (no instala hooks de
  teclado/raton), por eso funciona aunque main.dll cargue los suyos despues
  de los plugins. Solo dispara dentro del juego y con el cliente en primer
  plano, y solo en TRANSICIONES de estado (sin spam por mantener pulsada).
    on_key(vk, pressed)      -> teclas suscritas con Input.RegisterKey(vk)
    on_click(x, y, boton, pressed)
         boton: 1 = izquierdo, 2 = derecho, 4 = medio (siempre activos)
  Si on_key/on_click lanzan un error, se loguea y se cuentan fallos
  CONSECUTIVOS; solo tras 5 fallos seguidos el input queda desactivado hasta
  reiniciar el cliente (un error puntual no deja el input mudo). Flag
  independiente de on_draw.

BLOQUEO DE CLICKS SOBRE UI CUSTOM (FASE 6, 3 capas - v0.3.1)
  El script registra cada frame los rectangulos de las ventanas visibles con
  UI.BlockMouse(x,y,w,h) (UI.ClearBlockedRects + SetMouseBlockEnabled). Con
  el cursor dentro de un rect y el bloqueo activo:
    - Capa 1: el hook del call site de MouseClick devuelve 0 (click consumido).
    - Capa 2: el WndProc guard traga los mensajes de raton (el juego no los ve).
    - Capa 3: el SendGuard traga los packets C1:04/05/06 (no sale nada a la red).
  Con el checkbox "Bloqueo de clicks en UI" del panel Configuracion (o
  mouse_block=0 en Lua\config.txt) se desactivan las tres a la vez
  (UI.SetMouseBlockEnabled(false)).
  VERIFICACION EN VIVO (imprescindible tras copiar la DLL):
    1) Abre el panel Sistema Custom Demo (boton SYS) y mira la linea "Capas".
    2) Debe mostrar "hook SI | wnd SI | send SI" con rects > 0.
    3) Con el cursor dentro de la ventana, la linea "Cursor" debe decir
       "dentro=SI". Si dice dentro=NO al clickear la ventana, los rects no
       coinciden con el cursor (coordenadas) -> dime el valor de (x,y).
    4) Clickea dentro de una ventana custom: el personaje NO debe moverse, la
       linea "Bloqueados" debe incrementar (wnd y/o send) y los botones
       muestran mensajes [LUA] en el chat.
    5) Si alguna capa dice NO, revisa Lua\lua_plugin.log al arrancar (ahi se
       registra tambien si main.dll re-escribio el puntero de envio y el guard
       se re-instalo).
  Si aun asi el personaje se mueve con el bloqueo activo, el problema es que el
  cursor (pCursorX/pCursorY) no coincide con las coordenadas de dibujo o que la
  ventana no esta registrada (rects=0); revisa la linea "Mouse block" del panel.
  Ejemplo:
    Input.RegisterKey(0x2D)              -- Insert (tecla del panel del demo)
    function on_key(vk, pressed)
        if pressed and vk == 0x2D then ... end
    end
    function on_click(x, y, btn, pressed)
        if pressed and btn == 1 and x > 8 and x < 90 and y > 96 and y < 114 then
            -- boton PING del panel
        end
    end
  NOTA IMPORTANTE sobre teclas: F9 (0x78) esta OCUPADA por el health bar de
  main.dll (KeyCodeHealthBarSwitch=120 en MainInfo.ini), y F8/F10/F11/F12 por
  autoattack/camara/tray. Para el toggle del panel el demo usa Insert (0x2D),
  libre en el cliente. Si cambias la tecla, ajusta PANEL_KEY en main.lua.

ROUTER DE OPCODES (RegisterPacketHandler)
  Registra handlers por head/sub para no pagar el coste de on_packet en cada
  packet que no te interesa. Se consulta ANTES que on_packet:
    local id = RegisterPacketHandler(0xFC, 0x01, function(head, sub, data)
        SendPacket("C1 07 FC 02 01 00 00")   -- responder
        return true                            -- consumir
    end)
    RegisterPacketHandler(0xFC, nil, fn)       -- wildcard: cualquier sub de 0xFC
    UnregisterPacketHandler(id)
  - Prioridad: handler exacto [head][sub] > wildcard [head][nil] > on_packet.
  - Solo se llama a UN consumidor por packet (si hay handler registrado y
    devuelve false, el packet sigue la cadena; on_packet NO se llama).
  - El sub se extrae segun el tipo (C1/C3: byte[3]; C2/C4: byte[4]).
  - Max 32 handlers; el id devuelto sirve para desregistrar.

ON_PACKET (recepcion de packets desde el servidor)
  Fallback global: se llama solo si NO hay handler registrado en el router.
  El script puede definir:
    function on_packet(head, sub, data_hex)
  - Se llama por CADA packet que llega del servidor (antes del dispatcher del
    cliente). head/sub se extraen segun el tipo (C1/C3: head=byte[3], sub=byte[4];
    C2/C4: head=byte[4], sub=byte[5]). Para packets sin sub, sub = -1.
  - data_hex = packet completo en hex (data[1]=tipo, data[2]=size, data[3]=head
    para C1). Sirve para parsear campos del packet.
  - DEVUELVE true para CONSUMIR el packet (el cliente original no lo procesa:
    ideal para opcodes custom que el cliente no entiende).
  - Devuelve false/nil para dejar pasar el packet (flujo normal).
  - OJO: corre por cada packet; filtra por head lo antes posible para no
    penalizar el lag (los strings hex son pequenos, pero suma).
  - Si on_packet lanza un error, se loguea una vez y los callbacks de packets
    quedan DESACTIVADOS hasta reiniciar el cliente (anti-spam; el packet que
    fallo se re-envia al flujo normal). Tras corregir el script hay que
    relanzar el cliente. on_draw tiene su propio flag independiente.
  Ejemplo:
    function on_packet(head, sub, data)
        if head == 0xFC and sub == 0x01 then
            SendPacket("C1 07 FC 02 01 00 00")   -- responder al servidor
            return true                            -- consumir
        end
        return false
    end

  COMO FUNCIONA INTERNAMENTE:
  - PROTOCOL_HOOK_OFFSET (0x0065FD79) es el call del dispatcher de packets;
    main.dll ya lo redirige a su ProtocolCoreEx. El plugin GUARDA ese destino
    y re-hookea con LuaProtocolCoreEx (SetCompleteHook 0xFF, igual que main.dll),
    encadenando: cliente -> LuaProtocolCoreEx -> ProtocolCoreEx de main.dll ->
    ProtocolCore (0x00663B20). Nunca se rompe el flujo de packets del cliente.
  - IMPORTANTE: el destino del chaining se valida con VirtualQuery (memoria
    EJECUTABLE), NO con un rango de direcciones fijo. main.dll es una DLL: el
    linker la ubica con base por defecto 0x10000000 (sus funciones viven en
    0x1000xxxx) o la reubica el ASLR en cualquier base alta. Un sanity-check
    tipo "target <= 0x10000000" la DESCARTARIA y romperia la cadena: al entrar
    al mundo el cliente no procesaria F3:0E0 (info del personaje), F1:00
    (hardware ID), F3:0E1/E2, 0xDE... -> desconexion al seleccionar el
    personaje y entrar al juego. Ver seccion PROBLEMA RESUELTO mas abajo.
  - Los packets llegan YA descifrados al dispatcher (el cliente descifra antes),
    asi que on_packet ve el protocolo plano; el envio se cifra en SendPacket.

PROBLEMA RESUELTO: DESCONEXION AL SELECCIONAR PERSONAJE / ENTRAR AL MUNDO
  Sintoma: login OK, lista de personajes OK, pero al entrar al juego el
  servidor desconecta al instante.
  Causa raiz (bug del plugin): el sanity-check del hook de packets usaba un
  rango fijo 0x00400000-0x10000000 para validar el destino del chaining.
  main.dll se carga como DLL (base por defecto 0x10000000, o cualquier base
  alta si el ASLR la reubica), asi que su ProtocolCoreEx estaba SIEMPRE por
  encima de 0x10000000 -> el plugin lo descartaba y encadenaba directo al
  ProtocolCore original del cliente, SALTEANDOSE los handlers custom de
  main.dll: F3:0E0 (GCNewCharacterInfoRecv, rellena el struct del personaje
  al entrar al mundo), F3:0E1 (calc), F1:00 (responde con el hardware ID
  F3:09), 0xDE, 0x11, 0x88... El cliente nunca completaba la entrada al
  mundo y el servidor lo desconectaba.
  Fix: InstallPacketHook valida el destino con VirtualQuery comprobando que
  apunta a memoria EJECUTABLE dentro del proceso (acepta cualquier base de
  modulo: main.exe a 0x00400000, main.dll en 0x1000xxxx o reubicada por
  ASLR). Si el destino no es ejecutable (caso extremo), recien ahi cae al
  fallback ProtocolCore.
  Nota adicional: tras recompilar Lua.dll SIEMPRE vuelve a ejecutar
  GetMainInfo (el CRC de ServerInfo.sse debe coincidir; si no, el cliente
  se cierra al arrancar).

DEMO CLIENTE <-> SERVIDOR (prueba de que todo funciona)
  Incluido en el proyecto: main.lua (cliente) + server_scripts/LuaPluginDemo.lua
  (servidor). Prueba el canal bidireccional SIN tocar el codigo C++ del
  GameServer (solo Lua + una linea de config):

  Servidor -> Cliente: NoticeSend -> packet C1:0D con prefijo "[LUA:".
                       El cliente lo intercepta en on_packet, lo muestra en el
                       panel y lo consume.
  Cliente  -> Servidor: boton PING del panel -> SendChat("/luatest") -> packet
                       C1:00 (chat con el nombre real del personaje, que el
                       GameServer verifica en CGChatRecv) -> OnCommandManager
                       -> el servidor responde con un notice "[LUA:CMD]...".
  Custom:    boton ALQ -> SendPacket("C1 FC ...") (requiere handler C++ en el
                       GameServer para que responda; ver abajo).

  DESPLIEGUE DEL SERVIDOR (LuaPluginDemo.lua):
    1) Copiar LuaPluginDemo.lua a:  Data\Script\Script\LuaPluginDemo.lua
    2) En Data\Script\ScriptMain.lua anadir:
         require('Script\LuaPluginDemo')
    3) En Data\CommandManager.txt anadir ANTES de la ultima linea "end"
       (misma estructura que las demas):
         200      1   1   1   1     "/luatest"        0        *         *          *          *          *          0           0           0           0           0           0           0           0           0           0           0           0           0           0           0           0
    4) Recargar scripts (comando /reload de GM) o reiniciar el GameServer.

  DESPLIEGUE DEL CLIENTE:
    1) Copiar main.lua como Lua\main.lua junto al main.exe del cliente.
    2) Entrar al juego: debe verse el panel "LuaPlugin Demo".
    3) Pruebas:
       - Insert (no F9: lo usa el health bar de main.dll) oculta/muestra el
         panel.
       - PING: envia /luatest; en pocos segundos el panel muestra
         "[LUA:CMD] <tu personaje> nivel=.. mapa=.. zen=.." (round-trip OK).
       - Cada 10 segundos llega "[LUA:HEARTBEAT] tick=.. online=..".
       - INV / CERRAR abren/cierran el inventario nativo (Interface).
       - MSG muestra un mensaje del cliente.
       - ALQ envia un packet C1:FC custom (para que el servidor responda
         necesitas el handler C++ del GameServer descrito abajo).

  SI PING NO RESPONDE (quedaba en "esperando"):
  Causa: SendChat rellenaba el nombre del personaje con ESPACIOS y el server
  compara con strcmp(name, lpObj->Name) -> no coincidia y descartaba el chat.
  Fix aplicado: el relleno ahora es con ceros (\0). Si aun asi no responde,
  revisa en Lua\lua_plugin.log que el nombre que envia el panel coincide
  exactamente con el del personaje.

  SI EL BOTON INV ABRE OTRA VENTANA (p.ej. el MoveList en vez del
  inventario): el window ID varia segun el build del cliente (en muchos
  EX603 0x02 = MoveList, no inventario). El boton INV tiene CALIBRACION
  AUTOMATICA en 2 pasos:
    1) Pulsa INV una vez (guarda el estado de ventanas abiertas).
    2) Abre el inventario con la tecla I del juego y pulsa INV otra vez.
       El plugin detecta la ventana nueva y guarda el ID solo (el panel
       muestra "INV: 0xXX (cal)" cuando queda calibrado).
  Si se abren varias ventanas a la vez, usa la primera y avisa en el log;
  para recalibrar, reinicia el cliente y repite el proceso. El boton SCAN
  sigue disponible para listar las ventanas abiertas en cualquier momento.

  OPCIONAL: handler C++ en el GameServer para packets custom C1:FC
  (el canal realmente custom, sin pasar por chat/notices):
    En Protocol.cpp de tu GameServer, dentro del switch de ProtocolCore,
    anadir para el head 0xFC un case que llame a una funcion registrada en
    el Lua del servidor (p.ej. gScriptLoader.OnCustomPacket(aIndex, lpMsg)).
    El cliente ya envia C1:FC desde el boton ALQ y responde al C1:FC:01
    con C1:FC:02 (ver main.lua). Opcodes libres tipicos: 0xFC / 0xFD.

SENDPACKET (envio de packets desde Lua)
  SendPacket("C1 05 BF 51 00")
  - El primer byte define la cabecera: C1/C2 = plano, C3/C4 = encriptado con
    serial (el plugin replica exactamente el DataSend de main.dll).
  - El campo de tamano se ajusta solo (buffer[1] o buffer[1..2]).
  - Devuelve true/false; los errores quedan en Lua\lua_plugin.log.
  Ejemplos:
    SendPacket("C1 03 04")               -> ataque
    SendPacket("C1 05 BF 51 00")         -> helper start
    SendPacket("C1 06 88 01 00 00 00")   -> pedir rate caja del caos

  COMO FUNCIONA INTERNAMENTE (importante):
  - Claves de cifrado: Data\Enc1.dat / Data\Dec2.dat (CPacketManager portado
    del Main_EX603: cifrado MU clasico 8->11 bytes + filtro XOR de cabecera).
  - El plugin NO llama a send() directo: usa el puntero de envio del cliente
    (CLIENT_SEND_POINTER 0x00D227F8), que main.dll ya sustituyo por MySend.
    MySend aplica el cifrado de stream (EncryptData, derivado de CustomerName
    + ClientSerial del ServerInfo.sse) a los puertos del juego y luego llama
    al send real. Usando ese puntero el canal queda 100% consistente con
    main.dll (mismo estado de cifrado y serial MAIN_PACKET_SERIAL).
  - El serial de C3/C4 se toma de MAIN_PACKET_SERIAL (0x08793700), el mismo
    contador que usa main.dll.
  - Socket activo: MAIN_ACTIVE_SOCKET (0x08793750) + 0x0C, leido en cada
    envio (sobrevive a cambios de mapa/reconexion).

DEPURACION
  Todo lo que hace el plugin queda en Lua\lua_plugin.log (junto al main.exe).
  Si el script falla al cargar o en on_draw, el error se escribe ahi.

ADVERTENCIAS IMPORTANTES
  * Las direcciones de Offset.h son del build EX603 (Season 6 Episode 3).
    Si tu main.exe es de otra version, ajusta RENDER_HOOK_OFFSET/RENDER_ORIGINAL
    y las demas direcciones (referencia: Offset.h del proyecto Main_EX603).
  * CADA VEZ QUE RECOMPILES Lua.dll, vuelve a ejecutar GetMainInfo (el CRC
    cambia y si no coincide, el cliente se cierra).
  * El anti-cheat MHPClient/MHPServer puede bloquear librerias no registradas:
    durante el desarrollo pruebalo con el anti-cheat desactivado o registrando
    el plugin oficialmente.
  * La libreria lua52.lib del pack esta compilada con /MD (MultiThreadedDLL),
    igual que este proyecto. No cambies RuntimeLibrary.

SIGUIENTES PASOS SUGERIDOS
  - Verificar en vivo las 3 capas de bloqueo (procedimiento arriba) y, si en
    tu build el cursor no coincide con el dibujo, ajustar las coordenadas de
    los rects en main.lua (UI.BlockMouse usa pCursorX/pCursorY del cliente).
  - Handler C++ en el GameServer para C1:FC: despachar packets custom al Lua
    del servidor (gScriptLoader.OnCustomPacket) y cerrar el canal custom puro
    (sin pasar por chat/notices).
  - Ventanas nativas: Interface.Open/Close/IsOpen con pOpenWindow/pClosekWindow/
    pCheckWindow + pWindowThis (Offset.h ya los tiene definidos).
  - Imagenes: pLoadImage + pDrawImage para sprites custom (Offset.h ya los tiene).
