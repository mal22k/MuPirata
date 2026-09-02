-- ================================================================
-- SilentDevs Lua Core - RewardApiSelfTest
-- Prueba controlada de recompensas desde Lua.
--
-- IMPORTANTE:
-- - Se carga solo si ScriptConfig.lua lo habilita.
-- - No entrega items ni monedas si TestItemGive/TestCoinAdd estan en false.
-- - Mantener desactivado en produccion/distribucion.
-- ================================================================

BridgeFunctionAttach('OnCharacterEntry', 'RewardApiSelfTest_OnCharacterEntry')

function RewardApiSelfTest_OnCharacterEntry(aIndex)
	if not SilentUtils.IsValidUser(aIndex) then
		return
	end

	local cfg = SilentLua.RewardApiSelfTest
	local label = SilentUtils.GetUserLabel(aIndex)

	SilentUtils.LogInfo('RewardApiSelfTest habilitado para ' .. label .. '.')

	if cfg.TestItemGive then
		local ok = SilentUtils.GiveItem(
			aIndex,
			cfg.ItemGroup,
			cfg.ItemIndex,
			cfg.ItemLevel,
			cfg.ItemSkill,
			cfg.ItemLuck,
			cfg.ItemOption,
			cfg.ItemExcellent
		)

		SilentUtils.LogInfo(string.format(
			'RewardApiSelfTest ItemGive result:%s Item:[%d,%d]',
			tostring(ok),
			cfg.ItemGroup,
			cfg.ItemIndex
		))
	end

	if cfg.TestCoinAdd then
		local ok = SilentUtils.AddCoins(aIndex, cfg.WCoinC, cfg.WCoinP, cfg.GoblinPoint)

		SilentUtils.LogInfo(string.format(
			'RewardApiSelfTest CoinAdd result:%s WCoinC:%d WCoinP:%d GoblinPoint:%d',
			tostring(ok),
			cfg.WCoinC,
			cfg.WCoinP,
			cfg.GoblinPoint
		))
	end
end
