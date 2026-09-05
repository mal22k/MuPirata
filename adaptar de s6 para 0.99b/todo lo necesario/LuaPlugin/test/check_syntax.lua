-- check_syntax.lua - compila (loadfile, sin ejecutar) los scripts del espejo
-- para detectar errores de sintaxis ANTES de que el usuario los traslade.
-- Uso:  test_lua.exe check_syntax.lua   (desde la carpeta LuaPlugin\test\)
local files = {
  "..\\..\\scriptlua\\servidor\\Script\\BattlePass.lua",
  "..\\..\\scriptlua\\servidor\\Script\\Trivia.lua",
  "..\\..\\scriptlua\\servidor\\ScriptMain.lua",
  "..\\..\\scriptlua\\cliente\\main.lua",
  "..\\..\\scriptlua\\cliente\\battlepass\\main.lua",
  "..\\..\\scriptlua\\cliente\\trivia\\main.lua",
}
local okAll = true
for _, p in ipairs(files) do
  local f, err = loadfile(p)
  if f then
    print("SYNTAX-OK   " .. p)
  else
    okAll = false
    print("SYNTAX-FAIL " .. p .. " :: " .. tostring(err))
  end
end
print(okAll and "== TODOS LOS SCRIPTS COMPILAN OK ==" or "== HAY ERRORES DE SINTAXIS ==")
