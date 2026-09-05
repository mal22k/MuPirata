// ============================================================================
// MoneyCache.h
// Parser del dinero del personaje a partir de los packets del servidor.
//
// El struct del personaje del cliente (MAIN_CHARACTER_STRUCT) NO guarda el
// dinero: llega por dos packets, ya descifrados por main.dll antes de llegar
// al ProtocolCore:
//
//   C3:F3:03 (PMSG_CHARACTER_INFO_SEND, al entrar al mundo):
//       layout: header(4) + X/Y/Map/Dir(4) + Experience[8] + NextExperience[8]
//               + LevelUpPoint(2) + Str/Dex/Vit/Ene(8) + Life/MaxLife/Mana/
//               MaxMana/Shield/MaxShield/BP/MaxBP(16) = 50
//       -> Money (DWORD little-endian plano) en bytes [50..53]
//
//   C3:22 result=0xFE (GCMoneySend, al ganar/gastar zen):
//       layout: header(4) + result(1) + ItemInfo[12]
//       -> money big-endian en ItemInfo[0..3] = bytes [4..7]
//
// Este header es COMPARTIDO entre el plugin (LuaPlugin.cpp) y el test harness
// (test/test_lua.cpp) para que el test ejercite EXACTAMENTE el mismo codigo
// que corre dentro del cliente.
// ============================================================================

#ifndef MONEYCACHE_H
#define MONEYCACHE_H

#include <windows.h>

// Parsea el dinero de un packet recibido. Devuelve true si el packet traia
// dinero y lo deja en *outMoney; false en cualquier otro caso.
static inline bool ParseMoneyFromPacket(BYTE head,int sub,BYTE* lpMsg,int msgSize,DWORD* outMoney)
{
	if(lpMsg == 0 || outMoney == 0 || msgSize < 3)
	{
		return false;
	}

	// C3:F3:03 -> Money en bytes [50..53] (DWORD little-endian plano)
	if(head == 0xF3 && sub == 0x03 && msgSize >= 54)
	{
		*outMoney = *(DWORD*)(lpMsg+50);
		return true;
	}

	// C3:22 result=0xFE -> money big-endian en ItemInfo[0..3] (bytes [4..7])
	if(head == 0x22 && sub == 0xFE && msgSize >= 8)
	{
		*outMoney = ((DWORD)lpMsg[4]<<24)|((DWORD)lpMsg[5]<<16)|((DWORD)lpMsg[6]<<8)|(DWORD)lpMsg[7];
		return true;
	}

	return false;
}

#endif // MONEYCACHE_H
