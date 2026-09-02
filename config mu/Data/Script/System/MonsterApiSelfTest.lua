-- ================================================================
-- SilentDevs Lua Core - MonsterApiSelfTest
-- Prueba controlada de funciones nativas de monstruos.
-- Se carga solo si ScriptConfig.lua lo habilita.
--
-- IMPORTANTE:
-- - Crea un monstruo temporal.
-- - Lee sus datos.
-- - Lo elimina inmediatamente.
-- - Mantener desactivado en produccion/distribucion.
-- ================================================================

BridgeFunctionAttach('OnReadScript', 'MonsterApiSelfTest_OnReadScript')

function MonsterApiSelfTest_OnReadScript()
	local cfg = SilentLua.MonsterApiSelfTest

	SilentUtils.LogInfo(string.format(
		'MonsterApiSelfTest habilitado. Probando Class:%d Map:%d X:%d Y:%d.',
		cfg.MonsterClass,
		cfg.Map,
		cfg.X,
		cfg.Y
	))

	local monsterIndex = SilentUtils.CreateMonster(cfg.MonsterClass, cfg.Map, cfg.X, cfg.Y, cfg.Dir)

	if monsterIndex == nil or monsterIndex < 0 then
		SilentUtils.LogError('MonsterApiSelfTest fallo: MonsterCreate devolvio indice invalido.')
		return
	end

	SilentUtils.LogInfo('MonsterApiSelfTest creado: ' .. SilentUtils.GetMonsterLabel(monsterIndex))

	if SilentUtils.DeleteMonster(monsterIndex) then
		SilentUtils.LogInfo('MonsterApiSelfTest eliminado correctamente.')
	else
		SilentUtils.LogError('MonsterApiSelfTest fallo: no se pudo eliminar el monstruo temporal.')
	end
end
