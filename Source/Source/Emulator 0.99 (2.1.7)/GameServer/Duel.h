// Duel.h: interface for the CDuel class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"

//**********************************************//
//************ Client -> GameServer ************//
//**********************************************//

struct PMSG_DUEL_START_RECV
{
	PBMSG_HEAD header; // C1:AA
	BYTE index[2];
	char name[10];
};

struct PMSG_DUEL_OK_RECV
{
	PBMSG_HEAD header; // C1:AC
	BYTE flag;
	BYTE index[2];
	char name[10];
};

//**********************************************//
//************ GameServer -> Client ************//
//**********************************************//

struct PMSG_DUEL_START_SEND
{
	PBMSG_HEAD header; // C1:AA
	BYTE result;
	BYTE index[2];
	char name[10];
};

struct PMSG_DUEL_OK_SEND
{
	PBMSG_HEAD header; // C1:AC
	BYTE index[2];
	char name[10];
};

struct PMSG_DUEL_END_SEND
{
	PBMSG_HEAD header; // C1:AB
	BYTE index[2];
	char name[10];
};

struct PMSG_DUEL_SCORE_SEND
{
	PBMSG_HEAD header; // C1:AD
	BYTE index1[2];
	BYTE index2[2];
	BYTE score[2];
};

//**********************************************//
//**********************************************//
//**********************************************//

class CDuel
{
public:
	CDuel();
	virtual ~CDuel();
	bool CheckDuel(LPOBJ lpObj,LPOBJ lpTarget);
	void CheckDuelScore(LPOBJ lpObj,LPOBJ lpTarget);
	void CheckDuelUser();
	void ResetDuel(LPOBJ lpObj);
	void RespawnDuelUser(LPOBJ lpObj);
	void UserDieProc(LPOBJ lpObj,LPOBJ lpTarget);
	void CGDuelStartRecv(PMSG_DUEL_START_RECV* lpMsg,int aIndex);
	void CGDuelOkRecv(PMSG_DUEL_OK_RECV* lpMsg,int aIndex);
	void CGDuelEndRecv(int aIndex);
	void GCDuelStartSend(int aIndex,BYTE flag,int bIndex);
	void GCDuelOkSend(int aIndex,int bIndex);
	void GCDuelEndSend(int aIndex,BYTE result);
	void GCDuelScoreSend(int aIndex,int bIndex);
};

extern CDuel gDuel;
