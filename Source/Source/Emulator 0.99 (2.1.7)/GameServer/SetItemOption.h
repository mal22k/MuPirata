// SetItemOption.h: interface for the CSetItemOption class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Item.h"
#include "User.h"
#include "SetItemType.h"

#define MAX_SET_ITEM_OPTION 100
#define MAX_SET_ITEM_OPTION_TABLE 6
#define MAX_SET_ITEM_FULL_OPTION_TABLE 5

enum eSetItemOption
{
	SET_ITEM_OPTION_ADD_STRENGTH = 0,
	SET_ITEM_OPTION_ADD_DEXTERITY = 1,
	SET_ITEM_OPTION_ADD_ENERGY = 2,
	SET_ITEM_OPTION_ADD_MIN_PHYSI_DAMAGE = 5,
	SET_ITEM_OPTION_ADD_MAX_PHYSI_DAMAGE = 6,
	SET_ITEM_OPTION_MUL_MAGIC_DAMAGE = 7,
	SET_ITEM_OPTION_MUL_ATTACK_SUCCESS_RATE = 9,
	SET_ITEM_OPTION_ADD_DEFENSE = 10,
	SET_ITEM_OPTION_ADD_MAX_LIFE = 11,
	SET_ITEM_OPTION_ADD_MAX_MANA = 12,
	SET_ITEM_OPTION_ADD_MAX_BP = 13,
	SET_ITEM_OPTION_ADD_BP_RECOVERY = 14,
	SET_ITEM_OPTION_ADD_CRITICAL_DAMAGE_RATE = 15,
	SET_ITEM_OPTION_ADD_CRITICAL_DAMAGE = 16,
	SET_ITEM_OPTION_ADD_EXCELLENT_DAMAGE_RATE = 17,
	SET_ITEM_OPTION_ADD_EXCELLENT_DAMAGE = 18,
	SET_ITEM_OPTION_ADD_SKILL_DAMAGE = 19,
	SET_ITEM_OPTION_ADD_DOUBLE_DAMAGE_RATE = 20,
	SET_ITEM_OPTION_ADD_IGNORE_DEFENSE_RATE = 21,
	SET_ITEM_OPTION_MUL_SHIELD_DEFENSE = 22,
};

struct SET_ITEM_OPTION_TABLE
{
	int Index;
	int Value;
};

struct SET_ITEM_OPTION_INFO
{
	int Index;
	char Name[32];
	SET_ITEM_OPTION_TABLE OptionTable[MAX_SET_ITEM_OPTION_TABLE][MAX_SET_ITEM_OPTION_INDEX];
	SET_ITEM_OPTION_TABLE FullOptionTable[MAX_SET_ITEM_FULL_OPTION_TABLE];
};

class CSetItemOption
{
public:
	CSetItemOption();
	virtual ~CSetItemOption();
	void Init();
	void Load(char* path);
	void SetInfo(SET_ITEM_OPTION_INFO info);
	SET_ITEM_OPTION_INFO* GetInfo(int index);
	bool IsSetItem(CItem* lpItem);
	int GetSetItemMaxOptionCount(int index);
	int GetInventorySetItemOptionCount(LPOBJ lpObj,int index);
	void CalcSetItemStat(LPOBJ lpObj);
	void CalcSetItemOption(LPOBJ lpObj,bool flag);
	void InsertOption(LPOBJ lpObj,int index,int value,bool flag);
private:
	SET_ITEM_OPTION_INFO m_SetItemOptionInfo[MAX_SET_ITEM_OPTION];
};

extern CSetItemOption gSetItemOption;
