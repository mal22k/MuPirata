-- ============================================================================
-- forge_ui_check.lua - validacion del rediseno UI de la Forja (estilo MU)
-- Uso: cd LuaPlugin\test && test_lua.exe forge_ui_check.lua
-- ============================================================================
LUA_CUSTOMS_DIR = "..\\..\\forja\\cliente"

local okCnt, failCnt = 0, 0
local function check(name, cond, detail)
    if cond then
        okCnt = okCnt + 1
        print("[UI-CHECK] ok   " .. name .. (detail and ("  (" .. tostring(detail) .. ")") or ""))
    else
        failCnt = failCnt + 1
        print("[UI-CHECK] FAIL " .. name .. (detail and ("  (" .. tostring(detail) .. ")") or ""))
    end
end

local okL, errL = pcall(dofile, "..\\..\\forja\\cliente\\main.lua")
check("nucleo carga", okL, okL and "ok" or tostring(errL))
if not okL then print("[UI-CHECK] RESULTADO: " .. okCnt .. "/" .. (okCnt + failCnt) .. " OK") return end
check("custom forge cargado", type(CUSTOM_FORGE) == "table")

-- Texturas por OpenGL (v0.6): con forge_textures=1 el nucleo carga BMP/TGA
-- via Draw.LoadImage (GL directo, sin pLoadImage nativo). El stub del harness
-- devuelve id 100 -> las texturas DEBEN cargarse (id > 0) y el fallback no aplica.
UI_TEST.config.set("forge_textures", "1")

local panel = CUSTOM_FORGE.panel
local st = CUSTOM_FORGE.state
local parse = CUSTOM_FORGE.parse

-- simular el flujo del server: OPEN + 7 recetas + STOCK
local function feed(line)
    parse(line)
end
feed("OPEN")
feed("RECIPE|1|Espada Legendaria|5000000|80|14:3:1:Espada de Plata;7:0:5:Piedra de Forja;12:0:1:Bendicion;22:0:2:Mineral;23:0:1:Gema Rara|14:5:255:1:0:0:0:Espada Legendaria")
feed("RECIPE|2|Anillo de Hielo|0|100|7:0:3:Piedra de Forja|21:0:255:1:0:0:0:Anillo de Hielo")
feed("RECIPE|3|Rune Blade Reforjada|5000000|70|7:0:5:Piedra de Forja;14:3:2:Espada de Plata|16:0:255:1:0:0:0:Rune Blade")
feed("RECIPE|4|Espada de la Destruccion|10000000|60|7:0:8:Piedra de Forja;14:4:2:Espada de Plata|17:0:255:1:0:0:0:Espada Destruccion")
feed("RECIPE|5|Espada Divina|10000000|50|7:0:10:Piedra de Forja;14:5:3:Espada de Plata;22:0:3:Mineral|18:0:255:1:0:0:0:Espada Divina")
feed("RECIPE|6|Arco Celestial|8000000|65|7:0:6:Piedra de Forja;14:2:2:Espada de Plata|19:0:255:1:0:0:0:Arco Celestial")
feed("RECIPE|7|Joya de la Forja|0|100|7:0:1:Piedra de Forja|20:0:255:1:0:0:0:Joya de la Forja")
feed("STOCK|14:3:1;7:0:3;22:0:2")

check("7 recetas parseadas", #st.recipes == 7, tostring(#st.recipes))
check("panel visible tras OPEN", panel.visible == true)
check("panel CENTRADO al abrir (800x600, panel 356x360 -> 222,120)", panel.x == 222 and panel.y == 120,
    panel.x .. "," .. panel.y)
check("stock 7:0 = 3", st.stock["7:0"] == 3)

-- on_draw aplica el rebuild pendiente (UNA vez por frame) sin errores. Tambien
-- dispara la carga LAZY de texturas (ensureMUTextures): se comprueba despues.
local drawOk = true
for i = 1, 5 do
    local ok = pcall(on_draw, 400, 300)
    if not ok then drawOk = false end
end
check("on_draw x5 sin errores (rebuild dirty + clamp + ensureMUTextures)", drawOk)

-- Carga por GL validada: con forge_textures=1 y el stub (LoadImage->100), las
-- texturas DEBEN cargarse (titlebar/slotbg > 0) y la carga es idempotente.
check("textura titlebar cargada por GL (forge_textures=1)",
    UI_TEST.ui.mu.titlebar > 0, tostring(UI_TEST.ui.mu.titlebar))
check("textura slotbg cargada por GL",
    UI_TEST.ui.mu.slotbg > 0, tostring(UI_TEST.ui.mu.slotbg))
UI_TEST.ui.ensureMUTextures()
check("carga lazy idempotente (2a llamada no re-carga ni rompe)",
    UI_TEST.ui.mu.tried == true, "tried=" .. tostring(UI_TEST.ui.mu.tried))

-- widgets del panel (sin receta seleccionada aun: no hay detalle)
local types = {}
local hasBox, hasSlot, hasFlat, hasNormal = false, false, false, false
for _, wid in ipairs(panel.widgets) do
    types[wid.type] = (types[wid.type] or 0) + 1
    if wid.type == "box" then hasBox = true end
    if wid.type == "slot" then hasSlot = true end
    if wid.type == "button" and wid.flat then hasFlat = true end
    if wid.type == "button" and not wid.flat then hasNormal = true end
end
check("botones flat (filas clickeables)", hasFlat)
check("botones CREAR normales", hasNormal)
local tstr = ""
for k, v in pairs(types) do tstr = tstr .. k .. "=" .. v .. " " end
check("panel con contenido", #panel.widgets > 10, tstr)

-- posicion dentro de la pantalla (clamp: panel x=380 + w=356 < 800)
check("panel dentro de pantalla", panel.x >= 2 and panel.x + panel.w <= 798, panel.x .. "+" .. panel.w)
check("panel y dentro de pantalla", panel.y >= 2 and panel.y + panel.h <= 598, panel.y .. "+" .. panel.h)

-- seleccionar la receta 1 (click en la fila) y redibujar
local clicked = false
for _, wid in ipairs(panel.widgets) do
    if wid.type == "button" and wid.flat and wid.label:find("Espada Legendaria", 1, true) then
        local ok = pcall(wid.onclick)
        clicked = ok
        break
    end
end
check("click en fila selecciona receta", clicked and st.selected == 1, tostring(st.selected))

local drawOk2 = true
for i = 1, 3 do
    local ok = pcall(on_draw, 400, 300)
    if not ok then drawOk2 = false end
end
check("redibujo tras seleccion sin errores", drawOk2)
local slotCount = 0
local hasMarker, flatColors = false, {}
for _, wid in ipairs(panel.widgets) do
    if wid.type == "slot" then slotCount = slotCount + 1 end
    if wid.type == "label" and wid.text == ">" then hasMarker = true end
    if wid.type == "button" and wid.flat then
        flatColors[table.concat(wid.color or {}, ",")] = true
    end
end
check("detalle con 5 slots (4 materiales + 1 resultado)", slotCount == 5, tostring(slotCount))
-- La fila seleccionada usa una caja DORADA con borde {235,180,80} (el fondo
-- del mensaje de resultado es otra caja oscura sin borde dorado).
local selBox = 0
for _, wid in ipairs(panel.widgets) do
    if wid.type == "box" and wid.border and wid.border[1] == 235 and wid.border[2] == 180 then
        selBox = selBox + 1
    end
end
check("fila seleccionada con caja dorada (box)", selBox == 1, tostring(selBox))
-- fix "texto que se mueve": el marcador ">" es un LABEL aparte en x fija y el
-- texto de TODAS las filas usa el MISMO color (el dorado al seleccionar hacia
-- que el renderizador del juego dibujara la letra mas gruesa = crecia/achicaba)
check("marcador '>' separado en x fija (label)", hasMarker)
local colorStr, colorCount = "", 0
for c in pairs(flatColors) do colorStr = colorStr .. c .. " " colorCount = colorCount + 1 end
check("TODAS las filas con el MISMO color de texto (no crece/achica)",
    colorCount == 1 and flatColors["230,224,200"] == true, colorStr)

-- centrado POR RESOLUCION REAL (no fijo a 800x600: el usuario pidio que sirva
-- en cualquier resolucion). Se simula 1024x768 reemplazando las funciones del
-- stub de Client y reabriendo el panel.
local origRX, origRY = Client.ResolutionX, Client.ResolutionY
Client.ResolutionX = function() return 1024 end
Client.ResolutionY = function() return 768 end
feed("OPEN")
check("panel CENTRADO a 1024x768 (-> 334,204)", panel.x == 334 and panel.y == 204,
    panel.x .. "," .. panel.y)
Client.ResolutionX, Client.ResolutionY = origRX, origRY
feed("OPEN") -- volver a abrir a 800x600 (los checks siguientes lo asumen)
-- el OPEN reseteo recipes/selected: re-alimentar la receta 1 (5 materiales) y
-- re-seleccionarla para los checks de solapamiento
feed("RECIPE|1|Espada Legendaria|5000000|80|14:3:1:Espada de Plata;7:0:5:Piedra de Forja;12:0:1:Bendicion;22:0:2:Mineral;23:0:1:Gema Rara|14:5:255:1:0:0:0:Espada Legendaria")
feed("STOCK|14:3:1;7:0:3;22:0:2")
for _, wid in ipairs(panel.widgets) do
    if wid.type == "button" and wid.flat and wid.label:find("Espada Legendaria", 1, true) then
        pcall(wid.onclick)
        break
    end
end
local drawOk3 = true
for i = 1, 2 do
    local ok = pcall(on_draw, 400, 300)
    if not ok then drawOk3 = false end
end
check("reabierto a 800x600 y re-seleccionada receta 1 sin errores", drawOk3)
check("receta 1 seleccionada tras reabrir", st.selected == 1, tostring(st.selected))

-- receta 1 tiene 5 materiales -> debe aparecer "+1 materiales mas" y los
-- sectores fijos (Costo/Resultado) NO deben solaparse con el aviso
local plusN, zenY, resY, msgSep = false, nil, nil, nil
local minPlus, maxPlus = nil, nil
for _, wid in ipairs(panel.widgets) do
    if wid.type == "label" then
        local t = wid.text or ""
        if t:find("materiales mas") then plusN = true end
        if t:find("Costo zen") then zenY = wid.y end
        if t:find("Resultado") then resY = wid.y end
    elseif wid.type == "separator" then
        if msgSep == nil or wid.y > msgSep then msgSep = wid.y end
    end
end
check("aviso '+1 materiales mas' con 5 materiales", plusN)
check("sectores sin solape: zen < resultado", zenY ~= nil and resY ~= nil and zenY + 12 <= resY, (zenY or "?") .. " vs " .. (resY or "?"))
check("resultado no pisa el area de mensaje", resY ~= nil and msgSep ~= nil and resY + 14 <= msgSep, (resY or "?") .. " vs sep " .. (msgSep or "?"))

-- OK/FAIL pintan el mensaje de resultado sin errores
feed("OK|1|Espada Legendaria")
check("OK marcado como exito", st.lastOk == true)
local drawOk4 = true
for i = 1, 2 do
    local ok = pcall(on_draw, 400, 300)
    if not ok then drawOk4 = false end
end
check("dibujo con mensaje de exito sin errores", drawOk4)

print("[UI-CHECK] RESULTADO: " .. okCnt .. "/" .. (okCnt + failCnt) .. " OK")
