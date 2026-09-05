// ============================================================================
// MouseBlock.h
// Estado del bloqueo de clicks sobre UI custom (FASE 6).
//
// La DLL v0.3.0 mantiene una lista de rectangulos registrados desde Lua
// (UI.BlockMouse) y, en el hook del call site de MouseClick (0x007D2920), si el
// cursor esta dentro de uno de esos rectangulos devuelve 0 -> el juego NO recibe
// el click (mismo significado que HelperMouseClick del Main_EX603 cuando las
// ventanas 8/9 estan abiertas).
//
// Este header es COMPARTIDO entre el plugin (LuaPlugin.cpp) y el test harness
// (test/test_lua.cpp) para que el test ejercite la MISMA logica de rects que
// corre dentro del cliente. La logica es pura (estado propio, sin lecturas de
// memoria del cliente) -> segura.
// ============================================================================

#ifndef MOUSEBLOCK_H
#define MOUSEBLOCK_H

#include <windows.h>

#define MAX_BLOCKED_RECTS 64

struct BLOCKED_RECT
{
	int x,y,w,h;
};

struct MOUSE_BLOCK_STATE
{
	BLOCKED_RECT rects[MAX_BLOCKED_RECTS];
	int count;
	bool active;    // bloqueo habilitado (UI.SetMouseBlockEnabled)
	bool consumed;  // el ultimo click fue bloqueado (UI.ConsumeClick)

	MOUSE_BLOCK_STATE() : count(0), active(false), consumed(false)
	{
		// C++11 no, toolset v100: inicializar el array manualmente
		for(int n=0;n<MAX_BLOCKED_RECTS;n++)
		{
			rects[n].x = 0;
			rects[n].y = 0;
			rects[n].w = 0;
			rects[n].h = 0;
		}
	}

	void Clear()
	{
		count = 0;
	}

	bool AddRect(int x,int y,int w,int h)
	{
		if(count >= MAX_BLOCKED_RECTS)
		{
			return false;
		}

		rects[count].x = x;
		rects[count].y = y;
		rects[count].w = w;
		rects[count].h = h;
		count++;

		active = true; // registrar un rect activa el bloqueo

		return true;
	}

	bool ShouldBlock(int cx,int cy) const
	{
		if(!active || count == 0)
		{
			return false;
		}

		for(int n=0;n<count;n++)
		{
			if(cx >= rects[n].x && cx <= rects[n].x + rects[n].w
				&& cy >= rects[n].y && cy <= rects[n].y + rects[n].h)
			{
				return true;
			}
		}

		return false;
	}

	bool ConsumeClick()
	{
		bool c = consumed;
		consumed = false;
		return c;
	}
};

// Deberia tragarse este mensaje de raton del WndProc del cliente? (capa 2 del
// bloqueo: el juego NO llega a procesar el click si el cursor esta sobre la UI).
static inline bool ShouldSwallowClickMessage(const MOUSE_BLOCK_STATE& mb,UINT msg,int x,int y)
{
	switch(msg)
	{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			break;
		default:
			return false;
	}

	return mb.ShouldBlock(x,y);
}

// Deberia tragarse este envio de red? (capa 3 del bloqueo: los packets que
// mueven/atacan al personaje cuando el click cae fuera de la UI custom).
//   C1:04 = ataque (click derecho) | C1:05 = mover (click izquierdo en el suelo)
//   C1:06 = seguir/atacar objetivo o cancelar movimiento
// El buffer en este punto es el packet PLANO (antes del cifrado de stream que
// aplica MySend), asi que el opcode vive en byte[2].
static inline bool ShouldBlockPacket(const MOUSE_BLOCK_STATE& mb,int cx,int cy,const BYTE* buff,int size)
{
	if(!mb.ShouldBlock(cx,cy) || buff == 0 || size < 3)
	{
		return false;
	}

	if(buff[0] != 0xC1)
	{
		return false;
	}

	BYTE op = buff[2];

	return (op == 0x04 || op == 0x05 || op == 0x06);
}

#endif // MOUSEBLOCK_H
