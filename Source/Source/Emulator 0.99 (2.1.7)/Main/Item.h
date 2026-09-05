#pragma once

void InitItem();
void ItemModelLoad();
void ItemTextureLoad();
void LoadItemModel(int index,char* folder,char* name);
void LoadItemTexture(int index,char* folder);
void GetItemColor(DWORD a,DWORD b,DWORD c,DWORD d,DWORD e);
void GetItemEffect(DWORD a,int b,float* c,float d,int e,int f,int g,int h,int i);
void DrawItemToolTip(DWORD address);
bool MoveItem(DWORD a,DWORD b,int c,int d,int e);
void GetItemToolTip();