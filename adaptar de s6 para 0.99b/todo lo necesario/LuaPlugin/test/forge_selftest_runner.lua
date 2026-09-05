-- ============================================================================
-- forge_selftest_runner.lua - ejecuta el selftest de Forge.lua (server)
-- ============================================================================
-- Uso:  cd LuaPlugin\test  &&  test_lua.exe forge_selftest_runner.lua
-- Carga el script del server (Data/Script/Script/Forge.lua) con los stubs que
-- el propio selftest define, ejecuta Forge.selftest() y reporta el resultado.
-- 0 efectos reales: el selftest aísla la API con stubs locales.
-- ============================================================================

-- Stub del registro de bridges (en el server real lo define ScriptCore)
if type(BridgeFunctionAttach) ~= "function" then BridgeFunctionAttach = function() end end

local ok, err = pcall(dofile, "..\\..\\MuServerS6Evercion maxima\\Data\\Script\\Script\\Forge.lua")
if not ok then
    print("[Forge-TEST] ERROR cargando Forge.lua: " .. tostring(err))
    os.exit(1)
end

-- El selftest loguea con LogPrint: lo redirigimos a print para ver los checks
local realLogPrint = LogPrint
local logLines = {}
LogPrint = function(msg)
    logLines[#logLines + 1] = tostring(msg)
    if realLogPrint then pcall(realLogPrint, msg) end
end

local r = Forge.selftest(0)
for _, ln in ipairs(logLines) do print(ln) end
print(r and "[Forge-TEST] RUNNER: SELFTEST OK" or "[Forge-TEST] RUNNER: SELFTEST CON FALLOS")
os.exit(r and 0 or 1)
