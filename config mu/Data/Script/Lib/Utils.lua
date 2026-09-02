-- ================================================================
-- SilentDevs Lua Core - Utils
-- Funciones auxiliares reutilizables para scripts del lado servidor.
-- ================================================================

SilentUtils = SilentUtils or {}

function SilentUtils.LogInfo(message)
	LogColor(2, string.format('[Lua] %s', tostring(message)))
end

function SilentUtils.LogError(message)
	LogColor(1, string.format('[Lua] %s', tostring(message)))
end

function SilentUtils.LogDebug(message)
	if SilentLua ~= nil and SilentLua.Debug then
		LogColor(3, string.format('[Lua][Debug] %s', tostring(message)))
	end
end

function SilentUtils.IsValidUser(aIndex)
	return GetObjectConnected(aIndex) == 1 and GetObjectName(aIndex) ~= ''
end

function SilentUtils.GetUserLabel(aIndex)
	if not SilentUtils.IsValidUser(aIndex) then
		return string.format('InvalidUser(%s)', tostring(aIndex))
	end

	return string.format('%s[%s]', GetObjectName(aIndex), GetObjectAccount(aIndex))
end

function SilentUtils.GetUserPosition(aIndex)
	return {
		Map = GetObjectMap(aIndex),
		X = GetObjectMapX(aIndex),
		Y = GetObjectMapY(aIndex),
	}
end

function SilentUtils.FormatUserPosition(aIndex)
	local pos = SilentUtils.GetUserPosition(aIndex)
	return string.format('Map:%d X:%d Y:%d', pos.Map, pos.X, pos.Y)
end

function SilentUtils.CheckUserMap(aIndex, map)
	return CheckUserMap(aIndex, map) == true
end

function SilentUtils.CheckUserRange(aIndex, map, x, y, range)
	return CheckUserRange(aIndex, map, x, y, range) == true
end

function SilentUtils.CheckMoney(aIndex, amount)
	return CheckObjectMoney(aIndex, amount) == true
end

function SilentUtils.AddMoney(aIndex, amount)
	return AddObjectMoney(aIndex, amount)
end

function SilentUtils.SubMoney(aIndex, amount)
	return SubObjectMoney(aIndex, amount)
end

function SilentUtils.CreateMonster(monsterClass, map, x, y, dir)
	return MonsterCreate(monsterClass, map, x, y, dir or 0)
end

function SilentUtils.DeleteMonster(monsterIndex)
	return MonsterDelete(monsterIndex) == true
end

function SilentUtils.GetMonsterLabel(monsterIndex)
	return string.format('Monster(Index:%d Class:%d Map:%d Life:%s)',
		monsterIndex,
		GetMonsterClass(monsterIndex),
		GetMonsterMap(monsterIndex),
		tostring(GetMonsterLife(monsterIndex))
	)
end

function SilentUtils.GiveItem(aIndex, group, id, level, skill, luck, option, excellent)
	return ItemGive(aIndex, group, id, level or 0, skill or 0, luck or 0, option or 0, excellent or 0) == true
end

function SilentUtils.AddCoins(aIndex, wCoinC, wCoinP, goblinPoint)
	return CoinAdd(aIndex, wCoinC or 0, wCoinP or 0, goblinPoint or 0) == true
end
