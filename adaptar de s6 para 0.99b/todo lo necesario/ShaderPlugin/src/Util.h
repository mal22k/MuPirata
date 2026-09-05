#pragma once

#include <windows.h>

// Utilidades de parcheo de memoria (mismas que LuaPlugin/src/Util.h)

void SetByte(DWORD offset, BYTE value);
void SetWord(DWORD offset, WORD value);
void SetDword(DWORD offset, DWORD value);
void SetCompleteHook(BYTE head, DWORD offset, ...);   // head 0xE8=call, 0xE9=jmp, 0xFF=solo rel32
void MemoryCpy(DWORD offset, void* value, DWORD size);
void MemorySet(DWORD offset, DWORD value, DWORD size);
