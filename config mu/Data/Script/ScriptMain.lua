--#############################################################################
--# SilentDevs MuOnline Emulator
--# Lua Server-Side Core
--#
--# Archivo: ScriptMain.lua
--#
--# Este archivo es el punto de entrada principal del sistema Lua.
--# Su funcion principal es cargar otros scripts con require().
--#
--# GUIA RAPIDA PARA ADMINISTRADORES
--#
--# 1) No borres estas lineas obligatorias:
--#      LoadModule('System\\ScriptCore', true)
--#      LoadModule('System\\ScriptConfig', true)
--#      LoadModule('Lib\\Utils', true)
--#
--#    Sin ellas el sistema Lua no podra iniciar correctamente.
--#
--# 2) Para crear tus propios scripts:
--#      - Crea un archivo .lua dentro de C:\MuServer\Data\Script
--#      - Recomendado:
--#          Events\\MiEvento.lua     -> Eventos nuevos
--#          Custom\\MiSistema.lua    -> Sistemas custom simples
--#
--# 3) Para cargar un script nuevo:
--#      - Agrega su activador en System\\ScriptConfig.lua:
--#
--#          SilentLua.Modules = {
--#              MiEvento = true,
--#          }
--#
--#      - Luego agrega aqui su carga:
--#
--#          if SilentLua.Modules.MiEvento then
--#              LoadModule('Events\\MiEvento', false)
--#          end
--#
--# 4) required = true / false:
--#      true  = modulo obligatorio. Si falla, detiene la carga.
--#      false = modulo opcional. Si falla, se registra en el GS y continua.
--#
--# 5) Seguridad:
--#      Lua esta limitado por sandbox. No hay acceso libre a io, os, debug,
--#      package, dofile, loadfile ni load.
--#
--# 6) Recarga:
--#      Para recargar scripts usa el menu del GS:
--#          Reload > Reload Lua Script
--#
--# Recomendacion:
--#      Agrega tus scripts en la seccion "MODULOS CUSTOM DEL ADMINISTRADOR".
--#############################################################################

-- ================================================================
-- FUNCION INTERNA DE CARGA SEGURA
-- ================================================================
local function LoadModule(moduleName, required)
	local ok, result = pcall(require, moduleName)

	if ok then
		LogColor(2, string.format('[Lua] Module loaded: %s', moduleName))
		return true
	end

	LogColor(1, string.format('[Lua] Module load failed: %s | %s', moduleName, tostring(result)))

	if required then
		error(result)
	end

	return false
end

-- ================================================================
-- MODULOS OBLIGATORIOS DEL CORE SILENTDEVS
-- No modificar salvo que sepas exactamente lo que haces.
-- ================================================================
LoadModule('System\\ScriptCore', true)
LoadModule('System\\ScriptConfig', true)
LoadModule('Lib\\Utils', true)

if SilentLua.Enabled then

	-- ================================================================
	-- MODULOS OFICIALES SILENTDEVS
	-- Se activan/desactivan desde System\\ScriptConfig.lua.
	-- ================================================================
	if SilentLua.Modules.WelcomeMessage then
		LoadModule('Custom\\WelcomeMessage', false)
	end

	-- ================================================================
	-- MODULOS INTERNOS DE PRUEBA
	-- Mantener desactivados en produccion/distribucion.
	-- Solo sirven para validar APIs Lua del servidor.
	-- ================================================================
	if SilentLua.Modules.NativeApiSelfTest and SilentLua.NativeApiSelfTest.Enabled then
		LoadModule('System\\NativeApiSelfTest', false)
	end

	if SilentLua.Modules.MonsterApiSelfTest and SilentLua.MonsterApiSelfTest.Enabled then
		LoadModule('System\\MonsterApiSelfTest', false)
	end

	if SilentLua.Modules.RewardApiSelfTest and SilentLua.RewardApiSelfTest.Enabled then
		LoadModule('System\\RewardApiSelfTest', false)
	end

	-- ================================================================
	-- MODULOS CUSTOM DEL ADMINISTRADOR
	-- Agrega aqui tus scripts propios.
	--
	-- Ejemplo recomendado:
	--
	-- if SilentLua.Modules.MyEvent then
	--     LoadModule('Events\\MyEvent', false)
	-- end
	--
	-- Luego activalo en System\\ScriptConfig.lua:
	--
	-- SilentLua.Modules = {
	--     MyEvent = true,
	-- }
	-- ================================================================
else
	LogColor(1, '[Lua] SilentLua.Enabled = false. Modulos custom desactivados.')
end
