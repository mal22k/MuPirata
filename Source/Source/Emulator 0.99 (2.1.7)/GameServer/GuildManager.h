#pragma once

#include "User.h"

#define MAX_GUILD_MEMBER 80

struct GUILD_MEMBER_INFO
{
	void Clear()
	{
		memset(this->Name,0,sizeof(this->Name));

		this->Index = -1;

		this->Server = -1;
	}

	bool IsEmpty()
	{
		return ((this->Name[0]==0)?1:0);
	}

	bool IsValid()
	{
		return ((this->IsEmpty()==0)?gObjIsNameValid(this->Index,this->Name):0);
	}

	char Name[11];
	WORD Index;
	int Server;
};

struct GUILD_INFO
{
	GUILD_INFO()
	{
		this->Index = 0;

		this->Score = 0;

		memset(this->Name,0,sizeof(this->Name));

		memset(this->Master,0,sizeof(this->Master));

		memset(this->Notice,0,sizeof(this->Notice));

		memset(this->Mark,0,sizeof(this->Mark));

		for(int n=0;n < MAX_GUILD_MEMBER;n++){this->Member[n].Clear();}

		this->WarEnemy = 0;
		
		this->WarDeclare = 0;

		this->WarState = 0;

		this->WarType = 0;
		
		this->WarTeam = 0;

		this->WarScore = 0;
	}

	int GetMemberCount()
	{
		int count = 0;

		for(int n=0;n < MAX_GUILD_MEMBER;n++)
		{
			if(this->Member[n].IsEmpty() == 0)
			{
				count++;
			}
		}

		return count;
	}

	DWORD Index;
	DWORD Score;
	char Name[9];
	char Master[11];
	char Notice[60];
	BYTE Mark[32];
	GUILD_MEMBER_INFO Member[MAX_GUILD_MEMBER];
	GUILD_INFO* WarEnemy;
	BYTE WarDeclare;
	BYTE WarState;
	BYTE WarType;
	BYTE WarTeam;
	BYTE WarScore;
};

typedef std::unordered_map<int,GUILD_INFO*> GuildList;

class CGuildManager
{
public:
	CGuildManager();
	virtual ~CGuildManager();
	void AddGuild(int index,char* name,BYTE* mark,char* master,char* notice,int score);
	void DelGuild(int index);
	void DelGuild(char* name);
	GUILD_INFO* GetGuild(int index);
	GUILD_INFO* GetGuild(char* name);
	GUILD_MEMBER_INFO* GetMemberInfo(char* name,char* member);
	int AddMember(char* name,char* member,int index,int server);
	void DelMember(char* name,char* member);
	void ConnectUser(char* name,char* member,int index,int server);
	void SetServer(char* name,char* member,int server);
private:
	GuildList m_GuildList;
};

extern CGuildManager gGuildManager;