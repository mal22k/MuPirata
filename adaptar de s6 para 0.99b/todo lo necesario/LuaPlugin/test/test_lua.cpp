// ============================================================================
// test_lua.cpp - Test harness STANDALONE del LuaPlugin v0.3.0
//
// Simula la API que expone el plugin al script (Draw/Input/Client/Interface/
// UI/SendPacket/Log/RegisterPacketHandler) con stubs que imprimen en consola,
// carga main.lua y ejecuta on_draw, on_packet, el router y el polling de
// input (on_key/on_click). NO necesita el cliente MU: solo lua52.lib.
//
// Cubre: parser de dinero (MoneyCache.h), parser de estados del cliente
// (ClientStateCache.h: trade/shop/caos), config persistente, framework de UI
// (botones, arrastre, checkboxes, z-order), calibracion de ventanas,
// cache de packets en Lua, funciones Client.* y bloqueo de clicks.
//
// Compilar:  build_test.bat   (requiere VS2010 / v100, como el plugin)
// Ejecutar:  test_lua.exe [ruta_a_main.lua]
//            (default: ..\lua_scripts\main.lua relativo a la carpeta test\)
// ============================================================================

#include <stdio.h>
#include <string.h>
#include <windows.h> // BYTE y otros tipos de Windows

// Parser REAL del dinero (el mismo codigo que usa el plugin en el cliente)
#include "..\\src\\MoneyCache.h"

// Parser REAL de los estados del cliente (trade / personal shop / caos)
#include "..\\src\\ClientStateCache.h"

// Logica REAL del bloqueo de clicks (rects + consumido)
#include "..\\src\\MouseBlock.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// ---------------------------------------------------------------------------
// Registro del router (misma logica que el plugin: luaL_ref + tabla C++)
// ---------------------------------------------------------------------------

struct PACKET_HANDLER
{
	BYTE head;
	int sub;
	int luaRef;
};

static PACKET_HANDLER g_PacketHandlers[32];
static int g_PacketHandlerCount = 0;

static PACKET_HANDLER* FindPacketHandler(int head,int sub)
{
	PACKET_HANDLER* wildcard = 0;

	for(int n=0;n<g_PacketHandlerCount;n++)
	{
		if(g_PacketHandlers[n].luaRef == LUA_NOREF)
		{
			continue;
		}

		if(g_PacketHandlers[n].head == (BYTE)head)
		{
			if(g_PacketHandlers[n].sub == sub)
			{
				return &g_PacketHandlers[n];
			}

			if(g_PacketHandlers[n].sub == -1 && wildcard == 0)
			{
				wildcard = &g_PacketHandlers[n];
			}
		}
	}

	return wildcard;
}

static int StubRegisterPacketHandler(lua_State* L)
{
	int head = (int)luaL_checkinteger(L,1);

	int sub = -1;

	if(lua_isnil(L,2) == 0)
	{
		sub = (int)luaL_checkinteger(L,2);
	}

	luaL_checktype(L,3,LUA_TFUNCTION);

	PACKET_HANDLER* existing = FindPacketHandler(head,sub);

	if(existing != 0 && existing->sub == sub)
	{
		lua_pop(L,1);
		lua_pushinteger(L,(int)(existing - g_PacketHandlers) + 1);
		return 1;
	}

	int slot = -1;

	for(int n=0;n<g_PacketHandlerCount;n++)
	{
		if(g_PacketHandlers[n].luaRef == LUA_NOREF)
		{
			slot = n;
			break;
		}
	}

	if(slot == -1)
	{
		if(g_PacketHandlerCount >= (int)(sizeof(g_PacketHandlers)/sizeof(g_PacketHandlers[0])))
		{
			lua_pop(L,1);
			lua_pushinteger(L,0);
			return 1;
		}

		slot = g_PacketHandlerCount;
		g_PacketHandlerCount++;
	}

	int ref = luaL_ref(L,LUA_REGISTRYINDEX);

	if(ref == LUA_REFNIL || ref == LUA_NOREF)
	{
		lua_pushinteger(L,0);
		return 1;
	}

	g_PacketHandlers[slot].head = (BYTE)head;
	g_PacketHandlers[slot].sub = sub;
	g_PacketHandlers[slot].luaRef = ref;

	lua_pushinteger(L,slot + 1);

	return 1;
}

static int StubUnregisterPacketHandler(lua_State* L)
{
	int id = (int)luaL_checkinteger(L,1);

	if(id >= 1 && id <= g_PacketHandlerCount)
	{
		if(g_PacketHandlers[id-1].luaRef != LUA_NOREF)
		{
			luaL_unref(L,LUA_REGISTRYINDEX,g_PacketHandlers[id-1].luaRef);
			g_PacketHandlers[id-1].luaRef = LUA_NOREF;
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------
// Input simulado (polling identico al plugin, pero con estado de teclas fake)
// ---------------------------------------------------------------------------

static BYTE g_HarnessKeyState[256];
static BYTE g_PrevKeyState[256];
static BYTE g_SubscribedKeys[32];
static int g_SubscribedKeyCount = 0;

static int g_HarnessCursorX = 320;
static int g_HarnessCursorY = 240;

// Estado simulado de las ventanas nativas (para la calibracion y GetOpenWindows)
static BYTE g_HarnessWindows[0x20];

// Estado del cliente cacheado por packets (lo alimenta el parser real)
static CLIENT_STATE g_HarnessState;

// Estado del bloqueo de clicks simulado
static int g_HarnessBlockedRects[64][4];
static int g_HarnessBlockedRectCount = 0;
static int g_HarnessBlockActive = 0;
static int g_HarnessConsumed = 0; // 1 = el hook de la DLL consumio el ultimo click

static int g_HarnessSendCount = 0;

static int StubRegisterKey(lua_State* L)
{
	int vk = (int)luaL_checkinteger(L,1);

	if(vk < 0 || vk > 255)
	{
		lua_pushboolean(L,0);
		return 1;
	}

	for(int n=0;n<g_SubscribedKeyCount;n++)
	{
		if(g_SubscribedKeys[n] == (BYTE)vk)
		{
			lua_pushboolean(L,1);
			return 1;
		}
	}

	if(g_SubscribedKeyCount < (int)sizeof(g_SubscribedKeys))
	{
		g_SubscribedKeys[g_SubscribedKeyCount++] = (BYTE)vk;
		lua_pushboolean(L,1);
	}
	else
	{
		lua_pushboolean(L,0);
	}

	return 1;
}

static int StubKeyPressed(lua_State* L)
{
	int vk = (int)luaL_checkinteger(L,1);

	lua_pushboolean(L,(vk >= 0 && vk <= 255 && g_HarnessKeyState[vk] != 0) ? 1 : 0);

	return 1;
}

static void PollInput(lua_State* L)
{
	for(int n=0;n<g_SubscribedKeyCount;n++)
	{
		BYTE vk = g_SubscribedKeys[n];

		bool down = (g_HarnessKeyState[vk] != 0);

		if(down == (g_PrevKeyState[vk] != 0))
		{
			continue;
		}

		g_PrevKeyState[vk] = down ? 1 : 0;

		lua_getglobal(L,"on_key");

		if(lua_isfunction(L,-1) == 0)
		{
			lua_pop(L,1);
			continue;
		}

		lua_pushinteger(L,vk);
		lua_pushboolean(L,down ? 1 : 0);

		if(lua_pcall(L,2,0,0) != 0)
		{
			fprintf(stderr,"ERROR en on_key: %s\n",lua_tostring(L,-1));
			lua_pop(L,1);
			return;
		}
	}

	static const BYTE mouseButtons[] = { 0x01, 0x02, 0x04 };

	for(int n=0;n<3;n++)
	{
		BYTE vk = mouseButtons[n];

		bool down = (g_HarnessKeyState[vk] != 0);

		if(down == (g_PrevKeyState[vk] != 0))
		{
			continue;
		}

		g_PrevKeyState[vk] = down ? 1 : 0;

		lua_getglobal(L,"on_click");

		if(lua_isfunction(L,-1) == 0)
		{
			lua_pop(L,1);
			continue;
		}

		lua_pushinteger(L,g_HarnessCursorX);
		lua_pushinteger(L,g_HarnessCursorY);
		lua_pushinteger(L,vk);
		lua_pushboolean(L,down ? 1 : 0);

		if(lua_pcall(L,4,0,0) != 0)
		{
			fprintf(stderr,"ERROR en on_click: %s\n",lua_tostring(L,-1));
			lua_pop(L,1);
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Stubs de la API del plugin (mismos nombres/tablas que registra el plugin)
// ---------------------------------------------------------------------------

static int StubDrawText(lua_State* L)
{
	// Estricto como la DLL real (LuaDrawText usa luaL_checknumber/string):
	// un color anidado { {r,g,b} } falla igual que en el juego (regresion v0.3.3).
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	const char* t = luaL_checkstring(L,3);
	double r = luaL_checknumber(L,4);
	double g = luaL_checknumber(L,5);
	double b = luaL_checknumber(L,6);
	printf("[Draw.Text] (%d,%d) \"%s\" rgb(%g,%g,%g)\n",x,y,t,r,g,b);
	return 0;
}

static int StubDrawBar(lua_State* L)
{
	// Estricto como la DLL real: 8 numeros (x,y,w,h,r,g,b,a)
	printf("[Draw.Bar] (%g,%g) %g x %g rgb(%g,%g,%g) a=%g\n",
		luaL_checknumber(L,1),luaL_checknumber(L,2),luaL_checknumber(L,3),luaL_checknumber(L,4),
		luaL_checknumber(L,5),luaL_checknumber(L,6),luaL_checknumber(L,7),luaL_checknumber(L,8));
	return 0;
}

static int StubDrawMessage(lua_State* L)
{
	const char* t = luaL_checkstring(L,1);
	int type = (int)luaL_optinteger(L,2,1);
	printf("[Draw.Message] type=%d \"%s\"\n",type,t);
	return 0;
}

static int StubDrawImage(lua_State* L)
{
	printf("[Draw.Image] id=%d\n",(int)luaL_checkinteger(L,1));
	return 0;
}

static int StubLoadImage(lua_State* L)
{
	const char* path = luaL_checkstring(L,1);
	printf("[Draw.LoadImage] \"%s\" -> id 100 (stub)\n",path);
	lua_pushinteger(L,100);
	return 1;
}

static int StubDrawTooltip(lua_State* L)
{
	const char* t = luaL_checkstring(L,3);
	printf("[Draw.Tooltip] (%d,%d) \"%s\"\n",(int)luaL_checkinteger(L,1),(int)luaL_checkinteger(L,2),t);
	return 0;
}

static int StubCursorX(lua_State* L) { lua_pushinteger(L,g_HarnessCursorX); return 1; }
static int StubCursorY(lua_State* L) { lua_pushinteger(L,g_HarnessCursorY); return 1; }

static int StubScreenState(lua_State* L) { lua_pushinteger(L,5); return 1; }
static int StubInGame(lua_State* L) { lua_pushboolean(L,1); return 1; }
static int StubResolutionX(lua_State* L) { lua_pushinteger(L,800); return 1; }
static int StubResolutionY(lua_State* L) { lua_pushinteger(L,600); return 1; }
static int StubCharacterName(lua_State* L) { lua_pushstring(L,"TestChar"); return 1; }

static int StubCharacterLevel(lua_State* L)   { lua_pushinteger(L,120); return 1; }
static int StubCharacterClass(lua_State* L)   { lua_pushinteger(L,1); return 1; }
static int StubCharacterHP(lua_State* L)      { lua_pushinteger(L,50000); return 1; }
static int StubCharacterMaxHP(lua_State* L)   { lua_pushinteger(L,65000); return 1; }
static int StubCharacterMP(lua_State* L)      { lua_pushinteger(L,4000); return 1; }
static int StubCharacterMaxMP(lua_State* L)   { lua_pushinteger(L,5000); return 1; }
static int StubCharacterShield(lua_State* L)  { lua_pushinteger(L,3000); return 1; }
static int StubCharacterMaxShield(lua_State* L) { lua_pushinteger(L,3000); return 1; }
static int StubCharacterBP(lua_State* L)      { lua_pushinteger(L,2000); return 1; }
static int StubCharacterMaxBP(lua_State* L)   { lua_pushinteger(L,2000); return 1; }
static int StubCharacterStrength(lua_State* L)    { lua_pushinteger(L,3000); return 1; }
static int StubCharacterDexterity(lua_State* L)   { lua_pushinteger(L,1500); return 1; }
static int StubCharacterVitality(lua_State* L)    { lua_pushinteger(L,1000); return 1; }
static int StubCharacterEnergy(lua_State* L)      { lua_pushinteger(L,800); return 1; }
static int StubCharacterLeadership(lua_State* L)  { lua_pushinteger(L,400); return 1; }
static int StubCharacterLevelUpPoint(lua_State* L){ lua_pushinteger(L,25); return 1; }
static int StubCharacterExperience(lua_State* L)  { lua_pushinteger(L,12345678); return 1; }
static int StubCharacterNextExperience(lua_State* L) { lua_pushinteger(L,25000000); return 1; }
static int StubCharacterMoney(lua_State* L)   { lua_pushinteger(L,9999999); return 1; }
static int StubCharacterMap(lua_State* L)     { lua_pushinteger(L,0); return 1; }

// --- Stubs de los estados cacheados (trade/shop/caos) que leen el parser REAL ---
static int StubClientIsTradeOpen(lua_State* L)      { lua_pushboolean(L,g_HarnessState.tradeOpen ? 1 : 0); return 1; }
static int StubClientIsTradeAccepted(lua_State* L)  { lua_pushboolean(L,g_HarnessState.tradeAccepted ? 1 : 0); return 1; }
static int StubClientGetTradeMoney(lua_State* L)    { lua_pushinteger(L,g_HarnessState.tradeMoney); return 1; }

static int StubClientGetTradeItem(lua_State* L)
{
	int slot = (int)luaL_checkinteger(L,1);

	if(slot < 0 || slot >= TRADE_MAX_ITEMS || g_HarnessState.tradeItems[slot].index <= 0)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	lua_pushinteger(L,g_HarnessState.tradeItems[slot].slot);  lua_setfield(L,-2,"slot");
	lua_pushinteger(L,g_HarnessState.tradeItems[slot].index); lua_setfield(L,-2,"index");
	lua_pushinteger(L,g_HarnessState.tradeItems[slot].level); lua_setfield(L,-2,"level");
	lua_pushinteger(L,g_HarnessState.tradeItems[slot].dur);   lua_setfield(L,-2,"dur");

	return 1;
}

static int StubClientIsShopOpen(lua_State* L)     { lua_pushboolean(L,g_HarnessState.shopOpen ? 1 : 0); return 1; }
static int StubClientIsChaosBoxOpen(lua_State* L) { lua_pushboolean(L,g_HarnessState.chaosBoxSeen ? 1 : 0); return 1; }

// --- Stubs NO CONFIRMADOS (deben devolver lo mismo que la DLL) ---
static int StubClientIsNpcDialogOpen(lua_State* L) { lua_pushboolean(L,0); return 1; }
static int StubClientGetNpcIndex(lua_State* L)     { lua_pushinteger(L,-1); return 1; }
static int StubClientGetNpcName(lua_State* L)      { lua_pushstring(L,""); return 1; }
static int StubClientGetInventoryItem(lua_State* L){ lua_pushnil(L); return 1; }
static int StubClientIsInventorySlotEmpty(lua_State* L) { lua_pushnil(L); return 1; }
static int StubClientGetWearItem(lua_State* L)     { lua_pushnil(L); return 1; }
static int StubClientGetShopItem(lua_State* L)     { lua_pushnil(L); return 1; }
static int StubClientGetShopPrice(lua_State* L)    { lua_pushnil(L); return 1; }
static int StubClientGetChaosItem(lua_State* L)    { lua_pushnil(L); return 1; }

static int StubInterfaceOpen(lua_State* L)
{
	int wid = (int)luaL_checkinteger(L,1);
	if(wid >= 0 && wid < 0x20) { g_HarnessWindows[wid] = 1; }
	printf("[Interface.Open] wid=0x%02X -> 1 (stub)\n",wid);
	lua_pushinteger(L,1);
	return 1;
}

static int StubInterfaceClose(lua_State* L)
{
	int wid = (int)luaL_checkinteger(L,1);
	if(wid >= 0 && wid < 0x20) { g_HarnessWindows[wid] = 0; }
	printf("[Interface.Close] wid=0x%02X -> 1 (stub)\n",wid);
	lua_pushinteger(L,1);
	return 1;
}

static int StubInterfaceIsOpen(lua_State* L)
{
	int wid = (int)luaL_checkinteger(L,1);
	int open = (wid >= 0 && wid < 0x20) ? (g_HarnessWindows[wid] != 0) : 0;
	printf("[Interface.IsOpen] wid=0x%02X -> %s (stub)\n",wid,open ? "true" : "false");
	lua_pushboolean(L,open);
	return 1;
}

static int StubInterfaceGetOpenWindows(lua_State* L)
{
	lua_newtable(L);

	int count = 0;

	for(int id=1;id<=0x1F;id++)
	{
		if(g_HarnessWindows[id])
		{
			count++;
			lua_pushinteger(L,id);
			lua_rawseti(L,-2,count);
		}
	}

	printf("[Interface.GetOpenWindows] %d ventanas abiertas (stub)\n",count);

	return 1;
}

static int StubInterfaceGetActiveWindow(lua_State* L)
{
	printf("[Interface.GetActiveWindow] -> -1 (NO CONFIRMADO stub)\n");
	lua_pushinteger(L,-1);
	return 1;
}

// --- Stubs UI (bloqueo de clicks): misma logica que la DLL ---
static int StubBlockMouse(lua_State* L)
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	int w = (int)luaL_checkinteger(L,3);
	int h = (int)luaL_checkinteger(L,4);

	if(g_HarnessBlockedRectCount >= 64)
	{
		lua_pushboolean(L,0);
		return 1;
	}

	g_HarnessBlockedRects[g_HarnessBlockedRectCount][0] = x;
	g_HarnessBlockedRects[g_HarnessBlockedRectCount][1] = y;
	g_HarnessBlockedRects[g_HarnessBlockedRectCount][2] = w;
	g_HarnessBlockedRects[g_HarnessBlockedRectCount][3] = h;
	g_HarnessBlockedRectCount++;

	g_HarnessBlockActive = 1;

	lua_pushboolean(L,1);
	return 1;
}

static int StubClearBlockedRects(lua_State* L)
{
	g_HarnessBlockedRectCount = 0;
	return 0;
}

static int StubSetMouseBlockEnabled(lua_State* L)
{
	g_HarnessBlockActive = (lua_toboolean(L,1) != 0) ? 1 : 0;
	printf("[UI.SetMouseBlockEnabled] %s (stub)\n",g_HarnessBlockActive ? "true" : "false");
	lua_pushboolean(L,g_HarnessBlockActive);
	return 1;
}

static int StubIsMouseInside(lua_State* L)
{
	int x = (int)luaL_checkinteger(L,1);
	int y = (int)luaL_checkinteger(L,2);
	int w = (int)luaL_checkinteger(L,3);
	int h = (int)luaL_checkinteger(L,4);

	lua_pushboolean(L,(g_HarnessCursorX >= x && g_HarnessCursorX <= x+w && g_HarnessCursorY >= y && g_HarnessCursorY <= y+h) ? 1 : 0);
	return 1;
}

static int StubConsumeClick(lua_State* L)
{
	int c = g_HarnessConsumed;
	g_HarnessConsumed = 0;
	printf("[UI.ConsumeClick] -> %s (stub)\n",c ? "true" : "false");
	lua_pushboolean(L,c);
	return 1;
}

static int StubIsMouseBlockHooked(lua_State* L)
{
	printf("[UI.IsMouseBlockHooked] -> true (stub)\n");
	lua_pushboolean(L,1);
	return 1;
}

static int StubMouseClickCallSite(lua_State* L)
{
	lua_pushinteger(L,0x007D2B0C);
	return 1;
}

// --- Stubs v0.3.1: estado por capa del bloqueo (WndProc guard + SendGuard) ---
static int StubWndProcInstalled(lua_State* L)
{
	printf("[UI.WndProcInstalled] -> true (stub)\n");
	lua_pushboolean(L,1);
	return 1;
}

static int StubSendGuardInstalled(lua_State* L)
{
	printf("[UI.SendGuardInstalled] -> true (stub)\n");
	lua_pushboolean(L,1);
	return 1;
}

static int StubBlockedByHook(lua_State* L)
{
	printf("[UI.BlockedByHook] -> 0 (stub)\n");
	lua_pushinteger(L,0);
	return 1;
}

static int StubBlockedByWndProc(lua_State* L)
{
	printf("[UI.BlockedByWndProc] -> 0 (stub)\n");
	lua_pushinteger(L,0);
	return 1;
}

static int StubBlockedBySend(lua_State* L)
{
	printf("[UI.BlockedBySend] -> 0 (stub)\n");
	lua_pushinteger(L,0);
	return 1;
}

static int StubCursorInsideUI(lua_State* L)
{
	int inside = 0;

	if(g_HarnessBlockActive)
	{
		for(int n=0;n<g_HarnessBlockedRectCount;n++)
		{
			int x = g_HarnessBlockedRects[n][0];
			int y = g_HarnessBlockedRects[n][1];
			int w = g_HarnessBlockedRects[n][2];
			int h = g_HarnessBlockedRects[n][3];

			if(g_HarnessCursorX >= x && g_HarnessCursorX <= x+w
				&& g_HarnessCursorY >= y && g_HarnessCursorY <= y+h)
			{
				inside = 1;
				break;
			}
		}
	}

	printf("[UI.CursorInsideUI] -> %s (stub)\n",inside ? "true" : "false");
	lua_pushboolean(L,inside);
	return 1;
}

static int StubSendPacket(lua_State* L)
{
	const char* hex = luaL_checkstring(L,1);
	printf("[SendPacket] %s\n",hex);
	g_HarnessSendCount++;
	lua_pushboolean(L,1);
	return 1;
}

static int StubLog(lua_State* L)
{
	const char* t = luaL_checkstring(L,1);
	printf("[Log] %s\n",t);
	return 0;
}

// ---------------------------------------------------------------------------
// Registro (igual estructura que RegisterLuaFunctions del plugin)
// ---------------------------------------------------------------------------

static void RegisterStubs(lua_State* L)
{
	lua_createtable(L,0,7);
	lua_pushcfunction(L,StubDrawText);     lua_setfield(L,-2,"Text");
	lua_pushcfunction(L,StubDrawBar);      lua_setfield(L,-2,"Bar");
	lua_pushcfunction(L,StubDrawMessage);  lua_setfield(L,-2,"Message");
	lua_pushcfunction(L,StubDrawImage);    lua_setfield(L,-2,"Image");
	lua_pushcfunction(L,StubLoadImage);    lua_setfield(L,-2,"LoadImage");
	lua_pushcfunction(L,StubDrawTooltip);  lua_setfield(L,-2,"Tooltip");
	lua_setglobal(L,"Draw");

	lua_createtable(L,0,4);
	lua_pushcfunction(L,StubCursorX);      lua_setfield(L,-2,"CursorX");
	lua_pushcfunction(L,StubCursorY);      lua_setfield(L,-2,"CursorY");
	lua_pushcfunction(L,StubRegisterKey);  lua_setfield(L,-2,"RegisterKey");
	lua_pushcfunction(L,StubKeyPressed);   lua_setfield(L,-2,"KeyPressed");
	lua_setglobal(L,"Input");

	lua_createtable(L,0,5);
	lua_pushcfunction(L,StubInterfaceOpen);   lua_setfield(L,-2,"Open");
	lua_pushcfunction(L,StubInterfaceClose);  lua_setfield(L,-2,"Close");
	lua_pushcfunction(L,StubInterfaceIsOpen); lua_setfield(L,-2,"IsOpen");
	lua_pushcfunction(L,StubInterfaceGetOpenWindows); lua_setfield(L,-2,"GetOpenWindows");
	lua_pushcfunction(L,StubInterfaceGetActiveWindow); lua_setfield(L,-2,"GetActiveWindow");
	lua_setglobal(L,"Interface");

	lua_createtable(L,0,13);
	lua_pushcfunction(L,StubBlockMouse);        lua_setfield(L,-2,"BlockMouse");
	lua_pushcfunction(L,StubClearBlockedRects); lua_setfield(L,-2,"ClearBlockedRects");
	lua_pushcfunction(L,StubSetMouseBlockEnabled); lua_setfield(L,-2,"SetMouseBlockEnabled");
	lua_pushcfunction(L,StubIsMouseInside);     lua_setfield(L,-2,"IsMouseInside");
	lua_pushcfunction(L,StubConsumeClick);      lua_setfield(L,-2,"ConsumeClick");
	lua_pushcfunction(L,StubIsMouseBlockHooked); lua_setfield(L,-2,"IsMouseBlockHooked");
	lua_pushcfunction(L,StubMouseClickCallSite); lua_setfield(L,-2,"MouseClickCallSite");
	// v0.3.1: estado por capa
	lua_pushcfunction(L,StubWndProcInstalled);   lua_setfield(L,-2,"WndProcInstalled");
	lua_pushcfunction(L,StubSendGuardInstalled); lua_setfield(L,-2,"SendGuardInstalled");
	lua_pushcfunction(L,StubBlockedByHook);      lua_setfield(L,-2,"BlockedByHook");
	lua_pushcfunction(L,StubBlockedByWndProc);   lua_setfield(L,-2,"BlockedByWndProc");
	lua_pushcfunction(L,StubBlockedBySend);      lua_setfield(L,-2,"BlockedBySend");
	lua_pushcfunction(L,StubCursorInsideUI);     lua_setfield(L,-2,"CursorInsideUI");
	lua_setglobal(L,"UI");

	lua_createtable(L,0,41);
	lua_pushcfunction(L,StubScreenState);  lua_setfield(L,-2,"ScreenState");
	lua_pushcfunction(L,StubInGame);       lua_setfield(L,-2,"InGame");
	lua_pushcfunction(L,StubResolutionX);  lua_setfield(L,-2,"ResolutionX");
	lua_pushcfunction(L,StubResolutionY);  lua_setfield(L,-2,"ResolutionY");
	lua_pushcfunction(L,StubCharacterName);lua_setfield(L,-2,"CharacterName");
	lua_pushcfunction(L,StubCharacterLevel);   lua_setfield(L,-2,"Level");
	lua_pushcfunction(L,StubCharacterClass);   lua_setfield(L,-2,"Class");
	lua_pushcfunction(L,StubCharacterHP);      lua_setfield(L,-2,"HP");
	lua_pushcfunction(L,StubCharacterMaxHP);   lua_setfield(L,-2,"MaxHP");
	lua_pushcfunction(L,StubCharacterMP);      lua_setfield(L,-2,"MP");
	lua_pushcfunction(L,StubCharacterMaxMP);   lua_setfield(L,-2,"MaxMP");
	lua_pushcfunction(L,StubCharacterShield);  lua_setfield(L,-2,"Shield");
	lua_pushcfunction(L,StubCharacterMaxShield); lua_setfield(L,-2,"MaxShield");
	lua_pushcfunction(L,StubCharacterBP);      lua_setfield(L,-2,"BP");
	lua_pushcfunction(L,StubCharacterMaxBP);   lua_setfield(L,-2,"MaxBP");
	lua_pushcfunction(L,StubCharacterStrength);lua_setfield(L,-2,"Strength");
	lua_pushcfunction(L,StubCharacterDexterity);lua_setfield(L,-2,"Dexterity");
	lua_pushcfunction(L,StubCharacterVitality);lua_setfield(L,-2,"Vitality");
	lua_pushcfunction(L,StubCharacterEnergy);  lua_setfield(L,-2,"Energy");
	lua_pushcfunction(L,StubCharacterLeadership);lua_setfield(L,-2,"Leadership");
	lua_pushcfunction(L,StubCharacterLevelUpPoint);lua_setfield(L,-2,"LevelUpPoint");
	lua_pushcfunction(L,StubCharacterExperience);lua_setfield(L,-2,"Experience");
	lua_pushcfunction(L,StubCharacterNextExperience);lua_setfield(L,-2,"NextExperience");
	lua_pushcfunction(L,StubCharacterMoney);  lua_setfield(L,-2,"Money");
	lua_pushcfunction(L,StubCharacterMap);    lua_setfield(L,-2,"Map");
	// Estados cacheados por packets (FASE 7)
	lua_pushcfunction(L,StubClientIsTradeOpen);      lua_setfield(L,-2,"IsTradeOpen");
	lua_pushcfunction(L,StubClientIsTradeAccepted);  lua_setfield(L,-2,"IsTradeAccepted");
	lua_pushcfunction(L,StubClientGetTradeMoney);    lua_setfield(L,-2,"GetTradeMoney");
	lua_pushcfunction(L,StubClientGetTradeItem);     lua_setfield(L,-2,"GetTradeItem");
	lua_pushcfunction(L,StubClientIsShopOpen);       lua_setfield(L,-2,"IsShopOpen");
	lua_pushcfunction(L,StubClientIsChaosBoxOpen);   lua_setfield(L,-2,"IsChaosBoxOpen");
	// NO CONFIRMADOS (devuelven nil/false/-1/"")
	lua_pushcfunction(L,StubClientIsNpcDialogOpen);  lua_setfield(L,-2,"IsNpcDialogOpen");
	lua_pushcfunction(L,StubClientGetNpcIndex);      lua_setfield(L,-2,"GetNpcIndex");
	lua_pushcfunction(L,StubClientGetNpcName);       lua_setfield(L,-2,"GetNpcName");
	lua_pushcfunction(L,StubClientGetInventoryItem); lua_setfield(L,-2,"GetInventoryItem");
	lua_pushcfunction(L,StubClientIsInventorySlotEmpty); lua_setfield(L,-2,"IsInventorySlotEmpty");
	lua_pushcfunction(L,StubClientGetWearItem);      lua_setfield(L,-2,"GetWearItem");
	lua_pushcfunction(L,StubClientGetShopItem);      lua_setfield(L,-2,"GetShopItem");
	lua_pushcfunction(L,StubClientGetShopPrice);     lua_setfield(L,-2,"GetShopPrice");
	lua_pushcfunction(L,StubClientGetChaosItem);     lua_setfield(L,-2,"GetChaosItem");
	lua_setglobal(L,"Client");

	lua_register(L,"SendPacket",StubSendPacket);
	lua_register(L,"Log",StubLog);

	lua_register(L,"RegisterPacketHandler",StubRegisterPacketHandler);
	lua_register(L,"UnregisterPacketHandler",StubUnregisterPacketHandler);
}

// ---------------------------------------------------------------------------
// Dispatch de packets: primero el router, luego on_packet global
// ---------------------------------------------------------------------------

static int DispatchPacket(lua_State* L,int head,int sub,const char* data)
{
	PACKET_HANDLER* handler = FindPacketHandler(head,sub);

	if(handler != 0)
	{
		lua_rawgeti(L,LUA_REGISTRYINDEX,handler->luaRef);

		lua_pushinteger(L,head);
		lua_pushinteger(L,sub);
		lua_pushstring(L,data);

		if(lua_pcall(L,3,1,0) != 0)
		{
			fprintf(stderr,"ERROR en handler de packet: %s\n",lua_tostring(L,-1));
			lua_pop(L,1);
			return -1;
		}

		int handled = lua_toboolean(L,-1);

		lua_pop(L,1);

		printf("[router 0x%02X:%d devolvio %s]\n",head,sub,handled ? "true (consumido)" : "false (re-enviado)");

		return handled;
	}

	lua_getglobal(L,"on_packet");

	if(lua_isfunction(L,-1) == 0)
	{
		fprintf(stderr,"ERROR: on_packet no es una funcion\n");
		lua_pop(L,1);
		return -1;
	}

	lua_pushinteger(L,head);
	lua_pushinteger(L,sub);
	lua_pushstring(L,data);

	if(lua_pcall(L,3,1,0) != 0)
	{
		fprintf(stderr,"ERROR en on_packet: %s\n",lua_tostring(L,-1));
		lua_pop(L,1);
		return -1;
	}

	int handled = lua_toboolean(L,-1);

	lua_pop(L,1);

	printf("[on_packet devolvio %s]\n",handled ? "true (consumido)" : "false (re-enviado)");

	return handled;
}

static int CallOnDraw(lua_State* L,int cx,int cy)
{
	lua_getglobal(L,"on_draw");

	if(lua_isfunction(L,-1) == 0)
	{
		fprintf(stderr,"ERROR: on_draw no es una funcion\n");
		lua_pop(L,1);
		return -1;
	}

	lua_pushinteger(L,cx);
	lua_pushinteger(L,cy);

	if(lua_pcall(L,2,0,0) != 0)
	{
		fprintf(stderr,"ERROR en on_draw: %s\n",lua_tostring(L,-1));
		lua_pop(L,1);
		return -1;
	}

	return 0;
}

// Simula un click del raton (down o up) con el cursor en (x,y)
static void ClickAt(lua_State* L,int x,int y,int down)
{
	g_HarnessCursorX = x;
	g_HarnessCursorY = y;

	g_HarnessKeyState[0x01] = down ? 1 : 0;
	PollInput(L);
}

// ---------------------------------------------------------------------------
// Helpers de lectura del estado expuesto por el script (UI_TEST)
// ---------------------------------------------------------------------------

// lua_tointeger NO convierte booleanos (devuelve 0 para true): estos helpers
// devuelven 1/0 para booleanos y el numero para numeros.
static int LuaValueToInt(lua_State* L)
{
	if(lua_isboolean(L,-1) != 0)
	{
		return lua_toboolean(L,-1) ? 1 : 0;
	}

	return (int)lua_tointeger(L,-1);
}

// Lee UI_TEST.<a>.<key> (numero o booleano -> 1/0)
static int ReadInt2(lua_State* L,const char* a,const char* key)
{
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,a);
	lua_getfield(L,-1,key);
	int v = LuaValueToInt(L);
	lua_pop(L,3);
	return v;
}

// Lee UI_TEST.<a>.<b>.<key> (numero o booleano -> 1/0)
static int ReadInt3(lua_State* L,const char* a,const char* b,const char* key)
{
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,a);
	lua_getfield(L,-1,b);
	lua_getfield(L,-1,key);
	int v = LuaValueToInt(L);
	lua_pop(L,4);
	return v;
}

// Lee UI_TEST.pcache.trade_items[slot].<field> (numero)
static int ReadTradeItem(lua_State* L,int slot,const char* field)
{
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,"pcache");
	lua_getfield(L,-1,"trade_items");
	lua_rawgeti(L,-1,slot);
	lua_getfield(L,-1,field);
	int v = (int)lua_tointeger(L,-1);
	lua_pop(L,5);
	return v;
}

// Titulo de la ultima ventana del z-order (UI_TEST.ui.windows[#].title)
static int ReadTopWindowTitle(lua_State* L,char* out,int outLen)
{
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,"ui");
	lua_getfield(L,-1,"windows");
	lua_rawgeti(L,-1,(int)lua_rawlen(L,-1)); // ultima
	lua_getfield(L,-1,"title");
	const char* t = lua_tostring(L,-1);
	strncpy(out,t ? t : "?",outLen-1);
	out[outLen-1] = 0;
	lua_pop(L,5);
	return 0;
}

// Llama Config.getint(clave) del script y devuelve el resultado
static int CallConfigGetint(lua_State* L,const char* key)
{
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,"config");
	lua_getfield(L,-1,"getint");
	lua_remove(L,-2); // fn

	lua_pushstring(L,key);

	if(lua_pcall(L,1,1,0) != 0)
	{
		fprintf(stderr,"ERROR en Config.getint: %s\n",lua_tostring(L,-1));
		lua_pop(L,3);
		return -1;
	}

	int v = (int)lua_tointeger(L,-1);

	lua_pop(L,2);

	return v;
}

// Llama una funcion Client.* (sin argumentos) desde Lua y devuelve el int
static int CallClientInt(lua_State* L,const char* fn)
{
	lua_getglobal(L,"Client");
	lua_getfield(L,-1,fn);
	lua_remove(L,-2); // fn

	if(lua_pcall(L,0,1,0) != 0)
	{
		fprintf(stderr,"ERROR en Client.%s: %s\n",fn,lua_tostring(L,-1));
		lua_pop(L,1);
		return -1;
	}

	int v = LuaValueToInt(L);

	lua_pop(L,1);

	return v;
}

static bool FileExists(const char* path)
{
	FILE* f = fopen(path,"r");

	if(f)
	{
		fclose(f);
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// Tests del parser de dinero (MoneyCache.h)
// ---------------------------------------------------------------------------

static int g_MoneyTestFailures = 0;
static int g_MoneyTestCount = 0;

static void CheckMoneyParse(const char* name,BYTE head,int sub,BYTE* pkt,int size,bool expectParsed,DWORD expectValue)
{
	g_MoneyTestCount++;

	DWORD out = 0;
	bool parsed = ParseMoneyFromPacket(head,sub,pkt,size,&out);

	if(parsed != expectParsed || (expectParsed && out != expectValue))
	{
		fprintf(stderr,"FAIL [%s]: parsed=%s value=%u (esperado: parsed=%s value=%u)\n",
			name,parsed ? "true" : "false",out,expectParsed ? "true" : "false",expectValue);
		g_MoneyTestFailures++;
	}
	else
	{
		printf("ok   [%s]: parsed=%s value=%u\n",name,parsed ? "true" : "false",out);
	}
}

static void TestMoneyParser()
{
	printf("--- parser de dinero (MoneyCache.h) ---\n");

	BYTE pktF303[56];
	memset(pktF303,0,sizeof(pktF303));

	pktF303[0] = 0xC3;
	pktF303[1] = 56;
	pktF303[2] = 0xF3;
	pktF303[3] = 0x03;
	pktF303[4] = 128;
	pktF303[5] = 100;
	pktF303[6] = 0;
	pktF303[7] = 3;

	pktF303[24] = 0x19; pktF303[25] = 0x00;
	pktF303[26] = 0xE8; pktF303[27] = 0x03;

	pktF303[50] = 0x15;
	pktF303[51] = 0xCD;
	pktF303[52] = 0x5B;
	pktF303[53] = 0x07;

	CheckMoneyParse("F3:03 money LE",0xF3,0x03,pktF303,56,true,123456789);
	CheckMoneyParse("F3:03 tamano justo (54)",0xF3,0x03,pktF303,54,true,123456789);
	CheckMoneyParse("F3:03 corto (53) -> rechaza",0xF3,0x03,pktF303,53,false,0);

	BYTE pkt22FE[8];
	memset(pkt22FE,0,sizeof(pkt22FE));

	pkt22FE[0] = 0xC3;
	pkt22FE[1] = 8;
	pkt22FE[2] = 0x22;
	pkt22FE[3] = 0xFE;
	pkt22FE[4] = 0x00;
	pkt22FE[5] = 0x98;
	pkt22FE[6] = 0x96;
	pkt22FE[7] = 0x7F;

	CheckMoneyParse("C3:22:FE money BE",0x22,0xFE,pkt22FE,8,true,9999999);

	BYTE pktMax[8] = { 0xC3, 8, 0x22, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF };
	CheckMoneyParse("C3:22:FE money max",0x22,0xFE,pktMax,8,true,0xFFFFFFFF);

	CheckMoneyParse("C3:22:FE corto (7) -> rechaza",0x22,0xFE,pkt22FE,7,false,0);

	BYTE pktF30E[8] = { 0xC3, 8, 0xF3, 0x0E, 0, 0, 0, 0 };
	CheckMoneyParse("F3:0E (no money) -> no parsea",0xF3,0x0E,pktF30E,8,false,0);

	BYTE pkt22Other[8] = { 0xC3, 8, 0x22, 0x01, 0, 0, 0, 0 };
	CheckMoneyParse("C3:22 result!=FE -> no parsea",0x22,0x01,pkt22Other,8,false,0);

	CheckMoneyParse("NULL buffer -> no parsea",0xF3,0x03,0,56,false,0);
	CheckMoneyParse("tamano < 3 -> no parsea",0xF3,0x03,pktF303,2,false,0);

	if(g_MoneyTestFailures == 0)
	{
		printf("--- parser de dinero: %d tests OK ---\n",g_MoneyTestCount);
	}
	else
	{
		fprintf(stderr,"ERROR: %d/%d test(s) del parser de dinero fallaron\n",g_MoneyTestFailures,g_MoneyTestCount);
	}
}

// ---------------------------------------------------------------------------
// Tests del parser de estados del cliente (ClientStateCache.h)
// ---------------------------------------------------------------------------

static int g_StateTestFailures = 0;
static int g_StateTestCount = 0;

static void CheckState(const char* name,CLIENT_STATE& st,bool tradeOpen,bool tradeAccepted,DWORD tradeMoney,bool shopOpen,bool chaosSeen)
{
	g_StateTestCount++;

	if(st.tradeOpen != tradeOpen || st.tradeAccepted != tradeAccepted || st.tradeMoney != tradeMoney
		|| st.shopOpen != shopOpen || st.chaosBoxSeen != chaosSeen)
	{
		fprintf(stderr,"FAIL [%s]: trade(%d,%d,%u) shop=%d chaos=%d (esperado trade(%d,%d,%u) shop=%d chaos=%d)\n",
			name,st.tradeOpen,st.tradeAccepted,st.tradeMoney,st.shopOpen,st.chaosBoxSeen,
			tradeOpen,tradeAccepted,tradeMoney,shopOpen,chaosSeen);
		g_StateTestFailures++;
	}
	else
	{
		printf("ok   [%s]: trade(%d,%d,%u) shop=%d chaos=%d\n",name,st.tradeOpen,st.tradeAccepted,st.tradeMoney,st.shopOpen,st.chaosBoxSeen);
	}
}

static void TestClientStateParser()
{
	printf("--- parser de estados (ClientStateCache.h) ---\n");

	CLIENT_STATE st;
	ResetClientState(&st);

	// C3:36 request trade
	BYTE p36[8] = { 0xC3, 8, 0x36, 0x02, 0, 0, 0, 0 };
	if(!ParseClientStatePacket(0x36,0x02,p36,8,&st))
	{
		fprintf(stderr,"FAIL [C3:36]: no parseado\n");
		g_StateTestFailures++;
	}
	else
	{
		printf("ok   [C3:36]: trade abierto\n");
	}
	g_StateTestCount++;

	CheckState("trade request",st,true,false,0,false,false);

	// C1:39 item slot 2 (index 47=0x2F, level 7, dur 100)
	BYTE p39[16] = { 0xC1, 16, 0x39, 2, 0x2F, 0x00, 0x38, 100, 0,0,0,0,0,0,0,0 };
	ParseClientStatePacket(0x39,2,p39,16,&st);
	g_StateTestCount++;

	if(st.tradeItems[2].index != 47 || st.tradeItems[2].level != 7 || st.tradeItems[2].dur != 100)
	{
		fprintf(stderr,"FAIL [C1:39]: item slot2 idx=%d lvl=%d dur=%d (esperado 47/7/100)\n",
			st.tradeItems[2].index,st.tradeItems[2].level,st.tradeItems[2].dur);
		g_StateTestFailures++;
	}
	else
	{
		printf("ok   [C1:39]: item slot2 idx=47 lvl=7 dur=100\n");
	}

	// C1:3B money 12345 LE
	BYTE p3B[8] = { 0xC1, 8, 0x3B, 0x39, 0x30, 0, 0, 0 };
	ParseClientStatePacket(0x3B,1,p3B,8,&st);
	CheckState("trade money",st,true,false,12345,false,false);

	// C1:3C OK -> accepted
	BYTE p3C[8] = { 0xC1, 8, 0x3C, 1, 0, 0, 0, 0 };
	ParseClientStatePacket(0x3C,1,p3C,8,&st);
	CheckState("trade OK",st,true,true,12345,false,false);

	// C1:3D cancel -> reset
	BYTE p3D[4] = { 0xC1, 4, 0x3D, 0 };
	ParseClientStatePacket(0x3D,0,p3D,4,&st);
	CheckState("trade cancel",st,false,false,0,false,false);

	if(st.tradeItems[2].index != 0)
	{
		fprintf(stderr,"FAIL [C1:3D]: los items del trade no se limpiaron\n");
		g_StateTestFailures++;
	}
	else
	{
		printf("ok   [C1:3D]: items del trade limpiados\n");
	}
	g_StateTestCount++;

	// C1:3F:02 shop abierto
	BYTE p3F02[6] = { 0xC1, 6, 0x3F, 0x02, 0, 0 };
	ParseClientStatePacket(0x3F,0x02,p3F02,6,&st);
	CheckState("shop abierto",st,false,false,0,true,false);

	// C1:3F:03 shop cerrado
	BYTE p3F03[6] = { 0xC1, 6, 0x3F, 0x03, 0, 0 };
	ParseClientStatePacket(0x3F,0x03,p3F03,6,&st);
	CheckState("shop cerrado",st,false,false,0,false,false);

	// C1:88 caos
	BYTE p88[8] = { 0xC1, 8, 0x88, 0x01, 0, 0, 0, 0 };
	ParseClientStatePacket(0x88,1,p88,8,&st);
	CheckState("caos visto",st,false,false,0,false,true);

	// Guardas de tamano: no debe crashear ni cambiar estado
	BYTE shortPkt[3] = { 0xC1, 3, 0x36 };
	ParseClientStatePacket(0x36,0,shortPkt,3,&st);
	ParseClientStatePacket(0x39,0,p39,10,&st);  // < 16 -> no parsea item
	if(st.tradeItems[2].index != 0)
	{
		fprintf(stderr,"FAIL [C1:39 corto]: parseo un item con tamano insuficiente\n");
		g_StateTestFailures++;
	}
	else
	{
		printf("ok   [C1:39 corto]: rechazado sin leer fuera de rango\n");
	}
	g_StateTestCount++;

	ParseClientStatePacket(0x3F,0x02,shortPkt,3,&st);
	CheckState("0x3F corto -> rechaza",st,false,false,0,false,true);

	// NULL safe
	ParseClientStatePacket(0x36,0,0,8,&st);

	if(g_StateTestFailures == 0)
	{
		printf("--- parser de estados: %d tests OK ---\n",g_StateTestCount);
	}
	else
	{
		fprintf(stderr,"ERROR: %d/%d test(s) del parser de estados fallaron\n",g_StateTestFailures,g_StateTestCount);
	}
}

// ---------------------------------------------------------------------------
// Tests de la logica real del bloqueo de clicks (MouseBlock.h)
// ---------------------------------------------------------------------------

static int g_BlockTestFailures = 0;
static int g_BlockTestCount = 0;

static void CheckBlock(const char* name,MOUSE_BLOCK_STATE& mb,int cx,int cy,bool expect)
{
	g_BlockTestCount++;

	bool got = mb.ShouldBlock(cx,cy);

	if(got != expect)
	{
		fprintf(stderr,"FAIL [%s]: ShouldBlock(%d,%d)=%s (esperado %s)\n",
			name,cx,cy,got ? "true" : "false",expect ? "true" : "false");
		g_BlockTestFailures++;
	}
	else
	{
		printf("ok   [%s]: ShouldBlock(%d,%d)=%s\n",name,cx,cy,got ? "true" : "false");
	}
}

static void TestMouseBlockState()
{
	printf("--- bloqueo de clicks (MouseBlock.h) ---\n");

	MOUSE_BLOCK_STATE mb;

	// sin rects ni activo -> nunca bloquea
	CheckBlock("inactivo",mb,100,100,false);

	mb.AddRect(10,20,100,50); // rect (10..110, 20..70)

	CheckBlock("dentro del rect",mb,50,40,true);
	CheckBlock("borde incluido",mb,10,20,true);
	CheckBlock("fuera x",mb,200,40,false);
	CheckBlock("fuera y",mb,50,200,false);

	// Clear -> deja de bloquear (active sigue true pero count==0)
	mb.Clear();
	CheckBlock("tras Clear",mb,50,40,false);

	// ConsumeClick: lee y resetea
	if(mb.ConsumeClick() != false) { g_BlockTestFailures++; fprintf(stderr,"FAIL [ConsumeClick sin consumido]\n"); }
	g_BlockTestCount++;

	mb.consumed = true;

	if(mb.ConsumeClick() != true) { g_BlockTestFailures++; fprintf(stderr,"FAIL [ConsumeClick consumido]\n"); }
	g_BlockTestCount++;

	if(mb.ConsumeClick() != false) { g_BlockTestFailures++; fprintf(stderr,"FAIL [ConsumeClick se resetea]\n"); }
	g_BlockTestCount++;

	// limite de rects (MAX_BLOCKED_RECTS) + rechazo del excedente
	MOUSE_BLOCK_STATE mb2;

	bool allOk = true;

	for(int i=0;i<MAX_BLOCKED_RECTS;i++)
	{
		if(!mb2.AddRect(i*10,0,5,5)) { allOk = false; break; }
	}

	g_BlockTestCount++;

	if(!allOk || mb2.AddRect(0,0,1,1))
	{
		fprintf(stderr,"FAIL [limite de rects]: esperado max %d y rechazo del excedente\n",MAX_BLOCKED_RECTS);
		g_BlockTestFailures++;
	}
	else
	{
		printf("ok   [limite de rects]: %d max OK\n",MAX_BLOCKED_RECTS);
	}

	// registrar un rect activa el bloqueo automaticamente
	MOUSE_BLOCK_STATE mb3;

	if(mb3.active != false) { g_BlockTestFailures++; fprintf(stderr,"FAIL [active inicial]\n"); }
	g_BlockTestCount++;

	mb3.AddRect(0,0,10,10);

	if(mb3.active != true) { g_BlockTestFailures++; fprintf(stderr,"FAIL [AddRect activa el bloqueo]\n"); }
	g_BlockTestCount++;

	if(g_BlockTestFailures == 0)
	{
		printf("--- mouse block: %d tests OK ---\n",g_BlockTestCount);
	}
	else
	{
		fprintf(stderr,"ERROR: %d/%d test(s) de mouse block fallaron\n",g_BlockTestFailures,g_BlockTestCount);
	}
}

// ---------------------------------------------------------------------------
// Tests de la capa 2 (ShouldSwallowClickMessage) y capa 3 (ShouldBlockPacket)
// ---------------------------------------------------------------------------

static int g_LayerTestFailures = 0;
static int g_LayerTestCount = 0;

static void CheckSwallow(const char* name,const MOUSE_BLOCK_STATE& mb,UINT msg,int x,int y,bool expect)
{
	g_LayerTestCount++;

	bool got = ShouldSwallowClickMessage(mb,msg,x,y);

	if(got != expect)
	{
		fprintf(stderr,"FAIL [%s]: msg=0x%04X(%d,%d)=%s (esperado %s)\n",
			name,msg,x,y,got ? "true" : "false",expect ? "true" : "false");
		g_LayerTestFailures++;
	}
	else
	{
		printf("ok   [%s]: msg=0x%04X(%d,%d)=%s\n",name,msg,x,y,got ? "true" : "false");
	}
}

static void CheckBlockPacket(const char* name,const MOUSE_BLOCK_STATE& mb,int cx,int cy,const BYTE* pkt,int size,bool expect)
{
	g_LayerTestCount++;

	bool got = ShouldBlockPacket(mb,cx,cy,pkt,size);

	if(got != expect)
	{
		fprintf(stderr,"FAIL [%s]: op=0x%02X =%s (esperado %s)\n",
			name,(pkt && size>=3) ? pkt[2] : 0,got ? "true" : "false",expect ? "true" : "false");
		g_LayerTestFailures++;
	}
	else
	{
		printf("ok   [%s]: op=0x%02X =%s\n",name,(pkt && size>=3) ? pkt[2] : 0,got ? "true" : "false");
	}
}

static void TestBlockLayers()
{
	printf("--- capas 2 y 3 del bloqueo (MouseBlock.h) ---\n");

	MOUSE_BLOCK_STATE mb;

	mb.AddRect(10,20,100,50); // rect (10..110, 20..70)

	// CAPA 2 - WndProc: tragar mensajes de raton sobre la UI
	CheckSwallow("LBUTTONDOWN dentro",mb,WM_LBUTTONDOWN,50,40,true);
	CheckSwallow("LBUTTONUP dentro",mb,WM_LBUTTONUP,50,40,true);
	CheckSwallow("LBUTTONDBLCLK dentro",mb,WM_LBUTTONDBLCLK,50,40,true);
	CheckSwallow("RBUTTONDOWN dentro",mb,WM_RBUTTONDOWN,50,40,true);
	CheckSwallow("MBUTTONDOWN dentro",mb,WM_MBUTTONDOWN,50,40,true);
	CheckSwallow("LBUTTONDOWN fuera",mb,WM_LBUTTONDOWN,300,300,false);
	CheckSwallow("MOUSEMOVE dentro (no se traga)",mb,WM_MOUSEMOVE,50,40,false);

	// Tras Clear no se traga nada
	MOUSE_BLOCK_STATE mbCleared = mb;
	mbCleared.Clear();
	CheckSwallow("LBUTTONDOWN tras Clear",mbCleared,WM_LBUTTONDOWN,50,40,false);

	// CAPA 3 - SendGuard: tragar C1:04/05/06 sobre la UI
	BYTE pMove[7]  = { 0xC1, 7, 0x05, 0x11, 0x22, 0x33, 0x44 }; // mover
	BYTE pAttack[7]= { 0xC1, 7, 0x04, 0x11, 0x22, 0x33, 0x44 }; // ataque
	BYTE pCancel[7]= { 0xC1, 7, 0x06, 0x11, 0x22, 0x33, 0x44 }; // cancelar/parar
	BYTE pChat[7]  = { 0xC1, 7, 0x00, 0x48, 0x69, 0x21, 0x00 }; // chat (NO bloquear)
	BYTE pC3Move[7]= { 0xC3, 7, 0x05, 0x11, 0x22, 0x33, 0x44 }; // C3 -> NO bloquear
	BYTE pShort[3] = { 0xC1, 3, 0x05 };

	CheckBlockPacket("C1:05 mover dentro",mb,50,40,pMove,7,true);
	CheckBlockPacket("C1:04 ataque dentro",mb,50,40,pAttack,7,true);
	CheckBlockPacket("C1:06 cancelar dentro",mb,50,40,pCancel,7,true);
	CheckBlockPacket("C1:05 fuera del rect",mb,300,300,pMove,7,false);
	CheckBlockPacket("C1:00 chat dentro",mb,50,40,pChat,7,false);
	CheckBlockPacket("C3:05 no se toca",mb,50,40,pC3Move,7,false);
	CheckBlockPacket("tamano 3 C1:05 si bloquea",mb,50,40,pShort,3,true); // 0xC1,0x03,0x05: packet minimo de mover
	CheckBlockPacket("tamano < 3",mb,50,40,pShort,2,false);
	CheckBlockPacket("buffer NULL",mb,50,40,0,7,false);

	// sin rects ni activo -> nunca bloquea el envio
	MOUSE_BLOCK_STATE mbInactive;
	CheckBlockPacket("inactivo C1:05",mbInactive,50,40,pMove,7,false);

	// Desactivado explicitamente (SetMouseBlockEnabled(false)) -> no bloquea
	MOUSE_BLOCK_STATE mbOff;
	mbOff.AddRect(10,20,100,50);
	mbOff.active = false;
	CheckBlockPacket("desactivado C1:05",mbOff,50,40,pMove,7,false);

	if(g_LayerTestFailures == 0)
	{
		printf("--- capas 2 y 3: %d tests OK ---\n",g_LayerTestCount);
	}
	else
	{
		fprintf(stderr,"ERROR: %d/%d test(s) de las capas 2 y 3 fallaron\n",g_LayerTestFailures,g_LayerTestCount);
	}
}

// ---------------------------------------------------------------------------

int main(int argc,char* argv[])
{
	TestMoneyParser();

	if(g_MoneyTestFailures != 0)
	{
		fprintf(stderr,"ABORT: el parser de dinero fallo, no se continua con el resto del test\n");
		return 1;
	}

	TestClientStateParser();

	if(g_StateTestFailures != 0)
	{
		fprintf(stderr,"ABORT: el parser de estados fallo, no se continua con el resto del test\n");
		return 1;
	}

	TestMouseBlockState();

	if(g_BlockTestFailures != 0)
	{
		fprintf(stderr,"ABORT: la logica de mouse block fallo, no se continua con el resto del test\n");
		return 1;
	}

	TestBlockLayers();

	if(g_LayerTestFailures != 0)
	{
		fprintf(stderr,"ABORT: la logica de las capas 2 y 3 fallo, no se continua con el resto del test\n");
		return 1;
	}

	const char* script = (argc >= 2) ? argv[1] : "..\\lua_scripts\\main.lua";
	const char* cfgPath = "test_config.txt";

	// FASE 2: si no existe, el script debe crearlo con defaults
	remove(cfgPath);

	lua_State* L = luaL_newstate();

	if(L == 0)
	{
		fprintf(stderr,"ERROR: sin memoria para lua_State\n");
		return 1;
	}

	luaL_openlibs(L);

	RegisterStubs(L);

	// El script lee CONFIG_PATH_OVERRIDE para no escribir Lua\config.txt aqui
	lua_pushstring(L,cfgPath);
	lua_setglobal(L,"CONFIG_PATH_OVERRIDE");

	if(luaL_loadfile(L,script) != 0 || lua_pcall(L,0,0,0) != 0)
	{
		fprintf(stderr,"ERROR cargando %s: %s\n",script,lua_tostring(L,-1));
		lua_pop(L,1);
		lua_close(L);
		return 1;
	}

	printf("--- %s cargado OK ---\n",script);

	// --- FASE 2: el archivo de config se creo con defaults ---
	printf("--- config: archivo creado al cargar ---\n");

	if(!FileExists(cfgPath))
	{
		fprintf(stderr,"ERROR: %s no fue creado por Config.load()\n",cfgPath);
		lua_close(L);
		return 1;
	}

	printf("[config] %s creado OK (defaults)\n",cfgPath);

	if(CallConfigGetint(L,"inv_window") != 0)
	{
		fprintf(stderr,"ERROR: inv_window default no es 0\n");
		lua_close(L);
		return 1;
	}

	// --- Frames de dibujo ---
	for(int i=0;i<3;i++)
	{
		printf("--- frame %d ---\n",i+1);
		if(CallOnDraw(L,320,240) != 0) { lua_close(L); return 1; }
	}

	printf("--- bucle de 500 frames (estabilidad on_draw) ---\n");
	for(int i=0;i<500;i++)
	{
		if(CallOnDraw(L,100,50) != 0) { lua_close(L); return 1; }
	}

	// --- Diagnostico: el cursor esta dentro de un rect de UI? (v0.3.2) ---
	printf("--- diagnostico cursor dentro de UI (UI.CursorInsideUI) ---\n");

	// cursor (100,80): dentro del mainPanel (40,60,260,176). El stub del cursor
	// lee el estado estatico del harness (como la DLL lee pCursorX/pCursorY), asi
	// que hay que posicionarlo igual que las coordenadas que se pasan a on_draw.
	g_HarnessCursorX = 100;
	g_HarnessCursorY = 80;

	if(CallOnDraw(L,100,80) != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"mouse_block","inside") != 1)
	{
		fprintf(stderr,"ERROR: el cursor (100,80) deberia estar DENTRO de la UI (inside=1)\n");
		lua_close(L);
		return 1;
	}

	printf("[cursor dentro] (100,80) dentro=SI OK\n");

	// cursor (500,400): fuera de todas las ventanas
	g_HarnessCursorX = 500;
	g_HarnessCursorY = 400;

	if(CallOnDraw(L,500,400) != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"mouse_block","inside") != 0)
	{
		fprintf(stderr,"ERROR: el cursor (500,400) deberia estar FUERA de la UI (inside=0)\n");
		lua_close(L);
		return 1;
	}

	printf("[cursor dentro] (500,400) dentro=NO OK\n");

	// --- Router: el script registra handlers de cache (0x36, 0x39, 0x3F...).
    // C1:FC ya NO se usa en el demo (el canal custom necesita handler C++ en el
    // GameServer; la especificacion dice no depender de el). El consume/passthrough
    // del router se prueba al final con un handler registrado por el propio test.
    printf("--- packets del router (cache del script) ---\n");

    // Sin handler registrado, 0xFC:01 cae al on_packet global -> pasa
    if(DispatchPacket(L,0xFC,0x01,"C1 06 FC 01 01 00 00") != 0) { lua_close(L); return 1; }


	// --- on_packet global: notice C1:0D con prefijo [LUA -> consume ---
	printf("--- notices C1:0D ---\n");

	if(DispatchPacket(L,0x0D,0x01,"C1 22 0D 01 00 00 00 00 00 00 00 00 00 5B 4C 55 41 3A 48 45 41 52 54 42 45 41 54 5D 20 31 30") != 1)
	{
		lua_close(L);
		return 1;
	}

	if(DispatchPacket(L,0x0D,0x01,"C1 12 0D 01 00 00 00 00 00 00 00 00 00 68 69 20 6D 61 70") != 0)
	{
		lua_close(L);
		return 1;
	}

	if(DispatchPacket(L,0x26,0x64,"C1 0A 26 FF 64 00 00 00 00 00") != 0)
	{
		lua_close(L);
		return 1;
	}

	// --- Hotkey Insert (0x2D): toggle del panel (FASE 1) ---
	printf("--- input: Insert toggle + click en PING ---\n");

	for(int k=0;k<2;k++)
	{
		g_HarnessKeyState[0x2D] = 1;
		PollInput(L);
		g_HarnessKeyState[0x2D] = 0;
		PollInput(L);
	}

	// Tras 2 pulsaciones el panel debe estar visible
	if(ReadInt3(L,"panels","main","visible") != 1)
	{
		fprintf(stderr,"ERROR: el panel no quedo visible tras dos pulsaciones de Insert\n");
		lua_close(L);
		return 1;
	}

	printf("[Insert x2] panel visible OK\n");

	// --- Click en PING: envia /luatest (SendChat -> SendPacket) ---
	int sendsBefore = g_HarnessSendCount;

	ClickAt(L,77,177,1); // mainPanel(40,60): boton PING en (48..106, 168..186)
	ClickAt(L,77,177,0);

	if(g_HarnessSendCount != sendsBefore + 1)
	{
		fprintf(stderr,"ERROR: el boton PING no envio el packet\n");
		lua_close(L);
		return 1;
	}

	printf("[PING] SendChat -> SendPacket OK\n");

	// --- Botones CONFIG y SYS: abren sus paneles (REGRESION del bug de
	// closures: referenciaban los locales configPanel/sysPanel declarados
	// DESPUES -> capturaban nil y UI.show(nil) fallaba silenciosamente) ---
	printf("--- botones CONFIG y SYS (abren paneles) ---\n");

	ClickAt(L,48,190,1);  // CONFIG: mainPanel(40,60) + (8,130)
	ClickAt(L,48,190,0);

	if(ReadInt3(L,"panels","config","visible") != 1)
	{
		fprintf(stderr,"ERROR: el boton CONFIG no abrio el panel de configuracion\n");
		lua_close(L);
		return 1;
	}

	printf("[CONFIG] panel Configuracion abierto OK\n");

	ClickAt(L,176,190,1); // SYS: mainPanel(40,60) + (136,130)
	ClickAt(L,176,190,0);

	if(ReadInt3(L,"panels","sys","visible") != 1)
	{
		fprintf(stderr,"ERROR: el boton SYS no abrio el panel Sistema Custom Demo\n");
		lua_close(L);
		return 1;
	}

	printf("[SYS] panel Sistema abierto OK\n");

	// --- Regresion v0.3.3: colores de widgets planos {r,g,b} ---
	// Antes: 3 labels del SYS tenian color anidado { {r,g,b} } -> "bad argument
	// #4 to 'Text'" cada frame en el cliente real (1872 errores en el log).
	// El harness lo detecta por 3 vias: stub estricto, chequeo estatico y
	// el contador de errores pcall del propio script al dibujar el SYS.
	printf("--- regresion v0.3.3: colores planos + draw SYS sin errores ---\n");

	// (1) stub estricto: Draw.Text con un color tabla debe fallar dentro de pcall
	{
		if(luaL_loadstring(L,"local ok = pcall(Draw.Text, 1, 1, 'x', {190,190,190}, 1, 1); return ok and 1 or 0") != 0 || lua_pcall(L,0,1,0) != 0)
		{
			fprintf(stderr,"ERROR: no se pudo ejecutar el test del stub estricto\n");
			lua_close(L);
			return 1;
		}

		int strict = (int)lua_tointeger(L,-1);
		lua_pop(L,1);

		if(strict != 0)
		{
			fprintf(stderr,"ERROR: el stub estricto acepto un color tabla (deberia fallar como la DLL real)\n");
			lua_close(L);
			return 1;
		}

		printf("ok   [stub estricto]: Draw.Text con color tabla -> error (como la DLL real)\n");
	}

	// (2) chequeo estatico: recorrer todos los widgets de todos los paneles
	{
		if(luaL_loadstring(L,
			"local bad = 0\n"
			"for _, w in ipairs(UI_TEST.ui.windows) do\n"
			"  for _, wid in ipairs(w.widgets) do\n"
			"    local col = wid.color\n"
			"    if type(col) == 'function' then col = col() end\n"
			"    if col then\n"
			"      if type(col[1]) ~= 'number' or type(col[2]) ~= 'number' or type(col[3]) ~= 'number' then bad = bad + 1 end\n"
			"    end\n"
			"  end\n"
			"end\n"
			"return bad\n") != 0 || lua_pcall(L,0,1,0) != 0)
		{
			fprintf(stderr,"ERROR: no se pudo ejecutar el chequeo estatico de colores\n");
			lua_close(L);
			return 1;
		}

		int bad = (int)lua_tointeger(L,-1);
		lua_pop(L,1);

		if(bad != 0)
		{
			fprintf(stderr,"ERROR: %d widget(s) con color anidado { {r,g,b} } (regresion v0.3.3)\n",bad);
			lua_close(L);
			return 1;
		}

		printf("ok   [colores planos]: todos los widgets con color {r,g,b} plano\n");
	}

	// (3) dibujar con el SYS abierto: err.count no debe subir (antes subia 1/frame)
	{
		int before = ReadInt2(L,"err","count");

		g_HarnessCursorX = 100;
		g_HarnessCursorY = 120;

		if(CallOnDraw(L,100,120) != 0) { lua_close(L); return 1; }

		int after = ReadInt2(L,"err","count");

		if(after != before)
		{
			fprintf(stderr,"ERROR: dibujar el SYS genero %d error(es) pcall (regresion v0.3.3, colores anidados)\n",after-before);
			lua_close(L);
			return 1;
		}

		printf("ok   [draw SYS limpio]: 0 errores pcall al dibujar (antes: 1 por frame)\n");
	}

	// --- Auto-diagnostico FASE 11 (UI_TEST.diag): toda la API sin FAIL ni AUSENTE ---
	printf("--- auto-diagnostico (UI_TEST.diag.run) ---\n");

	if(luaL_loadstring(L,"return UI_TEST.diag.run()") != 0 || lua_pcall(L,0,1,0) != 0)
	{
		fprintf(stderr,"ERROR: Diag.run fallo: %s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}
	lua_pop(L,1);

	int okC   = ReadInt3(L,"diag","stats","ok");
	int stubC = ReadInt3(L,"diag","stats","stub");
	int failC = ReadInt3(L,"diag","stats","fail");
	int liveC = ReadInt3(L,"diag","stats","live");
	int ausC  = ReadInt3(L,"diag","stats","absent");
	int totC  = ReadInt3(L,"diag","stats","total");

	printf("ok   [diag] OK=%d Stub=%d Live=%d Fail=%d Aus=%d / %d\n",okC,stubC,liveC,failC,ausC,totC);

	if(failC != 0)
	{
		fprintf(stderr,"ERROR: %d check(s) FAIL en el harness (la API debe responder)\n",failC);
		lua_close(L);
		return 1;
	}

	if(ausC != 0)
	{
		fprintf(stderr,"ERROR: %d funcion(es) AUSENTE en el harness\n",ausC);
		lua_close(L);
		return 1;
	}

	if(okC + stubC + liveC + failC + ausC != totC)
	{
		fprintf(stderr,"ERROR: conteo inconsistente (OK+Stub+Live+Fail+Aus=%d != total=%d)\n",okC+stubC+liveC+failC+ausC,totC);
		lua_close(L);
		return 1;
	}

	if(totC < 90)
	{
		fprintf(stderr,"ERROR: muy pocos checks (%d)\n",totC);
		lua_close(L);
		return 1;
	}

	printf("ok   [diag completo]: API completa sin fallos (checks=%d, live=%d, stub=%d)\n",totC,liveC,stubC);

	printf("--- calibracion INV (2 pasos) ---\n");

	ClickAt(L,141,177,1); // boton INV (112..170, 168..186) -> paso 1
	ClickAt(L,141,177,0);

	printf("[sim] usuario abre el inventario con la tecla I -> window 0x0B\n");
	g_HarnessWindows[0x0B] = 1;

	ClickAt(L,141,177,1); // paso 2: detecta 0x0B
	ClickAt(L,141,177,0);

	if(CallConfigGetint(L,"inv_window") != 0x0B)
	{
		fprintf(stderr,"ERROR: inv_window no quedo calibrado a 0x0B (valor=%d)\n",CallConfigGetint(L,"inv_window"));
		lua_close(L);
		return 1;
	}

	// El ID calibrado debe quedar persistido en el archivo
	{
		FILE* f = fopen(cfgPath,"r");
		char line[128];
		bool found = false;

		while(f && fgets(line,sizeof(line),f))
		{
			if(strstr(line,"inv_window = 11") != 0) { found = true; break; }
		}

		if(f) { fclose(f); }

		if(!found)
		{
			fprintf(stderr,"ERROR: inv_window=11 no persistio en %s\n",cfgPath);
			lua_close(L);
			return 1;
		}

		printf("[calibracion] inv_window=0x0B persistido en %s OK\n",cfgPath);
	}

	// --- Arrastre de ventanas (FASE 1) ---
	printf("--- arrastre de la ventana principal ---\n");

	int xBefore = ReadInt3(L,"panels","main","x");
	int yBefore = ReadInt3(L,"panels","main","y");

	ClickAt(L,100,65,1);  // barra de titulo (40..300, 60..76): inicia el arrastre
	CallOnDraw(L,150,90); // el frame actualiza la posicion con el cursor
	ClickAt(L,150,90,0);  // suelta

	int xAfter = ReadInt3(L,"panels","main","x");
	int yAfter = ReadInt3(L,"panels","main","y");

	// _dx = 100-40 = 60, _dy = 65-60 = 5 -> nueva pos = (150-60, 90-5) = (90,85)
	if(xAfter != xBefore + 50 || yAfter != yBefore + 25)
	{
		fprintf(stderr,"ERROR: el arrastre no movio la ventana (%d,%d)->(%d,%d)\n",xBefore,yBefore,xAfter,yAfter);
		lua_close(L);
		return 1;
	}

	printf("[drag] ventana movida a (%d,%d) OK\n",xAfter,yAfter);

	// --- Checkbox + z-order (FASE 1) ---
	printf("--- checkbox de configuracion y z-order ---\n");

	// Mostrar el panel de config desde el exterior (como haria el boton CONFIG)
	lua_getglobal(L,"UI_TEST");
	lua_getfield(L,-1,"panels");
	lua_getfield(L,-1,"config");
	lua_pushboolean(L,1);
	lua_setfield(L,-2,"visible");
	lua_pop(L,3);

	// El checkbox "Bloqueo de clicks" vive en (8,20) dentro de config(340,60)
	int blockBefore = CallConfigGetint(L,"mouse_block");

	ClickAt(L,363,85,1); // abs (348..378, 80..96)
	ClickAt(L,363,85,0);

	if(CallConfigGetint(L,"mouse_block") == blockBefore)
	{
		fprintf(stderr,"ERROR: el checkbox no alterno mouse_block (%d -> %d)\n",blockBefore,CallConfigGetint(L,"mouse_block"));
		lua_close(L);
		return 1;
	}

	printf("[checkbox] mouse_block %d -> %d OK\n",blockBefore,CallConfigGetint(L,"mouse_block"));

	// Al clickear el panel de config, debe quedar al frente (z-order)
	char topTitle[64];
	ReadTopWindowTitle(L,topTitle,sizeof(topTitle));

	if(strcmp(topTitle,"Configuracion") != 0)
	{
		fprintf(stderr,"ERROR: la ventana al frente es \"%s\" (esperada \"Configuracion\")\n",topTitle);
		lua_close(L);
		return 1;
	}

	printf("[z-order] ventana al frente: %s OK\n",topTitle);

	// --- Cache de estados por packets en Lua (FASE 7) ---
	printf("--- packet cache (Lua) ---\n");

	if(DispatchPacket(L,0x36,0x02,"C3 08 36 02 00 00 00 00") != 0) { lua_close(L); return 1; }
	if(DispatchPacket(L,0x39,0x02,"C1 10 39 02 2F 00 38 64 00 00 00 00 00 00 00 00") != 0) { lua_close(L); return 1; }
	if(DispatchPacket(L,0x3B,0x01,"C1 08 3B 39 30 00 00 00") != 0) { lua_close(L); return 1; }
	if(DispatchPacket(L,0x3C,0x01,"C1 08 3C 01 00 00 00 00") != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"pcache","trade_open") != 1 ||
		ReadInt2(L,"pcache","trade_accepted") != 1 ||
		ReadInt2(L,"pcache","trade_money") != 12345 ||
		ReadTradeItem(L,2,"index") != 47 ||
		ReadTradeItem(L,2,"level") != 7)
	{
		fprintf(stderr,"ERROR: el packet cache de trade no quedo con los valores esperados\n");
		lua_close(L);
		return 1;
	}

	printf("[pcache] trade open/aceptado/money/item OK\n");

	if(DispatchPacket(L,0x3D,0x01,"C1 04 3D 00") != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"pcache","trade_open") != 0 || ReadInt2(L,"pcache","trade_money") != 0)
	{
		fprintf(stderr,"ERROR: C1:3D no reseteo el cache de trade\n");
		lua_close(L);
		return 1;
	}

	printf("[pcache] trade cerrado (C1:3D) OK\n");

	if(DispatchPacket(L,0x3F,0x02,"C1 06 3F 02 00 00") != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"pcache","shop_open") != 1)
	{
		fprintf(stderr,"ERROR: C1:3F:02 no marco el shop como abierto\n");
		lua_close(L);
		return 1;
	}

	printf("[pcache] shop abierto (C1:3F:02) OK\n");

	if(DispatchPacket(L,0x3F,0x03,"C1 06 3F 03 00 00") != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"pcache","shop_open") != 0)
	{
		fprintf(stderr,"ERROR: C1:3F:03 no marco el shop como cerrado\n");
		lua_close(L);
		return 1;
	}

	printf("[pcache] shop cerrado (C1:3F:03) OK\n");

	if(DispatchPacket(L,0x88,0x01,"C1 08 88 01 00 00 00 00") != 0) { lua_close(L); return 1; }

	if(ReadInt2(L,"pcache","chaos_seen") != 1)
	{
		fprintf(stderr,"ERROR: C1:88 no marco la caja del caos\n");
		lua_close(L);
		return 1;
	}

	printf("[pcache] caos visto (C1:88) OK\n");

	// --- Client.* estados via el parser C++ real (FASE 5) ---
	printf("--- Client.* desde el cache C++ (parser real) ---\n");

	ResetClientState(&g_HarnessState);

	BYTE p36[8] = { 0xC3, 8, 0x36, 0x02, 0, 0, 0, 0 };
	ParseClientStatePacket(0x36,0x02,p36,8,&g_HarnessState);

	if(CallClientInt(L,"IsTradeOpen") != 1)
	{
		fprintf(stderr,"ERROR: Client.IsTradeOpen() != true tras C3:36\n");
		lua_close(L);
		return 1;
	}

	printf("[Client] IsTradeOpen() tras C3:36 OK\n");

	BYTE p39[16] = { 0xC1, 16, 0x39, 2, 0x2F, 0x00, 0x38, 100, 0,0,0,0,0,0,0,0 };
	ParseClientStatePacket(0x39,2,p39,16,&g_HarnessState);

	lua_getglobal(L,"Client");
	lua_getfield(L,-1,"GetTradeItem");
	lua_remove(L,-2);
	lua_pushinteger(L,2);

	if(lua_pcall(L,1,1,0) != 0)
	{
		fprintf(stderr,"ERROR en Client.GetTradeItem: %s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}

	lua_getfield(L,-1,"index");

	int itemIndex = (int)lua_tointeger(L,-1);

	lua_pop(L,2);

	if(itemIndex != 47)
	{
		fprintf(stderr,"ERROR: Client.GetTradeItem(2).index = %d (esperado 47)\n",itemIndex);
		lua_close(L);
		return 1;
	}

	printf("[Client] GetTradeItem(2).index=47 OK\n");

	BYTE p3F02[6] = { 0xC1, 6, 0x3F, 0x02, 0, 0 };
	ParseClientStatePacket(0x3F,0x02,p3F02,6,&g_HarnessState);

	if(CallClientInt(L,"IsShopOpen") != 1)
	{
		fprintf(stderr,"ERROR: Client.IsShopOpen() != true tras C1:3F:02\n");
		lua_close(L);
		return 1;
	}

	printf("[Client] IsShopOpen() tras C1:3F:02 OK\n");

	// NO CONFIRMADOS: GetInventoryItem debe devolver nil
	lua_getglobal(L,"Client");
	lua_getfield(L,-1,"GetInventoryItem");
	lua_remove(L,-2);
	lua_pushinteger(L,0);

	if(lua_pcall(L,1,1,0) != 0)
	{
		fprintf(stderr,"ERROR en Client.GetInventoryItem: %s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}

	if(!lua_isnil(L,-1))
	{
		fprintf(stderr,"ERROR: Client.GetInventoryItem(0) no devolvio nil (NO CONFIRMADO)\n");
		lua_close(L);
		return 1;
	}

	lua_pop(L,1);

	printf("[Client] GetInventoryItem -> nil (NO CONFIRMADO) OK\n");

	// --- Bloqueo de clicks (FASE 6): click consumido por la DLL ---
	printf("--- mouse block (FASE 6) ---\n");

	if(ReadInt2(L,"mouse_block","hooked") != 1)
	{
		fprintf(stderr,"ERROR: mouse block no esta hookeado (stubs)\n");
		lua_close(L);
		return 1;
	}

	// Simular que el hook de la DLL consumio un click sobre la UI
	g_HarnessConsumed = 1;

	CallOnDraw(L,150,90); // on_draw sondea ConsumeClick

	if(ReadInt2(L,"mouse_block","consumed") != 1)
	{
		fprintf(stderr,"ERROR: UI.ConsumeClick no incremento el contador\n");
		lua_close(L);
		return 1;
	}

	printf("[mouse block] click consumido contado OK\n");

	// --- Unregister del router (regresion del id estable) ---
	printf("--- unregister router ---\n");

	if(luaL_dostring(L,"reg_id = RegisterPacketHandler(0xAA, nil, function() return true end)") != 0)
	{
		fprintf(stderr,"ERROR registrando handler de prueba: %s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}

	// Consume
	if(DispatchPacket(L,0xAA,0x01,"C1 05 AA 01 00 00") != 1)
	{
		fprintf(stderr,"ERROR: el handler de prueba no consumio\n");
		lua_close(L);
		return 1;
	}

	if(luaL_dostring(L,"UnregisterPacketHandler(reg_id)") != 0)
	{
		fprintf(stderr,"ERROR en UnregisterPacketHandler: %s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}

	// Tras desregistrar cae al on_packet global (que no lo maneja) -> pasa
	if(DispatchPacket(L,0xAA,0x01,"C1 05 AA 01 00 00") != 0)
	{
		fprintf(stderr,"ERROR: se esperaba passthrough tras unregister\n");
		lua_close(L);
		return 1;
	}

	printf("[unregister] handler id estable OK\n");

	lua_close(L);

	printf("--- TEST OK: main.lua v0.3.6 (UI framework, config, calibracion, packet cache, mouse block 3 capas, DIAG) ---\n");
	return 0;
}
