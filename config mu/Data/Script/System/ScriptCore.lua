-- ================================================================
-- SilentDevs Lua Core - ScriptCore
-- Administra callbacks seguros desde C++ hacia Lua.
-- ================================================================

BridgeFunctionTable = {}

function BridgeFunctionAttach(bridgeName, functionName)
	if type(bridgeName) ~= 'string' or type(functionName) ~= 'string' then
		LogColor(1, '[Lua] BridgeFunctionAttach recibio parametros invalidos.')
		return
	end

	if BridgeFunctionTable[bridgeName] == nil then
		BridgeFunctionTable[bridgeName] = {}
	end

	for _, info in ipairs(BridgeFunctionTable[bridgeName]) do
		if info.Function == functionName then
			return
		end
	end

	table.insert(BridgeFunctionTable[bridgeName], { Function = functionName })
end

local function SilentCallList(bridgeName, ...)
	local list = BridgeFunctionTable[bridgeName]

	if list == nil then
		return 0
	end

	for _, info in ipairs(list) do
		local callback = _G[info.Function]

		if type(callback) == 'function' then
			local ok, result = pcall(callback, ...)

			if not ok then
				LogColor(1, string.format('[Lua][%s] %s', bridgeName, tostring(result)))
			elseif result ~= nil and result ~= 0 then
				return result
			end
		end
	end

	return 0
end

function BridgeFunction_OnReadScript()
	SilentCallList('OnReadScript')
end

function BridgeFunction_OnShutScript()
	SilentCallList('OnShutScript')
end

function BridgeFunction_OnTimerThread()
	SilentCallList('OnTimerThread')
end

function BridgeFunction_OnCharacterEntry(aIndex)
	SilentCallList('OnCharacterEntry', aIndex)
end

function BridgeFunction_OnCharacterClose(aIndex)
	SilentCallList('OnCharacterClose', aIndex)
end

function BridgeFunction_OnNpcTalk(npcIndex, userIndex)
	return SilentCallList('OnNpcTalk', npcIndex, userIndex)
end

function BridgeFunction_OnMonsterDie(monsterIndex, killerIndex)
	SilentCallList('OnMonsterDie', monsterIndex, killerIndex)
end
