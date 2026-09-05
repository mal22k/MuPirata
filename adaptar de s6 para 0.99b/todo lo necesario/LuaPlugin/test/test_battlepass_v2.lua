-- test_battlepass_v2.lua - test funcional del BattlePass V2 (persistencia SQL)
-- Mockea la API SQL del GS (SQLConnect/SQLQuery/SQLFetch/SQLGetNumber/
-- SQLGetString/SQLClose) con una base en memoria, y corre los flujos:
--   T1 perfil nuevo vacio | T2 migracion txt->SQL | T3 writeProfile a SQL
--   T4 cache + flushProfile | T5 fallback txt
-- Uso:  test_lua.exe test_battlepass_v2.lua  (desde LuaPlugin\test\)
-- ============================================================================

-- ---- bridges del GS que no existen en el harness ----
function BridgeFunctionAttach() end
function LogPrint(m) print("[log] " .. tostring(m)) end

-- ---- mock SQL (base en memoria) ----
local MEM = {}       -- [name] = { lvl, exp, claimed (CSV) }
local pending = nil

function SQLCheck() return 1 end
function SQLConnect() return true end
function SQLQuery(q)
    pending = nil
    if q:sub(1, 6) == "SELECT" then
        local name = q:match("Character%s*%]%s*=%s*'([^']+)'")
        if name and MEM[name] then
            local r = MEM[name]
            pending = { lvl = r.lvl, exp = r.exp, claimed = r.claimed or "" }
        end
        return true
    end
    local name = q:match("Character%s*%]%s*=%s*'([^']+)'")
    if not name then return false end
    if q:find("UPDATE", 1, true) then
        local lvl = tonumber(q:match("Level%s*%]%s*=%s*(%d+)"))
        local exp = tonumber(q:match("Exp%s*%]%s*=%s*(%d+)"))
        local cl  = q:match("Claimed%s*%]%s*=%s*'([^']*)'")
        MEM[name] = { lvl = lvl, exp = exp, claimed = cl or "" }
        return true
    end
    if q:find("INSERT", 1, true) then
        local lvl, exp, cl = q:match("VALUES%s*%('.-',%s*(%d+),%s*(%d+),%s*'([^']*)'%)")
        MEM[name] = { lvl = tonumber(lvl), exp = tonumber(exp), claimed = cl or "" }
        return true
    end
    return false
end
function SQLFetch()
    return pending ~= nil
end
function SQLGetNumber(col)
    if not pending then return 0 end
    if col == "Level" then return pending.lvl end
    if col == "Exp" then return pending.exp end
    return 0
end
function SQLGetString(col)
    if not pending then return "" end
    if col == "Claimed" then return pending.claimed end
    return ""
end
function SQLClose()
    pending = nil
end

-- ---- cargar el modulo real ----
local BP = dofile("../../scriptlua/servidor/Script/BattlePass.lua")

local fails = 0
local function check(cond, name)
    if cond then
        print("OK   " .. name)
    else
        fails = fails + 1
        print("FAIL " .. name)
    end
end

local dir = BP.dir

-- T1: perfil nuevo (sin fila SQL ni txt) -> vacio
local p1 = BP.readProfile("Nuevo")
check(p1.lvl == 0 and p1.exp == 0 and next(p1.claimed) == nil, "T1 perfil nuevo vacio")

-- T2: migracion desde txt existente (se escribe a SQL y se borra el txt)
local f = io.open(dir .. "Viejo.txt", "w")
f:write("lvl=3\nexp=100\nclaimed=1,2\n")
f:close()
local p2 = BP.readProfile("Viejo")
check(p2.lvl == 3 and p2.exp == 100 and p2.claimed[1] and p2.claimed[2], "T2 lee txt migrado")
check(MEM.Viejo ~= nil and MEM.Viejo.lvl == 3 and MEM.Viejo.exp == 100, "T2 migracion escrita en SQL")
local ft = io.open(dir .. "Viejo.txt", "r")
check(ft == nil, "T2 txt borrado tras migrar")
if ft then ft:close() end

-- T3: writeProfile escribe en SQL (no en txt)
BP.writeProfile("Juan", { lvl = 2, exp = 50, claimed = { [1] = true } })
check(MEM.Juan ~= nil and MEM.Juan.lvl == 2 and MEM.Juan.exp == 50 and MEM.Juan.claimed == "1", "T3 writeProfile -> SQL")

-- T4: cache + flushProfile
local e = BP.getProfile("Juan")
check(e.lvl == 2, "T4 getProfile carga desde SQL")
e.exp = e.exp + 10
e.dirty = true
BP.flushProfile("Juan")
check(MEM.Juan.exp == 60, "T4 flushProfile actualiza SQL")

-- T5: fallback txt forzado
BP._storage = "txt"
BP._sqlBroken = false
BP.writeProfile("Pepito", { lvl = 1, exp = 5, claimed = {} })
local pp = BP.readProfile("Pepito")
check(pp.lvl == 1 and pp.exp == 5, "T5 fallback txt escribe y lee")
os.remove(dir .. "Pepito.txt")

-- T6: flushAll con cache (OnShutScript)
BP._storage = "sql"
local e2 = BP.getProfile("Ana")
e2.lvl = 4
e2.exp = 900
e2.claimed[3] = true
e2.dirty = true
BP.flushAll()
check(MEM.Ana ~= nil and MEM.Ana.lvl == 4 and MEM.Ana.claimed == "3", "T6 flushAll escribe toda la cache")

print(fails == 0 and "== BATTLEPASS V2: TODOS LOS TESTS OK ==" or ("== BATTLEPASS V2: " .. fails .. " FALLARON =="))
