// ============================================================================
// ClientStateCache.h
// Cache de estados del cliente a partir de los packets del servidor.
//
// Algunos estados del juego (trade, personal shop, caja del caos) no viven en
// MAIN_CHARACTER_STRUCT y las estructuras de las ventanas nativas estan en el
// codigo cerrado del main.exe (no documentadas en el source viejo). La forma
// SEGURA de conocerlos es parsear los packets que el servidor envia (ya
// descifrados por main.dll antes de llegar al ProtocolCore).
//
// Layouts confirmados contra SOURCE viejos/Source/Emulator/GameServer
// (Trade.h/cpp, PersonalShop.h/cpp, ChaosBox.cpp) y documentados en
// REPORTE_CLIENTE_EX603.txt (seccion 5.5 y 6.2/6.3/6.5):
//
//   C3:36  request trade (server->client)      -> abre ventana de trade
//   C1:37  response trade (Response DWORD)     -> acepta(1)/declina(0)
//   C1:39  item trade anadido                  -> slot + ItemInfo[12]
//   C1:3B  dinero trade (DWORD LE)             -> money
//   C1:3C  boton OK (flag)                     -> trade aceptado
//   C1:3A  resultado trade (completo)          -> cierra trade
//   C1:3D  cancelar / resultado                -> cierra trade
//   C1:3F:02  abrir personal shop              -> shop abierto
//   C1:3F:03  cerrar personal shop             -> shop cerrado
//   C1:3F:12  leave personal shop              -> shop cerrado
//   C2:3F:05 / C2:3F:13  lista de items shop   -> evidencia de shop abierto
//   C1:88  rate de la caja del caos (recv)     -> evidencia de caja del caos
//
// NOTA: el NPC dialog (C1:30/C1:31) lo ENVIA el cliente (no se ve por el hook
// de recepcion) -> NO se cachea aqui; se aproxima desde Lua con las ventanas
// nativas calibradas (ver main.lua). El layout de los SLOTS de inventario del
// personaje tampoco esta documentado en el source viejo -> NO CONFIRMADO.
//
// Este header es COMPARTIDO entre el plugin (LuaPlugin.cpp) y el test harness
// (test/test_lua.cpp) para que el test ejercite EXACTAMENTE el mismo codigo
// que corre dentro del cliente.
// ============================================================================

#ifndef CLIENTSTATECACHE_H
#define CLIENTSTATECACHE_H

#include <windows.h>

#define TRADE_MAX_ITEMS 32
#define SHOP_MAX_ITEMS  64

// Item de trade cacheado (C1:39 -> ItemInfo[12])
struct CACHED_TRADE_ITEM
{
	int slot;   // slot del trade (0..31)
	int index;  // item index (ItemInfo[0..1] little-endian)
	int level;  // nivel (ItemInfo[2] >> 3 & 0x0F)
	int dur;    // durabilidad (ItemInfo[3])
};

struct CLIENT_STATE
{
	bool tradeOpen;        // C3:36 / C1:37 aceptado -> true; C1:3A/3D -> false
	bool tradeAccepted;    // C1:3C (ambos OK) -> true
	DWORD tradeMoney;      // C1:3B (zen en el trade)
	CACHED_TRADE_ITEM tradeItems[TRADE_MAX_ITEMS];
	bool shopOpen;         // C1:3F:02 -> true; C1:3F:03/0x12 -> false
	bool chaosBoxSeen;     // C1:88 recibido (el jugador uso la caja del caos)
};

// Pone el cache en cero.
static inline void ResetClientState(CLIENT_STATE* st)
{
	if(st == 0)
	{
		return;
	}

	st->tradeOpen = false;
	st->tradeAccepted = false;
	st->tradeMoney = 0;
	st->shopOpen = false;
	st->chaosBoxSeen = false;

	for(int n = 0; n < TRADE_MAX_ITEMS; n++)
	{
		st->tradeItems[n].slot = 0;
		st->tradeItems[n].index = 0;
		st->tradeItems[n].level = 0;
		st->tradeItems[n].dur = 0;
	}
}

// Parsea un packet recibido y actualiza el cache. Devuelve true si el packet
// era relevante (aunque solo cambie un flag). NULL-safe y con chequeos de
// tamano: nunca lee fuera del buffer.
static inline bool ParseClientStatePacket(BYTE head,int sub,BYTE* lpMsg,int msgSize,CLIENT_STATE* st)
{
	if(lpMsg == 0 || st == 0 || msgSize < 3)
	{
		return false;
	}

	// --- TRADE ---
	if(head == 0x36) // C3:36 request trade (server->cliente) -> ventana abierta
	{
		if(msgSize >= 4)
		{
			st->tradeOpen = true;
			st->tradeAccepted = false;
			return true;
		}
		return false;
	}

	if(head == 0x37) // C1:37 response: Response DWORD en bytes [3..6] LE
	{
		if(msgSize >= 7)
		{
			DWORD response = *(DWORD*)(lpMsg+3);

			st->tradeOpen = (response != 0); // 0 = declinado, != 0 = aceptado

			if(response != 0)
			{
				st->tradeAccepted = false;
			}

			return true;
		}
		return false;
	}

	if(head == 0x39) // C1:39 item trade anadido: Slot + ItemInfo[12]
	{
		if(msgSize >= 16)
		{
			int slot = lpMsg[3];

			if(slot >= 0 && slot < TRADE_MAX_ITEMS)
			{
				st->tradeItems[slot].slot = slot;
				st->tradeItems[slot].index = lpMsg[4] | (lpMsg[5] << 8);
				st->tradeItems[slot].level = (lpMsg[6] >> 3) & 0x0F;
				st->tradeItems[slot].dur = lpMsg[7];
			}

			return true;
		}
		return false;
	}

	if(head == 0x3B) // C1:3B dinero trade: DWORD LE en bytes [3..6]
	{
		if(msgSize >= 7)
		{
			st->tradeMoney = *(DWORD*)(lpMsg+3);
			return true;
		}
		return false;
	}

	if(head == 0x3C) // C1:3C boton OK -> trade aceptado por ambas partes
	{
		if(msgSize >= 4)
		{
			st->tradeAccepted = true;
			return true;
		}
		return false;
	}

	if(head == 0x3A || head == 0x3D) // C1:3A resultado / C1:3D cancelar
	{
		if(msgSize >= 4)
		{
			st->tradeOpen = false;
			st->tradeAccepted = false;
			st->tradeMoney = 0;

			for(int n = 0; n < TRADE_MAX_ITEMS; n++)
			{
				st->tradeItems[n].index = 0;
			}

			return true;
		}
		return false;
	}

	// --- PERSONAL SHOP ---
	if(head == 0x3F)
	{
		// C1:3F:02 abrir / C1:3F:03 cerrar / C1:3F:12 leave
		if(sub == 0x02 && msgSize >= 4)
		{
			st->shopOpen = true;
			return true;
		}

		if((sub == 0x03 || sub == 0x12) && msgSize >= 4)
		{
			st->shopOpen = false;
			return true;
		}

		// C2:3F:05 / C2:3F:13 lista de items (evidencia de shop abierto)
		if((sub == 0x05 || sub == 0x13) && msgSize >= 6)
		{
			st->shopOpen = true;
			return true;
		}

		return false;
	}

	// --- CAJA DEL CAOS ---
	if(head == 0x88) // C1:88 rate de la caja del caos (recv)
	{
		if(msgSize >= 4)
		{
			st->chaosBoxSeen = true;
			return true;
		}
		return false;
	}

	return false;
}

#endif // CLIENTSTATECACHE_H
