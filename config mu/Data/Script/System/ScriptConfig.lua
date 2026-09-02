-- ================================================================
-- SilentDevs Lua Core - ScriptConfig
-- Configuracion central del sistema Lua server-side.
-- ================================================================

SilentLua = SilentLua or {}

-- Activador general de modulos custom.
SilentLua.Enabled = true

-- Debug solo para desarrollo. En produccion se recomienda false.
SilentLua.Debug = false

-- Idioma base para futuros mensajes: ENG, SPN, POR.
SilentLua.Language = 'SPN'

-- Modulos cargados desde ScriptMain.lua.
SilentLua.Modules = {
	WelcomeMessage = true,
	NativeApiSelfTest = false,
	MonsterApiSelfTest = false,
	RewardApiSelfTest = false,
}

-- Configuracion del modulo Custom\\WelcomeMessage.lua.
SilentLua.WelcomeMessage = {
	Enabled = true,

	-- Recomendado en false para produccion, asi no muestra mensajes de prueba al cliente.
	SendClientMessage = false,

	-- Mantiene trazabilidad privada en el GS.
	LogCharacterEntry = true,

	Text = '[Lua] Core activo. Bienvenido {name}!',
}

-- Pruebas internas no destructivas para validar funciones nativas.
-- Mantener false en produccion/distribucion.
SilentLua.NativeApiSelfTest = {
	Enabled = false,
	SendClientMessage = false,
}

-- Pruebas internas para validar creacion/lectura/eliminacion de monstruos temporales.
-- Mantener false en produccion/distribucion.
SilentLua.MonsterApiSelfTest = {
	Enabled = false,

	-- MonsterClass 2 = Budge Dragon. Es una prueba liviana y temporal.
	MonsterClass = 2,
	Map = 0,
	X = 135,
	Y = 135,
	Dir = 0,
}

-- Pruebas internas para validar recompensas desde Lua.
-- Mantener false en produccion/distribucion.
-- Incluso con el modulo activo, no entrega nada si TestItemGive/TestCoinAdd estan en false.
SilentLua.RewardApiSelfTest = {
	Enabled = false,
	TestItemGive = false,
	TestCoinAdd = false,

	-- Item de prueba seguro: Jewel of Bless = Grupo 14, Index 13.
	ItemGroup = 14,
	ItemIndex = 13,
	ItemLevel = 0,
	ItemSkill = 0,
	ItemLuck = 0,
	ItemOption = 0,
	ItemExcellent = 0,

	-- Monedas de prueba. Usar valores pequenos solamente.
	WCoinC = 0,
	WCoinP = 0,
	GoblinPoint = 0,
}
