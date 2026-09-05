-- ============================================================================
-- prod_check.lua - valida la VERSION DE PRODUCCION del cliente (nucleo + customs)
-- ============================================================================
-- Uso:  cd LuaPlugin\test  &&  test_lua.exe prod_check.lua
--
-- El harness (test_lua.exe) define los stubs del juego (Draw/Client/Input/
-- Interface/UI/Log/SendPacket/...) y CONFIG_PATH_OVERRIDE ANTES de cargar
-- este script, igual que en el juego real.
--
-- Este validador:
--   1) carga Lua\main.lua (nucleo) con LUA_CUSTOMS_DIR apuntando a la carpeta
--      real del custom (en el juego esa variable no existe: el nucleo usa
--      "Lua\forge" relativo al main.exe);
--   2) escanea el codigo fuente del NUCLEO y de la CARPETA de customs buscando
--      cualquier token de codigo de prueba/dev (si apareciera, un user podria
--      activar paneles de test editando config.txt);
--   3) verifica que el custom "Forja Custom" quedo registrado y funcionando.
--
-- NOTA: los tests hardcodeados del harness NO aplican a esta invocacion
-- (esperan la version de desarrollo all-in-one). Se ignoran: leer solo las
-- lineas [PROD-CHECK] del output. Los Lua maestros viven en forja\cliente
-- (lua_scripts fue eliminado al reorganizar el proyecto).
-- ============================================================================

-- Base de customs en el entorno de test (el nucleo lee esta global; en el
-- juego real no existe y el nucleo usa "Lua" relativo al main.exe)
LUA_CUSTOMS_DIR = "..\\..\\forja\\cliente"

local PROD     = "..\\..\\forja\\cliente\\main.lua"
local CUSTOM   = LUA_CUSTOMS_DIR .. "\\forge\\main.lua"

local okCnt, failCnt = 0, 0

local function check(name, cond, detail)
    if cond then
        okCnt = okCnt + 1
        print("[PROD-CHECK] ok   " .. name .. (detail and ("  (" .. tostring(detail) .. ")") or ""))
    else
        failCnt = failCnt + 1
        print("[PROD-CHECK] FAIL " .. name .. (detail and ("  (" .. tostring(detail) .. ")") or ""))
    end
end

-- 1) El NUCLEO carga sin errores (mismo entorno que el juego) y arranca el
--    cargador de customs
local okL, errL = pcall(dofile, PROD)
check("nucleo main.lua carga sin errores", okL, okL and "ok" or tostring(errL))
if not okL then
    print("[PROD-CHECK] RESULTADO: " .. okCnt .. "/" .. (okCnt + failCnt) .. " OK")
    return
end

-- 2) El custom de la carpeta se cargo (CUSTOM_FORGE lo expone)
check("custom 'forge' cargado por el nucleo", type(CUSTOM_FORGE) == "table", type(CUSTOM_FORGE))

-- 3) El CODIGO FUENTE (nucleo + custom) no contiene NINGUN token de dev/test
local function readFile(path)
    local f = io.open(path, "r")
    if not f then return "" end
    local s = f:read("*a")
    f:close()
    return s
end
local src = readFile(PROD) .. "\n" .. readFile(CUSTOM)
local forbidden = {
    "CustomUI Demo", "ui_enabled", "Sistema Custom Demo", "Diagnostico",
    "Diag", "PCache", "inv_window", "panel_key", "Calibrar", "PING",
    "WorldCup", "mundial", "wc_key", "[LUA:WC]", "MUNDIAL",
}
for _, pat in ipairs(forbidden) do
    check("sin codigo dev/custom retirado: '" .. pat .. "'", src:find(pat, 1, true) == nil)
end

-- 4) Solo existe la ventana del custom (Forja Custom)
local titles = {}
for _, w in ipairs(UI_TEST.ui.windows) do titles[#titles + 1] = w.title end
check("solo existe la ventana 'Forja Custom'",
    #titles == 1 and titles[1] == "Forja Custom", table.concat(titles, ", "))

-- 5) Config de produccion
check("Config.version == 5", UI_TEST.config.version == "5", tostring(UI_TEST.config.version))
check("config.txt creado con defaults al cargar", io.open(UI_TEST.config.path, "r") ~= nil)

-- 6) El canal [LUA:MIX] esta registrado en CORE.notice (lo usa el custom)
check("CORE.notice.MIX registrado", type(UI_TEST.core.notice.MIX) == "function",
    type(UI_TEST.core.notice.MIX))

-- 7) on_draw estable (3 frames sin errores, incluye hooks del custom)
local drawOk = true
for i = 1, 3 do
    local ok = pcall(on_draw, 320, 240)
    if not ok then drawOk = false end
end
check("on_draw sin errores (nucleo + hooks del custom)", drawOk)

-- 8) on_packet consume [LUA:MIX] y lo reparte al custom (OPEN muestra el panel)
local msg = "[LUA:MIX] OPEN"
local hex = "C1 " .. string.format("%02X", 13 + #msg) .. " 0D 01"
for i = 1, 9 do hex = hex .. " 00" end
for i = 1, #msg do hex = hex .. " " .. string.format("%02X", msg:byte(i)) end
check("notice [LUA:MIX] consumido", on_packet(0x0D, 0x01, hex) == true)
check("panel Forja visible tras OPEN", CUSTOM_FORGE.panel.visible == true,
    tostring(CUSTOM_FORGE.panel.visible))

-- 9) RECIPE parseado (recetas cargadas en el estado del custom)
local msg2 = "[LUA:MIX] RECIPE|1|Espada Legendaria|5000000|80|14:3:1:Item14;7:0:5:Item7|14:5:255:1:0:0:0:Item14"
local hex2 = "C1 " .. string.format("%02X", 13 + #msg2) .. " 0D 01"
for i = 1, 9 do hex2 = hex2 .. " 00" end
for i = 1, #msg2 do hex2 = hex2 .. " " .. string.format("%02X", msg2:byte(i)) end
check("RECIPE consumido", on_packet(0x0D, 0x01, hex2) == true)
check("receta 1 parseada", #CUSTOM_FORGE.state.recipes >= 1
    and CUSTOM_FORGE.state.recipes[1].name == "Espada Legendaria",
    tostring(#CUSTOM_FORGE.state.recipes) .. " recetas")

-- 10) STOCK parseado (materiales del jugador)
local msg3 = "[LUA:MIX] STOCK|14:3:1;7:0:5"
local hex3 = "C1 " .. string.format("%02X", 13 + #msg3) .. " 0D 01"
for i = 1, 9 do hex3 = hex3 .. " 00" end
for i = 1, #msg3 do hex3 = hex3 .. " " .. string.format("%02X", msg3:byte(i)) end
check("STOCK consumido", on_packet(0x0D, 0x01, hex3) == true)
check("stock 14:3 = 1", CUSTOM_FORGE.state.stock["14:3"] == 1,
    tostring(CUSTOM_FORGE.state.stock["14:3"]))

-- 11) El click dentro del panel lo consume la UI (el juego NO ataca/camina)
UI_TEST.ui.show(CUSTOM_FORGE.panel)
local wcx, wcy = CUSTOM_FORGE.panel.x + 20, CUSTOM_FORGE.panel.y + 40
check("click dentro del panel consumido por la UI",
    UI_TEST.ui.handleClick(wcx, wcy, 1, true) == true)

-- 12) Un notice con TAG desconocido se consume sin romper (router robusto)
local msg4 = "[LUA:XXX] algo"
local hex4 = "C1 " .. string.format("%02X", 13 + #msg4) .. " 0D 01"
for i = 1, 9 do hex4 = hex4 .. " 00" end
for i = 1, #msg4 do hex4 = hex4 .. " " .. string.format("%02X", msg4:byte(i)) end
check("notice [LUA:XXX] (tag desconocido) se consume sin errores",
    on_packet(0x0D, 0x01, hex4) == true)

print("[PROD-CHECK] RESULTADO: " .. okCnt .. "/" .. (okCnt + failCnt) .. " OK")
