-- ================================================================
-- SilentDevs Lua Core - WelcomeMessage
-- Modulo de ejemplo para validar OnReadScript y OnCharacterEntry.
-- Puede desactivarse desde System\\ScriptConfig.lua.
-- ================================================================

BridgeFunctionAttach('OnReadScript', 'WelcomeMessage_OnReadScript')
BridgeFunctionAttach('OnCharacterEntry', 'WelcomeMessage_OnCharacterEntry')

function WelcomeMessage_OnReadScript()
	SilentUtils.LogInfo('WelcomeMessage.lua cargado correctamente.')
end

function WelcomeMessage_OnCharacterEntry(aIndex)
	if not SilentLua.WelcomeMessage.Enabled then
		return
	end

	local name = GetObjectName(aIndex)

	if name == nil or name == '' then
		return
	end

	if SilentLua.WelcomeMessage.SendClientMessage then
		MessageSend(aIndex, SilentLua.WelcomeMessage.Text:gsub('{name}', name))
	end

	if SilentLua.WelcomeMessage.LogCharacterEntry then
		SilentUtils.LogInfo(string.format('CharacterEntry: %s (%d)', name, aIndex))
	end
end
