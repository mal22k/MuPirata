#include <windows.h>
#include <stdio.h>
int main() {
    const char* path = "C:/Users/Administrator/Desktop/ssemu/Package_SSeMU_S6_ENG/Shader/Shader.ini";
    int mg = GetPrivateProfileIntA("Shader","ModernGL",-99,path);
    int ms = GetPrivateProfileIntA("Shader","MSAA",-99,path);
    int en = GetPrivateProfileIntA("Shader","Enabled",-99,path);
    char p[32]={0};
    GetPrivateProfileStringA("Shader","Preset","X",p,sizeof(p),path);
    printf("ModernGL=%d MSAA=%d Enabled=%d Preset=%s\n",mg,ms,en,p);
    return 0;
}
