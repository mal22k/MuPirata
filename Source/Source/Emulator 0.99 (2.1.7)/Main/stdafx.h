#pragma once

typedef unsigned __int64 QWORD;

#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT _WIN32_WINNT_WINXP

#ifndef LUA_SCRIPT
#define LUA_SCRIPT 1
#endif

// System Include
#include <windows.h>
#include <iostream>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <winsock2.h>
#include <Mmsystem.h>
#include <gl\GL.h>
#include <shellapi.h>
#if(LUA_SCRIPT == 1)
#include "..\\..\\..\\Util\\lua\\include\\lua.hpp"
#endif

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"Opengl32.lib")
#if(LUA_SCRIPT == 1)
#pragma comment(lib,"..\\..\\..\\Util\\lua\\lua52.lib")
#endif

extern WORD CharacterDeleteMaxLevel;
extern DWORD gLevelExperience[1001];