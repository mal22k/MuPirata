// ============================================================================
// test_shader.cpp - Harness standalone de la LOGICA del ShaderPlugin (sin GL).
//
// Prueba:
//   1. Lectura de la config (Shader\Shader.ini) con valores custom
//   2. Creacion del INI por defecto cuando no existe
//   3. Estructura de los shaders (existen, #version 330, llaves balanceadas)
//   4. GetPrivateFloat / parsing
// Uso:  build_test.bat && test_shader.exe
// ============================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/PostProcess.h"

static int g_Errors = 0;

#define CHECK(cond,msg) \
	do { if(cond) { printf("[OK] %s\n",msg); } \
	     else { printf("[ERROR] %s\n",msg); g_Errors++; } } while(0)

// ---------------------------------------------------------------------------
// Test 1: config custom
// ---------------------------------------------------------------------------
static void TestCustomConfig()
{
	printf("\n=== Test 1: config custom ===\n");

	CreateDirectoryA("Shader",0);

	WritePrivateProfileStringA("Shader","Enabled","0","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","ToggleKey","50","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","InGameOnly","0","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","TransitionProgress","0.5","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","WarmToneR","2.0","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","Saturation","0.80","Shader\\Shader.ini");
	WritePrivateProfileStringA("Shader","BloomIntensity","0.30","Shader\\Shader.ini");
	WritePrivateProfileStringA("WeatherByMap","Map7","2","Shader\\Shader.ini");

	PostProcess_Init();

	const ShaderConfig& c = ShaderConfig_Get();

	CHECK(c.enabled == false,                "Enabled=0 leido");
	CHECK(c.toggleKey == 50,                 "ToggleKey=50 leido");
	CHECK(c.inGameOnly == false,             "InGameOnly=0 leido");
	CHECK(c.transitionProgress > 0.49f && c.transitionProgress < 0.51f, "TransitionProgress=0.5 leido");
	CHECK(c.warmTone[0] > 1.99f && c.warmTone[0] < 2.01f, "WarmToneR=2.0 leido");
	CHECK(c.weather[7] == 2.0f,              "WeatherByMap Map7=2 leido");
	CHECK(c.weather[0] == 0.0f,              "Map0 por defecto = 0 (sin clima)");
	CHECK(c.saturation > 0.79f && c.saturation < 0.81f, "Saturation=0.80 leido");
	CHECK(c.bloomIntensity > 0.29f && c.bloomIntensity < 0.31f, "BloomIntensity=0.30 leido");
	CHECK(c.contrast == 1.00f,               "Contrast por defecto = 1.00");
	CHECK(c.vignettePower > 0.44f && c.vignettePower < 0.46f, "VignettePower por defecto = 0.45");
}

// ---------------------------------------------------------------------------
// Test 2: valores por defecto
// ---------------------------------------------------------------------------
static void TestDefaults()
{
	printf("\n=== Test 2: defaults (sin ini) ===\n");

	DeleteFileA("Shader\\Shader.ini");

	PostProcess_Init();

	const ShaderConfig& c = ShaderConfig_Get();

	CHECK(c.enabled == true,            "Enabled por defecto = 1");
	CHECK(c.toggleKey == 36,            "ToggleKey por defecto = 36 (VK_HOME)");
	CHECK(c.inGameOnly == true,         "InGameOnly por defecto = 1");
	CHECK(c.transitionProgress == 0.0f, "TransitionProgress por defecto = 0");
	CHECK(c.warmTone[0] > 1.09f && c.warmTone[0] < 1.11f, "WarmToneR por defecto = 1.10");
	CHECK(c.warmTone[1] > 1.02f && c.warmTone[1] < 1.04f, "WarmToneG por defecto = 1.03");

	// El ini se creo
	CHECK(GetFileAttributesA("Shader\\Shader.ini") != INVALID_FILE_ATTRIBUTES, "Shader.ini creado");
}

// ---------------------------------------------------------------------------
// Test 3: estructura de los shaders
// ---------------------------------------------------------------------------
static int CountChar(const char* s,char c)
{
	int n = 0;

	for(; *s; s++)
	{
		if(*s == c) n++;
	}

	return n;
}

static bool HasVersion330(const char* s)
{
	return (strstr(s,"#version 330") != 0);
}

static char* ReadFile(const char* path)
{
	FILE* f = 0;
	fopen_s(&f,path,"rb");

	if(f == 0)
	{
		return 0;
	}

	fseek(f,0,SEEK_END);
	long size = ftell(f);
	fseek(f,0,SEEK_SET);

	char* data = (char*)malloc(size+1);

	if(data != 0)
	{
		fread(data,1,size,f);
		data[size] = 0;
	}

	fclose(f);
	return data;
}

static void TestShaders()
{
	printf("\n=== Test 3: estructura de los shaders ===\n");

	char* vs = ReadFile("../shaders/post.vs");
	char* fs = ReadFile("../shaders/post.fs");

	CHECK(vs != 0, "post.vs existe");
	CHECK(fs != 0, "post.fs existe");

	if(vs != 0 && fs != 0)
	{
		CHECK(HasVersion330(vs),"post.vs tiene #version 330");
		CHECK(HasVersion330(fs),"post.fs tiene #version 330");

		int ob = CountChar(fs,'{'), cb = CountChar(fs,'}');
		int op = CountChar(fs,'('), cp = CountChar(fs,')');

		CHECK(ob == cb && ob > 0, "post.fs: llaves balanceadas");
		CHECK(op == cp && op > 0, "post.fs: parentesis balanceados");

		// Los array constructors deben usar la forma GLSL 1.20+ (vec2[8](...)),
		// NO la forma GLSL 4.0+ (vec2[](...)) que no compila en 3.3
		CHECK(strstr(fs,"vec2[8](") != 0, "post.fs: vec2[8](...) (GLSL 330 OK)");
		CHECK(strstr(fs,"float[16](") != 0,"post.fs: float[16](...) (GLSL 330 OK)");
		CHECK(strstr(fs,"vec2[](") == 0 && strstr(fs,"float[](") == 0, "post.fs: sin array constructors GLSL 4.0");

		// Uniforms que la DLL configura
		CHECK(strstr(fs,"u_Resolution") != 0,      "uniform u_Resolution");
		CHECK(strstr(fs,"u_Time") != 0,            "uniform u_Time");
		CHECK(strstr(fs,"u_PlayerHP") != 0,        "uniform u_PlayerHP");
		CHECK(strstr(fs,"u_WeatherType") != 0,     "uniform u_WeatherType");
		CHECK(strstr(fs,"u_TransitionProgress") != 0, "uniform u_TransitionProgress");
		CHECK(strstr(fs,"screenTexture") != 0,     "sampler screenTexture");

		CHECK(CountChar(fs,'(') == CountChar(fs,')'), "post.fs: parens balanceados (recheck)");

		free(vs);
		free(fs);
	}
}

// ---------------------------------------------------------------------------
int main()
{
	printf("=== test_shader: harness de logica del ShaderPlugin ===\n");

	TestCustomConfig();
	TestDefaults();
	TestShaders();

	printf("\n=== RESULTADO: %s, %d error(es) ===\n",g_Errors == 0 ? "TEST OK" : "TEST CON ERRORES",g_Errors);

	return (g_Errors == 0) ? 0 : 1;
}
