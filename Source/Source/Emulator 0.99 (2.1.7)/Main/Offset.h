#pragma once

#define MAIN_WINDOW				0x05688160
#define MAIN_CONNECTION_STATUS	0x0568C548
#define MAIN_SCREEN_STATE		0x00610660
#define MAIN_CHARACTER_STRUCT	0x07B5ECCC
#define MAIN_VIEWPORT_STRUCT	0x07924F08
#define MAIN_PACKET_SERIAL		0x0568C543
#define MAIN_FONT_SIZE			0x07C09B58
#define MAIN_RESOLUTION			0x05687F70
#define MAIN_RESOLUTION_X		0x006105F8
#define MAIN_RESOLUTION_Y		0x006105FC
#define MAIN_PARTY_MEMBER_COUNT	0x07D3EA48
#define MAIN_CURRENT_MAP		0x006081C0
#define MAIN_HOOK_RECV			0x005FA48C
#define MAIN_HOOK_SEND			0x005FA460
#define MAIN_ACTIVE_SOCKET		0x05688310

#define STRUCT_DECRYPT			((void(__thiscall*)(void*,void*))0x004A7520)((void*)0x05687D00,*(void**)0x07B5ECD4);
#define STRUCT_ENCRYPT			((void(__thiscall*)(void*,void*))0x0040A790)((void*)0x05687D00,*(void**)0x07B5ECD4);

#define ProtocolCore			((BOOL(*)(DWORD,BYTE*,DWORD,DWORD))0x004A3C70)
#define pGetPosFromAngle		((void(__cdecl*)(float*,int*,int*))0x005B0F70)
#define pCursorX				*(int*)0x081B9560
#define pCursorY				*(int*)0x081B955C
#define DrawInterfaceText		((void(*)(int,int,char*))0x005B4210)
#define pDrawText				((char*(__cdecl*)(int,int,char*,int,int,int))0x00500100)
#define pDrawBarForm			((void(__cdecl*)(float,float,float,float,float,int))0x005B2460)
#define pDrawBigText    		((void(*)(float,float,DWORD,float,float))0x005B22B0)
#define pDrawImage              ((void(*)(DWORD,float,float,float,float,float,float,float,float,int,int,GLfloat))0x005B2520)
#define pDrawMessage			((int(__cdecl*)(char*,int))0x00500570)
#define pLoadItemModel			((void(*)(int,char*,char*,int))0x005A2710)
#define pLoadItemTexture		((void(*)(int,char*,int,int,int))0x005A22D0)
#define pViewportAddress		*(DWORD*)(0x07924F00)
#define pPetMixIndex			*(BYTE*)(0x006482FC)
#define pChaosMixIndex			*(DWORD*)(0x081B0F8C)
#define pLoadWaveFile			((void(*)(int,char*,int,int))0x00414230)

#define pRenderPartObjectEffect	((void(*)(DWORD,int,float*,float,int,int,int,int,int))0x005A0490)
#define pTransformPosition      ((int(__thiscall*)(DWORD,DWORD,float*,float*,bool))0x004AB360)
#define pCreateSprite	        ((int(*)(int,float*,float,float*,DWORD,float,int))0x004F63D0)
#define pCreateParticle			((int(__cdecl*)(DWORD,float*,DWORD,float*,DWORD,float,DWORD))0x004F0150)
#define pCreateEffect			((void(__cdecl*)(int,float*,DWORD,float*,int,DWORD,short,BYTE,float,BYTE,float*))0x004D23C0)

#define pCreateMonster			((DWORD(*)(int,int,int,int))0x0041D5D0)
#define pCreateCharacter		((DWORD(*)(int,int,int,int,float))0x004CD360)
#define pSetCharacterScale		((void(*)(DWORD))0x004CD410)

#define pGetTextLine(x)			((char*)(0x007B969FC+(0x12C*x)))
#define pGetItemName(x)			((char*)(*(DWORD*)(0x0773D118)+0x4C*x))

#define pMouseLButton			0x081B95A8
#define pMouseRButton			0x081B9590

#define ITEM_BASE_MODEL			441

#define pSetTextColor			*(DWORD*)0x0060745C
#define pSetBGTextColor			*(DWORD*)0x00607464
#define pLoadImageJPG			((int(*)(char*,int,GLint,GLint,int,int))0x005CBE50)
#define pLoadImageTGA			((int(*)(char*,int,GLint,GLint,int,int))0x005CC2E0)
#define pPlayBuffer				((int(__cdecl*)(int,int,int))0x00414480)
#define pRenderTipText			((void(*)(int,int,char*))0x005002A0)
#define pCheckMouseIn			((BOOL(__cdecl*)(int,int,int,int,int))0x004354C0)
#define pRenderBitmapRotate     ((void(*)(int,float,float,float,float,float))0x005B2630)
#define pSetPlayerStop			((void(*)(DWORD))0x004B0BA0)

#define pglViewport2			((void(*)(int,int,int,int))0x005B16D0)
#define pgluPerspective2		((void(*)(float,float,float,float))0x005B0DC0)
#define pGetOpenGLMatrix		((void(*)(LPVOID))0x05B0D70)
#define pCameraMatrix			(LPVOID*)0x081B9424
#define pEnableDepthTest		((void(*)())0x005B1210)
#define pEnableDepthMask		((void(*)())0x005B1250)
#define pRenderItem3D			((void(*)(float,float,float,float,int,int,int,int,int))0x00577050)
#define pBeginBitmap			((void(*)())0x005B2370)
#define pCameraPosition			(GLfloat*)0x081B95B8
#define pVectorIRotate			((int(*)(int,int,int*,int))0x005B0E80)

#define pFontHDC				*(HDC*)0x05688124
#define pFontNormal				*(HFONT*)0x05688170
#define pFontBold				*(HFONT*)0x05688174
#define pFontBig				*(HFONT*)0x05688178

#define	EnableLightMap			((void(*)())0x005B1650)
#define	EnableAlphaBlend		((void(*)())0x005B1450)
#define	EnableAlphaBlend2		((void(*)())0x005B1550)
#define	EnableAlphaBlendMinus	((void(*)())0x005B14D0)
#define	EnableAlphaTest			((void(*)(bool))0x005B13C0)
#define	DisableAlphaBlend		((void(*)())0x005B1340)

#define GET_ITEM(x,y)			(((x)*32)+(y))
#define GET_ITEM_MODEL(x,y)		((((x)*32)+(y))+ITEM_BASE_MODEL)
#define GET_ITEM_OPT_LEVEL(x)	((x>>3)&15)
#define GET_ITEM_OPT_EXC(x)		((x)-(x&64))
#define GET_MAX_WORD_VALUE(x)	(((x)>65000)?65000:((WORD)(x)))

// pueden servir para algo
#define pSetAction	((void(*)(DWORD,int))0x004A93C0) // objeto y valor de animacion
#define pFindCharacterIndex	((int(*)(int))0x004CBDC0) // index devuelve el i del pj