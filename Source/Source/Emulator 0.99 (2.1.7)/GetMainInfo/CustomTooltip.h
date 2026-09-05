// CustomTooltip.h: interface for the CCustomTooltip class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CUSTOM_TOOLTIP 2000

struct CUSTOM_TOOLTIP_INFO
{
	int Index;
	int ItemIndex;
	int Level;
	int FontColor;
	int FontValue;
	char Text[64];
};

class CCustomTooltip
{
public:
	bool Load(char* path);
	void SetInfo(CUSTOM_TOOLTIP_INFO info);
	CUSTOM_TOOLTIP_INFO m_CustomTooltipInfo[MAX_CUSTOM_TOOLTIP];
};

extern CCustomTooltip gCustomTooltip;
