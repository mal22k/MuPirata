-- ================================================================
-- SilentDevs Lua Core - NativeApiSelfTest
-- Prueba no destructiva de funciones nativas C++ <-> Lua.
-- Se carga solo si ScriptConfig.lua lo habilita.
-- ================================================================

BridgeFunctionAttach('OnReadScript', 'NativeApiSelfTest_OnReadScript')
BridgeFunctionAttach('OnCharacterEntry', 'NativeApiSelfTest_OnCharacterEntry')

function NativeApiSelfTest_OnReadScript()
	SilentUtils.LogInfo('NativeApiSelfTest habilitado. Las pruebas se ejecutaran al entrar un personaje.')
end

function NativeApiSelfTest_OnCharacterEntry(aIndex)
	if not SilentUtils.IsValidUser(aIndex) then
		return
	end

	local label = SilentUtils.GetUserLabel(aIndex)
	local pos = SilentUtils.FormatUserPosition(aIndex)
	local money = GetObjectMoney(aIndex)
	local sameMap = CheckUserMap(aIndex, GetObjectMap(aIndex))
	local sameRange = CheckUserRange(aIndex, GetObjectMap(aIndex), GetObjectMapX(aIndex), GetObjectMapY(aIndex), 3)
	local hasOneZen = CheckObjectMoney(aIndex, 1)

	SilentUtils.LogInfo(string.format('SelfTest User:%s Pos:%s Money:%d MapOK:%s RangeOK:%s MoneyOK:%s',
		label,
		pos,
		money,
		tostring(sameMap),
		tostring(sameRange),
		tostring(hasOneZen)
	))

	if SilentLua.NativeApiSelfTest.SendClientMessage then
		MessageSend(aIndex, '[Lua] Native API SelfTest OK. Ver log del GS.')
	end
end
