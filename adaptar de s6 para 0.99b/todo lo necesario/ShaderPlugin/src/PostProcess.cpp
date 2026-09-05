#include "PostProcess.h"
#include "GLFuncs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SHADER_DIR   "Shader"
#define CONFIG_PATH  "Shader\\Shader.ini"
#define LOG_PATH     "Shader\\shader_plugin.log"
#define VS_PATH      "Shader\\post.vs"
#define FS_PATH      "Shader\\post.fs"
#define MAX_MAP_OVERRIDES 256   // [MapSettings]: hasta mapa 255

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------
static ShaderConfig g_Config;
static bool g_GLReady = false;          // funciones GL + shaders cargados
static bool g_ShaderLoaded = false;     // programa compilado
static bool g_Enabled = false;          // toggle en runtime
static bool g_GLInitFailed = false;     // fallo permanente (GLSL<3.3, shaders rotos)
static bool g_DisabledByFault = false;  // excepcion GL: efecto fuera hasta reiniciar

static GLuint g_Program = 0;
static GLuint g_QuadVBO = 0;
static GLuint g_QuadVAO = 0;
static GLuint g_ScreenTex = 0;
static int    g_TexW = 0;
static int    g_TexH = 0;

static int g_LocResolution = -1;
static int g_LocTime = -1;
static int g_LocPlayerHP = -1;
static int g_LocWeather = -1;
static int g_LocTransition = -1;
static int g_LocWarmTone = -1;
static int g_LocScreenTex = -1;
static int g_LocSaturation = -1;
static int g_LocContrast = -1;
static int g_LocBloomThreshold = -1;
static int g_LocBloomIntensity = -1;
static int g_LocVignettePower = -1;
static int g_LocGodRayIntensity = -1;
static int g_LocGodRaySamples = -1;
static int g_LocGodRayDecay = -1;
static int g_LocAnamorphicThreshold = -1;
static int g_LocAnamorphicIntensity = -1;
static int g_LocAnamorphicSpread = -1;
static int g_LocSharpenIntensity = -1;
static int g_LocFilmGrain = -1;
static int g_LocChromaticAberration = -1;
static int g_LocToneMap = -1;
static int g_LocExposure = -1;
static int g_LocBloomBlurRadius = -1;
static int g_LocAnamorphicTint = -1;
static int g_LocTransitionStyle = -1;

static bool g_KeyWasDown = false;

// Presets visuales (Cine/Limpio/Retro): se eligen SOLO desde Shader.ini
// ([Shader] Preset=...). Se ELIMINARON las teclas F1/F2/F3 que los activaban
// en vivo: en MU Online F1-F12 son atajos de skills/pociones y pisaban el
// look sin avisar. Default del INI: Preset=Retro (curva clasica, sin ACES).

// ---------------------------------------------------------------------------
// Log
// ---------------------------------------------------------------------------
void ShaderLog(const char* fmt, ...)
{
	char buff[512] = {0};

	va_list arg;
	va_start(arg,fmt);
	vsprintf_s(buff,sizeof(buff),fmt,arg);
	va_end(arg);

	FILE* file = 0;

	fopen_s(&file,LOG_PATH,"a");

	if(file != 0)
	{
		fprintf(file,"%s\n",buff);
		fclose(file);
	}
}

// ---------------------------------------------------------------------------
// Config visual POR MAPA (portado 1:1 del cliente original Main5.2, funcion
// CShaderGL::GetMapShaderSettings). Cada mapa tiene su propia paleta de tono,
// saturacion, contraste, bloom, vineta y clima. Los IDs son los del enum
// WD_* de MapManager.h del cliente original (coinciden con los del servidor).
// ---------------------------------------------------------------------------
struct MapShaderSettings
{
	float warmTone[3];
	float saturation;
	float contrast;
	float bloomThreshold;
	float bloomIntensity;
	float vignettePower;
	float weatherType;   // 0 = ninguno, 2 = ondas de calor
};

// Overrides de paleta por mapa desde Shader.ini: [MapSettings] MapN=valores.
// Si un mapa tiene override, se usa ESE en vez de la tabla interna (permite
// ajustar el look de cada mapa SIN recompilar la DLL).
static MapShaderSettings g_MapOverride[MAX_MAP_OVERRIDES];
static bool             g_MapOverrideSet[MAX_MAP_OVERRIDES];



static MapShaderSettings GetMapShaderSettings(int mapId)
{
	MapShaderSettings s;

	switch (mapId)
	{
	case 0:                      // Lorencia - Tono calido
		s.warmTone[0]=1.10f; s.warmTone[1]=1.03f; s.warmTone[2]=0.92f;
		s.saturation=1.20f; s.contrast=1.08f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.30f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 1: case 4:              // Dungeon / Lost Tower - Oscuro
		s.warmTone[0]=0.85f; s.warmTone[1]=0.80f; s.warmTone[2]=0.75f;
		s.saturation=0.90f; s.contrast=1.05f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.15f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 2:                      // Devias - Tono frio
		s.warmTone[0]=0.92f; s.warmTone[1]=0.95f; s.warmTone[2]=1.08f;
		s.saturation=1.10f; s.contrast=1.05f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.25f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 3:                      // Noria - Tono verde
		s.warmTone[0]=0.95f; s.warmTone[1]=1.05f; s.warmTone[2]=0.90f;
		s.saturation=1.20f; s.contrast=1.06f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.25f; s.vignettePower=0.45f; s.weatherType=0.0f;
		break;
	case 5:                      // Unknown - Desierto
		s.warmTone[0]=1.10f; s.warmTone[1]=1.05f; s.warmTone[2]=0.85f;
		s.saturation=1.15f; s.contrast=1.04f; s.bloomThreshold=0.72f;
		s.bloomIntensity=0.30f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 6:                      // Stadium - Neutro
		s.warmTone[0]=1.00f; s.warmTone[1]=1.00f; s.warmTone[2]=1.00f;
		s.saturation=1.00f; s.contrast=1.00f; s.bloomThreshold=0.80f;
		s.bloomIntensity=0.10f; s.vignettePower=0.35f; s.weatherType=0.0f;
		break;
	case 7: case 67:             // Atlans / Doppelganger 3 - Oceano Luminoso
		s.warmTone[0]=0.90f; s.warmTone[1]=0.95f; s.warmTone[2]=1.15f;
		s.saturation=1.15f; s.contrast=1.02f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.25f; s.vignettePower=0.35f; s.weatherType=0.0f;
		break;
	case 8:                      // Tarkan - Ondas de calor
		s.warmTone[0]=1.15f; s.warmTone[1]=0.88f; s.warmTone[2]=0.78f;
		s.saturation=1.25f; s.contrast=1.10f; s.bloomThreshold=0.68f;
		s.bloomIntensity=0.35f; s.vignettePower=0.50f; s.weatherType=2.0f;
		break;
	case 9:                      // Devil Square - Infernal
		s.warmTone[0]=1.05f; s.warmTone[1]=0.85f; s.warmTone[2]=0.80f;
		s.saturation=1.15f; s.contrast=1.08f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.30f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 10:                     // Heaven/Icarus - Cielo
		s.warmTone[0]=0.95f; s.warmTone[1]=0.98f; s.warmTone[2]=1.10f;
		s.saturation=1.15f; s.contrast=1.05f; s.bloomThreshold=0.75f;
		s.bloomIntensity=0.30f; s.vignettePower=0.35f; s.weatherType=0.0f;
		break;
	case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 52:
		                 // Blood Castle 1-7 (+master) - Sangriento
		s.warmTone[0]=1.12f; s.warmTone[1]=0.82f; s.warmTone[2]=0.78f;
		s.saturation=1.20f; s.contrast=1.10f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.35f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 18: case 19: case 20: case 21: case 53:
		                 // Chaos Castle 1-4 (+master) - Purpura
		s.warmTone[0]=1.05f; s.warmTone[1]=0.85f; s.warmTone[2]=0.95f;
		s.saturation=1.20f; s.contrast=1.08f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.30f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 24: case 25: case 26: case 27: case 28: case 29: case 36:
		                 // Kalima/Hellas 1-7 - Oscuro
		s.warmTone[0]=0.78f; s.warmTone[1]=0.75f; s.warmTone[2]=0.85f;
		s.saturation=0.85f; s.contrast=1.06f; s.bloomThreshold=0.55f;
		s.bloomIntensity=0.20f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 30:                     // Battle Castle - Severo
		s.warmTone[0]=0.95f; s.warmTone[1]=0.88f; s.warmTone[2]=0.82f;
		s.saturation=1.10f; s.contrast=1.06f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.25f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 31:                     // Hunting Ground - Diurno
		s.warmTone[0]=1.05f; s.warmTone[1]=1.02f; s.warmTone[2]=0.95f;
		s.saturation=1.10f; s.contrast=1.04f; s.bloomThreshold=0.72f;
		s.bloomIntensity=0.25f; s.vignettePower=0.40f; s.weatherType=0.0f;
		break;
	case 33: case 134:           // Aida / Ashen Aida - Bosque oscuro
		s.warmTone[0]=0.80f; s.warmTone[1]=0.90f; s.warmTone[2]=0.75f;
		s.saturation=0.95f; s.contrast=1.05f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.20f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 34: case 35:            // Crywolf - Hielo
		s.warmTone[0]=0.88f; s.warmTone[1]=0.92f; s.warmTone[2]=1.08f;
		s.saturation=1.00f; s.contrast=1.05f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 37: case 38:            // Kanturu 1/2 - Ruinas
		s.warmTone[0]=0.90f; s.warmTone[1]=0.88f; s.warmTone[2]=0.82f;
		s.saturation=0.95f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 39:                     // Kanturu 3rd - Batalla final
		s.warmTone[0]=1.10f; s.warmTone[1]=0.85f; s.warmTone[2]=0.90f;
		s.saturation=1.15f; s.contrast=1.10f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.35f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 137:                    // Kanturu Underground
		s.warmTone[0]=0.82f; s.warmTone[1]=0.78f; s.warmTone[2]=0.88f;
		s.saturation=0.90f; s.contrast=1.05f; s.bloomThreshold=0.55f;
		s.bloomIntensity=0.15f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 41: case 42:            // ChangeUp 3rd - Infernal
		s.warmTone[0]=1.18f; s.warmTone[1]=0.82f; s.warmTone[2]=0.72f;
		s.saturation=1.20f; s.contrast=1.12f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.40f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 45: case 46: case 47: case 48: case 49: case 50:
		                 // Cursed Temple - Maldito
		s.warmTone[0]=0.85f; s.warmTone[1]=0.95f; s.warmTone[2]=0.80f;
		s.saturation=1.05f; s.contrast=1.06f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.25f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 51:                     // Elbeland - Calido
		s.warmTone[0]=1.05f; s.warmTone[1]=1.02f; s.warmTone[2]=0.95f;
		s.saturation=1.15f; s.contrast=1.05f; s.bloomThreshold=0.72f;
		s.bloomIntensity=0.25f; s.vignettePower=0.40f; s.weatherType=0.0f;
		break;
	case 56:                     // Swamp of Quiet - Pantano
		s.warmTone[0]=0.82f; s.warmTone[1]=0.88f; s.warmTone[2]=0.78f;
		s.saturation=1.00f; s.contrast=1.05f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.20f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 57: case 58:            // Ice City - Congelado
		s.warmTone[0]=0.85f; s.warmTone[1]=0.88f; s.warmTone[2]=1.08f;
		s.saturation=1.00f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 62:                     // Santa Town - Festivo
		s.warmTone[0]=1.08f; s.warmTone[1]=0.95f; s.warmTone[2]=0.88f;
		s.saturation=1.20f; s.contrast=1.08f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.30f; s.vignettePower=0.45f; s.weatherType=0.0f;
		break;
	case 63:                     // PK Field - Sangriento
		s.warmTone[0]=1.10f; s.warmTone[1]=0.85f; s.warmTone[2]=0.80f;
		s.saturation=1.20f; s.contrast=1.10f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.35f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 64:                     // Duel Arena - Neutro
		s.warmTone[0]=1.00f; s.warmTone[1]=1.00f; s.warmTone[2]=1.00f;
		s.saturation=1.00f; s.contrast=1.02f; s.bloomThreshold=0.75f;
		s.bloomIntensity=0.15f; s.vignettePower=0.40f; s.weatherType=0.0f;
		break;
	case 65: case 66: case 68:   // Doppelganger 1/2/4
		s.warmTone[0]=1.00f; s.warmTone[1]=1.00f; s.warmTone[2]=1.00f;
		s.saturation=1.10f; s.contrast=1.05f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.25f; s.vignettePower=0.45f; s.weatherType=0.0f;
		break;
	case 69: case 70: case 71: case 72:
		                 // Empire Guardian
		s.warmTone[0]=0.92f; s.warmTone[1]=0.90f; s.warmTone[2]=0.95f;
		s.saturation=1.05f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 79:                     // United Marketplace
		s.warmTone[0]=1.05f; s.warmTone[1]=1.02f; s.warmTone[2]=0.95f;
		s.saturation=1.10f; s.contrast=1.04f; s.bloomThreshold=0.72f;
		s.bloomIntensity=0.25f; s.vignettePower=0.40f; s.weatherType=0.0f;
		break;
	case 80: case 81:            // Karutan 1/2 - Ondas de calor
		s.warmTone[0]=1.08f; s.warmTone[1]=1.02f; s.warmTone[2]=0.85f;
		s.saturation=1.15f; s.contrast=1.06f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.30f; s.vignettePower=0.50f; s.weatherType=2.0f;
		break;
	case 95: case 96: case 110: case 111:
		                 // Debenter/Nars - Tono frio
		s.warmTone[0]=0.88f; s.warmTone[1]=0.92f; s.warmTone[2]=1.05f;
		s.saturation=1.05f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 100: case 101:          // Uruk Mountain
		s.warmTone[0]=1.05f; s.warmTone[1]=0.95f; s.warmTone[2]=0.85f;
		s.saturation=1.10f; s.contrast=1.05f; s.bloomThreshold=0.68f;
		s.bloomIntensity=0.25f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 112: case 113:          // Ferea/Nixies Lake - Agua
		s.warmTone[0]=0.88f; s.warmTone[1]=0.95f; s.warmTone[2]=1.08f;
		s.saturation=1.10f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 122:                    // Swamp of Darkness
		s.warmTone[0]=0.78f; s.warmTone[1]=0.85f; s.warmTone[2]=0.75f;
		s.saturation=0.90f; s.contrast=1.06f; s.bloomThreshold=0.55f;
		s.bloomIntensity=0.20f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 131:                    // Scorched Canyon - Ondas de calor
		s.warmTone[0]=1.12f; s.warmTone[1]=0.88f; s.warmTone[2]=0.78f;
		s.saturation=1.20f; s.contrast=1.10f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.35f; s.vignettePower=0.50f; s.weatherType=2.0f;
		break;
	case 132:                    // Scarlet Icarus
		s.warmTone[0]=1.15f; s.warmTone[1]=0.85f; s.warmTone[2]=0.85f;
		s.saturation=1.25f; s.contrast=1.12f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.40f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 133:                    // Temple of Arnil
		s.warmTone[0]=0.95f; s.warmTone[1]=0.90f; s.warmTone[2]=0.85f;
		s.saturation=1.05f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	case 135:                    // Old Kethotum
		s.warmTone[0]=0.88f; s.warmTone[1]=0.85f; s.warmTone[2]=0.80f;
		s.saturation=0.95f; s.contrast=1.05f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.20f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 136:                    // Blaze Kethotum
		s.warmTone[0]=1.12f; s.warmTone[1]=0.85f; s.warmTone[2]=0.78f;
		s.saturation=1.20f; s.contrast=1.10f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.35f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 54: case 55: case 73: case 74: case 77: case 78:
	case 91: case 92: case 93: case 94:
		                 // Login/Character scenes - Neutro
		s.warmTone[0]=1.00f; s.warmTone[1]=1.00f; s.warmTone[2]=1.00f;
		s.saturation=1.00f; s.contrast=1.00f; s.bloomThreshold=0.80f;
		s.bloomIntensity=0.10f; s.vignettePower=0.30f; s.weatherType=0.0f;
		break;
	case 116: case 117: case 118: case 119: case 120:
		                 // Deep Dungeon
		s.warmTone[0]=0.80f; s.warmTone[1]=0.78f; s.warmTone[2]=0.85f;
		s.saturation=0.90f; s.contrast=1.05f; s.bloomThreshold=0.55f;
		s.bloomIntensity=0.15f; s.vignettePower=0.60f; s.weatherType=0.0f;
		break;
	case 123: case 124: case 125: case 126: case 127:
		                 // Kubera Mine
		s.warmTone[0]=0.90f; s.warmTone[1]=0.85f; s.warmTone[2]=0.78f;
		s.saturation=1.00f; s.contrast=1.04f; s.bloomThreshold=0.65f;
		s.bloomIntensity=0.20f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	case 128: case 129: case 130:
		                 // Atlans Abyss
		s.warmTone[0]=0.78f; s.warmTone[1]=0.85f; s.warmTone[2]=1.15f;
		s.saturation=1.10f; s.contrast=1.04f; s.bloomThreshold=0.60f;
		s.bloomIntensity=0.20f; s.vignettePower=0.55f; s.weatherType=0.0f;
		break;
	default:                     // Default - Calido
		s.warmTone[0]=1.10f; s.warmTone[1]=1.03f; s.warmTone[2]=0.92f;
		s.saturation=1.20f; s.contrast=1.08f; s.bloomThreshold=0.70f;
		s.bloomIntensity=0.30f; s.vignettePower=0.50f; s.weatherType=0.0f;
		break;
	}

	return s;
}

// ---------------------------------------------------------------------------
// Config (GetPrivateProfile* de kernel32: lee/crea el INI junto al cliente)
// ---------------------------------------------------------------------------
static float GetPrivateFloat(const char* section,const char* key,float def)
{
	char buf[32] = {0};

	GetPrivateProfileStringA(section,key,"",buf,sizeof(buf),CONFIG_PATH);

	if(buf[0] == 0)
	{
		return def;
	}

	return (float)atof(buf);
}

static void ReadConfig()
{
	g_Config.enabled            = (GetPrivateProfileIntA("Shader","Enabled",1,CONFIG_PATH) != 0);
	g_Config.toggleKey          = GetPrivateProfileIntA("Shader","ToggleKey",36,CONFIG_PATH);
	g_Config.inGameOnly         = (GetPrivateProfileIntA("Shader","InGameOnly",1,CONFIG_PATH) != 0);
	g_Config.useMapSettings     = (GetPrivateProfileIntA("Shader","UseMapSettings",1,CONFIG_PATH) != 0);
	g_Config.autoTransition     = (GetPrivateProfileIntA("Shader","AutoTransition",1,CONFIG_PATH) != 0);
	g_Config.transitionProgress = GetPrivateFloat("Shader","TransitionProgress",0.0f);
	g_Config.warmTone[0]        = GetPrivateFloat("Shader","WarmToneR",1.10f);
	g_Config.warmTone[1]        = GetPrivateFloat("Shader","WarmToneG",1.03f);
	g_Config.warmTone[2]        = GetPrivateFloat("Shader","WarmToneB",0.92f);
	g_Config.saturation         = GetPrivateFloat("Shader","Saturation",1.02f);
	g_Config.contrast           = GetPrivateFloat("Shader","Contrast",1.00f);
	g_Config.bloomThreshold     = GetPrivateFloat("Shader","BloomThreshold",0.60f);
	g_Config.bloomIntensity     = GetPrivateFloat("Shader","BloomIntensity",0.15f);
	g_Config.vignettePower      = GetPrivateFloat("Shader","VignettePower",0.45f);

	// Efectos (uniforms de post.fs, antes #defines): configurables desde el INI
	g_Config.godRayIntensity     = GetPrivateFloat("Shader","GodRayIntensity",0.15f);
	g_Config.godRaySamples       = GetPrivateFloat("Shader","GodRaySamples",40.0f);
	g_Config.godRayDecay         = GetPrivateFloat("Shader","GodRayDecay",0.96f);
	g_Config.anamorphicThreshold = GetPrivateFloat("Shader","AnamorphicThreshold",0.85f);
	g_Config.anamorphicIntensity = GetPrivateFloat("Shader","AnamorphicIntensity",0.20f);
	g_Config.anamorphicSpread    = GetPrivateFloat("Shader","AnamorphicSpread",48.0f);
	g_Config.sharpenIntensity    = GetPrivateFloat("Shader","SharpenIntensity",0.25f);
	g_Config.filmGrain           = GetPrivateFloat("Shader","FilmGrain",0.025f);
	g_Config.chromaticAberration = GetPrivateFloat("Shader","ChromaticAberration",0.006f);

	// Tono cinematico, bloom/flare finos y transiciones
	g_Config.toneMap            = (GetPrivateProfileIntA("Shader","ToneMap",1,CONFIG_PATH) != 0);
	g_Config.exposure           = GetPrivateFloat("Shader","Exposure",1.00f);
	g_Config.bloomBlurRadius    = GetPrivateFloat("Shader","BloomBlurRadius",2.0f);
	g_Config.anamorphicTint[0]  = GetPrivateFloat("Shader","AnamorphicTintR",0.60f);
	g_Config.anamorphicTint[1]  = GetPrivateFloat("Shader","AnamorphicTintG",0.85f);
	g_Config.anamorphicTint[2]  = GetPrivateFloat("Shader","AnamorphicTintB",1.00f);
	g_Config.transitionStyle    = GetPrivateProfileIntA("Shader","TransitionStyle",0,CONFIG_PATH);
	g_Config.transitionDuration = GetPrivateFloat("Shader","TransitionDuration",1.2f);

	if(g_Config.transitionStyle < 0)      g_Config.transitionStyle = 0;
	if(g_Config.transitionStyle > 3)      g_Config.transitionStyle = 3;
	if(g_Config.transitionDuration < 0.3f) g_Config.transitionDuration = 0.3f;
	if(g_Config.transitionDuration > 5.0f) g_Config.transitionDuration = 5.0f;
	if(g_Config.exposure < 0.1f) g_Config.exposure = 0.1f;
	if(g_Config.exposure > 3.0f) g_Config.exposure = 3.0f;

	// --- Preset visual: si [Shader] Preset=Nombre y existe la seccion
	// [Preset<Nombre>], los valores de efectos/tono de ESA seccion reemplazan
	// a los de [Shader]. Asi una linea cambia todo el look (sin teclas en
	// vivo: solo INI). El sistema por mapa (paletas) no se toca.
	char presetBuf[32] = {0};
	GetPrivateProfileStringA("Shader","Preset","Retro",presetBuf,sizeof(presetBuf),CONFIG_PATH);
	sprintf_s(g_Config.preset,sizeof(g_Config.preset),"%s",presetBuf);

	if(g_Config.preset[0] != 0 && _stricmp(g_Config.preset,"Off") != 0)
	{
		char section[48];
		sprintf_s(section,sizeof(section),"Preset%s",g_Config.preset);

		// Los defaults son los valores de [Shader] recien leidos: un preset
		// no esta obligado a definir todas las claves.
		g_Config.toneMap             = (GetPrivateProfileIntA(section,"ToneMap",g_Config.toneMap?1:0,CONFIG_PATH) != 0);
		g_Config.exposure            = GetPrivateFloat(section,"Exposure",g_Config.exposure);
		g_Config.godRayIntensity     = GetPrivateFloat(section,"GodRayIntensity",g_Config.godRayIntensity);
		g_Config.godRaySamples       = GetPrivateFloat(section,"GodRaySamples",g_Config.godRaySamples);
		g_Config.godRayDecay         = GetPrivateFloat(section,"GodRayDecay",g_Config.godRayDecay);
		g_Config.anamorphicThreshold = GetPrivateFloat(section,"AnamorphicThreshold",g_Config.anamorphicThreshold);
		g_Config.anamorphicIntensity = GetPrivateFloat(section,"AnamorphicIntensity",g_Config.anamorphicIntensity);
		g_Config.anamorphicSpread    = GetPrivateFloat(section,"AnamorphicSpread",g_Config.anamorphicSpread);
		g_Config.sharpenIntensity    = GetPrivateFloat(section,"SharpenIntensity",g_Config.sharpenIntensity);
		g_Config.filmGrain           = GetPrivateFloat(section,"FilmGrain",g_Config.filmGrain);
		g_Config.chromaticAberration = GetPrivateFloat(section,"ChromaticAberration",g_Config.chromaticAberration);
		g_Config.bloomBlurRadius     = GetPrivateFloat(section,"BloomBlurRadius",g_Config.bloomBlurRadius);

		ShaderLog("[Shader] Preset '%s' aplicado.",g_Config.preset);
	}

	for(int n=0;n<32;n++)
	{
		char key[16];
		sprintf_s(key,sizeof(key),"Map%d",n);
		g_Config.weather[n] = (float)GetPrivateProfileIntA("WeatherByMap",key,0,CONFIG_PATH);
	}

	// --- Overrides de paleta por mapa: [MapSettings] MapN = 9 valores ---
	// Formato: warmR,warmG,warmB,saturation,contrast,bloomThreshold,
	//          bloomIntensity,vignettePower,weatherType
	// Ejemplo: Map8 = 1.15,0.88,0.78,1.25,1.10,0.68,0.35,0.50,2.0   (Tarkan)
	for(int n=0;n<MAX_MAP_OVERRIDES;n++)
	{
		g_MapOverrideSet[n] = false;

		char key[16];
		sprintf_s(key,sizeof(key),"Map%d",n);

		char buf[128] = {0};
		GetPrivateProfileStringA("MapSettings",key,"",buf,sizeof(buf),CONFIG_PATH);

		if(buf[0] == 0)
		{
			continue;
		}

		float v[9] = {0};
		int parsed = sscanf_s(buf,"%f,%f,%f,%f,%f,%f,%f,%f,%f",
			&v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8]);

		if(parsed == 9)
		{
			g_MapOverride[n].warmTone[0] = v[0];
			g_MapOverride[n].warmTone[1] = v[1];
			g_MapOverride[n].warmTone[2] = v[2];
			g_MapOverride[n].saturation     = v[3];
			g_MapOverride[n].contrast       = v[4];
			g_MapOverride[n].bloomThreshold = v[5];
			g_MapOverride[n].bloomIntensity = v[6];
			g_MapOverride[n].vignettePower  = v[7];
			g_MapOverride[n].weatherType    = v[8];
			g_MapOverrideSet[n] = true;

			ShaderLog("[Shader] [MapSettings] Mapa %d: override cargado.",n);
		}
		else
		{
			ShaderLog("[Shader] [MapSettings] Mapa %d: formato invalido (%s)",n,buf);
		}
	}
}

void PostProcess_Init()
{
	CreateDirectoryA(SHADER_DIR,0);

	// Crear Shader.ini con valores por defecto si no existe
	if(GetFileAttributesA(CONFIG_PATH) == INVALID_FILE_ATTRIBUTES)
	{
		WritePrivateProfileStringA("Shader","Enabled","1",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","ToggleKey","36",CONFIG_PATH);      // VK_HOME
		WritePrivateProfileStringA("Shader","InGameOnly","1",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","UseMapSettings","1",CONFIG_PATH);  // paleta visual por mapa
		WritePrivateProfileStringA("Shader","AutoTransition","1",CONFIG_PATH);  // pixelate al cambiar de mapa
		WritePrivateProfileStringA("Shader","TransitionProgress","0.0",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","WarmToneR","1.10",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","WarmToneG","1.03",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","WarmToneB","0.92",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","Saturation","1.02",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","Contrast","1.00",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","BloomThreshold","0.60",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","BloomIntensity","0.15",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","VignettePower","0.45",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","GodRayIntensity","0.15",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","GodRaySamples","40",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","GodRayDecay","0.96",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicThreshold","0.85",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicIntensity","0.20",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicSpread","48.0",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","SharpenIntensity","0.25",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","FilmGrain","0.025",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","ChromaticAberration","0.006",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","ToneMap","1",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","Exposure","1.00",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","BloomBlurRadius","2.0",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicTintR","0.60",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicTintG","0.85",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","AnamorphicTintB","1.00",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","TransitionStyle","0",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","TransitionDuration","1.2",CONFIG_PATH);
		WritePrivateProfileStringA("Shader","Preset","Retro",CONFIG_PATH);

		// Presets de ejemplo (se eligen con Preset=Cine/Limpio/Retro en [Shader];
		// ya NO hay teclas F1/F2/F3 para no pisar las skills del juego)
		WritePrivateProfileStringA("PresetCine","Exposure","1.00",CONFIG_PATH);
		WritePrivateProfileStringA("PresetCine","GodRayIntensity","0.18",CONFIG_PATH);
		WritePrivateProfileStringA("PresetCine","AnamorphicIntensity","0.30",CONFIG_PATH);
		WritePrivateProfileStringA("PresetCine","SharpenIntensity","0.30",CONFIG_PATH);
		WritePrivateProfileStringA("PresetCine","FilmGrain","0.040",CONFIG_PATH);
		WritePrivateProfileStringA("PresetCine","ChromaticAberration","0.010",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","Exposure","1.00",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","GodRayIntensity","0.12",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","AnamorphicIntensity","0.15",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","SharpenIntensity","0.35",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","FilmGrain","0.0",CONFIG_PATH);
		WritePrivateProfileStringA("PresetLimpio","ChromaticAberration","0.0",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","Exposure","0.95",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","ToneMap","0",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","GodRayIntensity","0.10",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","AnamorphicIntensity","0.10",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","SharpenIntensity","0.20",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","FilmGrain","0.060",CONFIG_PATH);
		WritePrivateProfileStringA("PresetRetro","ChromaticAberration","0.014",CONFIG_PATH);

		WritePrivateProfileStringA("WeatherByMap","Map0","0",CONFIG_PATH);
		WritePrivateProfileStringA("WeatherByMap","Map12","2",CONFIG_PATH);     // Tarkan
		WritePrivateProfileStringA("WeatherByMap","Map13","2",CONFIG_PATH);     // Karutan

		ShaderLog("[Shader] Config creada: %s",CONFIG_PATH);
	}

	ReadConfig();

	g_Enabled = g_Config.enabled;

	ShaderLog("[Shader] Config: enabled=%d toggleKey=0x%02X inGameOnly=%d transition=%.2f",
		g_Config.enabled,g_Config.toggleKey,g_Config.inGameOnly,g_Config.transitionProgress);
}

// Re-lee Shader.ini (se llama al re-activar con el toggle: permite cambiar
// parametros en caliente sin reiniciar el cliente)
void PostProcess_ReloadConfig()
{
	ReadConfig();

	ShaderLog("[Shader] Config recargada: preset=%s tonemap=%d transicion=estilo%d/%.1fs warm=(%.2f,%.2f,%.2f) sat=%.2f cont=%.2f bloom=%.2f/%.2f/%.1f vignette=%.2f",
		g_Config.preset[0] ? g_Config.preset : "Off",g_Config.toneMap?1:0,
		g_Config.transitionStyle,g_Config.transitionDuration,
		g_Config.warmTone[0],g_Config.warmTone[1],g_Config.warmTone[2],
		g_Config.saturation,g_Config.contrast,g_Config.bloomThreshold,g_Config.bloomIntensity,
		g_Config.bloomBlurRadius,g_Config.vignettePower);
}

const ShaderConfig& ShaderConfig_Get()
{
	return g_Config;
}

bool PostProcess_IsEnabled()
{
	return g_Enabled;
}

void PostProcess_SetEnabled(bool on)
{
	g_Enabled = on;
	ShaderLog("[Shader] Post-procesado %s",on ? "ACTIVADO" : "desactivado");
}

// ---------------------------------------------------------------------------
// Helpers GL
// ---------------------------------------------------------------------------
static char* ReadFile(const char* path)
{
	FILE* file = 0;

	fopen_s(&file,path,"rb");

	if(file == 0)
	{
		return 0;
	}

	fseek(file,0,SEEK_END);
	long size = ftell(file);
	fseek(file,0,SEEK_SET);

	if(size <= 0)
	{
		fclose(file);
		return 0;
	}

	char* data = (char*)malloc(size+1);

	if(data == 0)
	{
		fclose(file);
		return 0;
	}

	fread(data,1,size,file);
	data[size] = 0;
	fclose(file);

	return data;
}

static GLuint CompileShader(GLenum type,const char* source,const char* tag)
{
	GLuint shader = glCreateShader(type);

	const GLchar* src = source;

	glShaderSource(shader,1,&src,0);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader,GL_COMPILE_STATUS,&status);

	if(status == 0)
	{
		GLchar log[2048] = {0};
		GLsizei len = 0;
		glGetShaderInfoLog(shader,sizeof(log),&len,log);

		ShaderLog("[Shader] Error compilando %s: %s",tag,log[0] ? log : "(sin detalle)");

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static bool LoadProgram()
{
	char* vs = ReadFile(VS_PATH);
	char* fs = ReadFile(FS_PATH);

	if(vs == 0 || fs == 0)
	{
		ShaderLog("[Shader] No se encontraron %s / %s (carpeta Shader junto al cliente)",VS_PATH,FS_PATH);

		if(vs) free(vs);
		if(fs) free(fs);
		return false;
	}

	GLuint vsShader = CompileShader(GL_VERTEX_SHADER,vs,"post.vs");
	GLuint fsShader = CompileShader(GL_FRAGMENT_SHADER,fs,"post.fs");

	free(vs);
	free(fs);

	if(vsShader == 0 || fsShader == 0)
	{
		if(vsShader) glDeleteShader(vsShader);
		if(fsShader) glDeleteShader(fsShader);
		return false;
	}

	GLuint program = glCreateProgram();

	glAttachShader(program,vsShader);
	glAttachShader(program,fsShader);
	glLinkProgram(program);

	glDeleteShader(vsShader);
	glDeleteShader(fsShader);

	GLint status = 0;
	glGetProgramiv(program,GL_LINK_STATUS,&status);

	if(status == 0)
	{
		GLchar log[2048] = {0};
		GLsizei len = 0;
		glGetProgramInfoLog(program,sizeof(log),&len,log);

		ShaderLog("[Shader] Error enlazando programa: %s",log[0] ? log : "(sin detalle)");

		glDeleteProgram(program);
		return false;
	}

	// Cache de uniforms (los que existan)
	g_LocResolution  = glGetUniformLocation(program,"u_Resolution");
	g_LocTime        = glGetUniformLocation(program,"u_Time");
	g_LocPlayerHP    = glGetUniformLocation(program,"u_PlayerHP");
	g_LocWeather     = glGetUniformLocation(program,"u_WeatherType");
	g_LocTransition  = glGetUniformLocation(program,"u_TransitionProgress");
	g_LocWarmTone    = glGetUniformLocation(program,"u_WarmTone");
	g_LocScreenTex   = glGetUniformLocation(program,"screenTexture");
	g_LocSaturation  = glGetUniformLocation(program,"u_Saturation");
	g_LocContrast    = glGetUniformLocation(program,"u_Contrast");
	g_LocBloomThreshold = glGetUniformLocation(program,"u_BloomThreshold");
	g_LocBloomIntensity = glGetUniformLocation(program,"u_BloomIntensity");
	g_LocVignettePower  = glGetUniformLocation(program,"u_VignettePower");
	g_LocGodRayIntensity = glGetUniformLocation(program,"u_GodRayIntensity");
	g_LocGodRaySamples   = glGetUniformLocation(program,"u_GodRaySamples");
	g_LocGodRayDecay     = glGetUniformLocation(program,"u_GodRayDecay");
	g_LocAnamorphicThreshold = glGetUniformLocation(program,"u_AnamorphicThreshold");
	g_LocAnamorphicIntensity = glGetUniformLocation(program,"u_AnamorphicIntensity");
	g_LocAnamorphicSpread    = glGetUniformLocation(program,"u_AnamorphicSpread");
	g_LocSharpenIntensity    = glGetUniformLocation(program,"u_SharpenIntensity");
	g_LocFilmGrain           = glGetUniformLocation(program,"u_FilmGrain");
	g_LocChromaticAberration = glGetUniformLocation(program,"u_ChromaticAberration");
	g_LocToneMap         = glGetUniformLocation(program,"u_ToneMap");
	g_LocExposure        = glGetUniformLocation(program,"u_Exposure");
	g_LocBloomBlurRadius = glGetUniformLocation(program,"u_BloomBlurRadius");
	g_LocAnamorphicTint  = glGetUniformLocation(program,"u_AnamorphicTint");
	g_LocTransitionStyle = glGetUniformLocation(program,"u_TransitionStyle");

	if(g_Program != 0)
	{
		glDeleteProgram(g_Program);
	}

	g_Program = program;
	g_ShaderLoaded = true;

	ShaderLog("[Shader] Programa compilado OK (GLSL %s)",GetGLSLVersionString());

	return true;
}

// ---------------------------------------------------------------------------
// Carga completa (funciones GL + shaders). Requiere contexto GL actual.
// ---------------------------------------------------------------------------
bool PostProcess_GLCanTry()
{
	return (g_GLReady == false && g_GLInitFailed == false);
}

bool PostProcess_IsReady()
{
	return (g_GLReady && g_Program != 0 && g_ShaderLoaded);
}

int PostProcess_ExceptionFilter(DWORD code)
{
	// Nunca crashear el cliente: loguear, apagar el efecto y seguir.
	ShaderLog("[Shader] EXCEPCION 0x%08X en la pasada de post-procesado: efecto DESACTIVADO (el cliente sigue normal).",code);

	g_DisabledByFault = true;
	g_Enabled = false;

	return EXCEPTION_EXECUTE_HANDLER;
}

void PostProcess_LoadShader()
{
	if(g_GLReady || g_GLInitFailed)
	{
		return;
	}

	if(LoadGLFunctions() == false)
	{
		// Sin contexto GL actual en este hilo (transitorio): reintentar en el
		// proximo frame del cliente. No marcar fallo permanente.
		return;
	}

	if(GLSLSupports330() == false)
	{
		ShaderLog("[Shader] GLSL %s: se requiere 3.30+. Post-procesado DESACTIVADO.",GetGLSLVersionString());
		g_GLInitFailed = true;
		return;
	}

	g_GLReady = true;

	// Compilar el programa AQUI directamente (nunca pasar por ReloadShader:
	// su guard con g_Program==0 re-entraria en LoadShader y nunca compilaria)
	if(LoadProgram() == false)
	{
		ShaderLog("[Shader] Los shaders no compilaron; el efecto queda desactivado (toggle Home reintenta).");
		g_GLInitFailed = true;
	}
}

void PostProcess_ReloadShader()
{
	// El usuario pide reintentar explicitamente (toggle): resetear el fallo
	g_GLInitFailed = false;

	// Sin funciones GL aun: cargar primero (incluye compilar)
	if(g_GLReady == false)
	{
		PostProcess_LoadShader();
		return;
	}

	if(g_Program != 0)
	{
		glDeleteProgram(g_Program);
		g_Program = 0;
		g_ShaderLoaded = false;
	}

	if(LoadProgram() == false)
	{
		ShaderLog("[Shader] Los shaders no compilaron; el efecto queda desactivado hasta corregirlos.");
	}
}

// ---------------------------------------------------------------------------
// Textura de pantalla (backbuffer -> textura RGBA del tamano del viewport)
// ---------------------------------------------------------------------------
static void EnsureScreenTexture(int w,int h)
{
	if(g_ScreenTex != 0 && g_TexW == w && g_TexH == h)
	{
		return;
	}

	if(g_ScreenTex != 0)
	{
		glDeleteTextures(1,&g_ScreenTex);
		g_ScreenTex = 0;
	}

	glGenTextures(1,&g_ScreenTex);
	glBindTexture(GL_TEXTURE_2D,g_ScreenTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,0);

	g_TexW = w;
	g_TexH = h;
}

// ---------------------------------------------------------------------------
// Quad fullscreen (2 triangulos, clip space). V SIN invertir: la fila 0 de la
// textura copiada con glCopyTexSubImage2D es la fila INFERIOR del framebuffer
// (t=0 abajo, t=1 arriba). Con V invertida la pantalla se ve boca abajo.
// ---------------------------------------------------------------------------
static void EnsureQuad()
{
	if(g_QuadVBO != 0)
	{
		return;
	}

	// pos(2) + uv(2) por vertice, 6 vertices
	static const float quad[] =
	{
		-1.0f, -1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,   1.0f, 1.0f,

		-1.0f, -1.0f,   0.0f, 0.0f,
		 1.0f,  1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,   0.0f, 1.0f
	};

	glGenBuffers(1,&g_QuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER,g_QuadVBO);
	glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER,0);

	if(glGenVertexArrays != 0)
	{
		glGenVertexArrays(1,&g_QuadVAO);
		glBindVertexArray(g_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER,g_QuadVBO);
		glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,16,0);
		glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,16,(const void*)8);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glBindVertexArray(0);
	}
}

// ---------------------------------------------------------------------------
// Pasada de post-procesado (llamada con contexto GL actual, antes del swap)
// ---------------------------------------------------------------------------
void PostProcess_Frame(float playerHP,int currentMap)
{
	if(g_DisabledByFault || g_GLReady == false || g_Program == 0 || g_ShaderLoaded == false || g_Enabled == false)
	{
		return;
	}

	EnsureQuad();

	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT,vp);

	int w = vp[2];
	int h = vp[3];

	if(w <= 0 || h <= 0)
	{
		return;
	}

	EnsureScreenTexture(w,h);

	float timeSec = (float)(GetTickCount()/1000.0);

	// --- Config visual: POR MAPA (tabla interna) o global del INI ---
	// (si useMapSettings=1 el look depende del mapa, como el cliente original;
	//  si no, se usan los valores globales de Shader.ini)
	MapShaderSettings ms;
	bool useMap = (g_Config.useMapSettings && currentMap >= 0);

	if(useMap)
	{
		// Si el mapa tiene override en [MapSettings] del INI, se usa ese
		// (permite ajustar la paleta de cada mapa sin recompilar la DLL)
		if(currentMap < MAX_MAP_OVERRIDES && g_MapOverrideSet[currentMap])
		{
			ms = g_MapOverride[currentMap];
		}
		else
		{
			ms = GetMapShaderSettings(currentMap);
		}
	}

	float warmTone[3]    = { g_Config.warmTone[0], g_Config.warmTone[1], g_Config.warmTone[2] };
	float saturation     = g_Config.saturation;
	float contrast       = g_Config.contrast;
	float bloomThreshold = g_Config.bloomThreshold;
	float bloomIntensity = g_Config.bloomIntensity;
	float vignettePower  = g_Config.vignettePower;
	float weather        = (currentMap >= 0 && currentMap < 32) ? g_Config.weather[currentMap] : 0.0f;

	if(useMap)
	{
		warmTone[0]    = ms.warmTone[0];
		warmTone[1]    = ms.warmTone[1];
		warmTone[2]    = ms.warmTone[2];
		saturation     = ms.saturation;
		contrast       = ms.contrast;
		bloomThreshold = ms.bloomThreshold;
		bloomIntensity = ms.bloomIntensity;
		vignettePower  = ms.vignettePower;
		weather        = ms.weatherType;
	}

	// --- Transicion pixelate AUTOMATICA al cambiar de mapa (1.2s) ---
	// Portado de CShaderGL::RenderPostProcess del cliente original:
	//   - Detecta el cambio de mapa
	//   - 0..0.6s: la pantalla se pixeliza (cierra)
	//   - 0.6..1.2s: se despixeliza (abre en el mapa nuevo)
	static int    s_lastMapId = -1;
	static bool   s_transitionActive = false;
	static float  s_transitionTimer = 0.0f;
	static DWORD  s_lastTick = 0;

	float transitionProgress = g_Config.transitionProgress;

	if(g_Config.autoTransition && currentMap >= 0)
	{
		if(s_lastMapId != -1 && s_lastMapId != currentMap)
		{
			s_transitionActive = true;
			s_transitionTimer = 0.0f;
			ShaderLog("[Shader] Mapa %d -> %d: transicion iniciada.",s_lastMapId,currentMap);
		}
		s_lastMapId = currentMap;

		if(s_transitionActive)
		{
			// Delta de tiempo real (si el FPS es bajo la transicion dura igual)
			DWORD now = GetTickCount();
			float delta = (s_lastTick != 0) ? (float)(now - s_lastTick) * 0.001f : 0.016f;
			s_lastTick = now;

			if(delta <= 0.0f || delta > 0.25f)
			{
				delta = 0.016f; // clamp de saltos raros (proceso pausado, etc)
			}

			s_transitionTimer += delta;

			// La duracion es configurable (TransitionDuration, 0.3..5s); la
			// mitad del tiempo cierra la pantalla y la otra mitad abre.
			float halfDur = g_Config.transitionDuration * 0.5f;

			if(s_transitionTimer >= g_Config.transitionDuration)
			{
				s_transitionActive = false;
				s_transitionTimer = 0.0f;
				transitionProgress = 0.0f;
			}
			else if(s_transitionTimer < halfDur)
			{
				transitionProgress = s_transitionTimer / halfDur;        // cierra
			}
			else
			{
				transitionProgress = 1.0f - (s_transitionTimer - halfDur) / halfDur; // abre
			}
		}
		else
		{
			s_lastTick = 0;
		}
	}
	else if(currentMap < 0)
	{
		// Fuera del juego (login/select): resetear para que al entrar al
		// juego no se dispare una transicion fantasma
		s_lastMapId = -1;
		s_transitionActive = false;
		s_transitionTimer = 0.0f;
		s_lastTick = 0;
	}

	// --- Guardar estado GL del cliente ---
	GLint    prevProgram;      glGetIntegerv(GL_CURRENT_PROGRAM,&prevProgram);
	GLint    prevActiveTex;    glGetIntegerv(GL_ACTIVE_TEXTURE,&prevActiveTex);
	GLint    prevTex0;         glGetIntegerv(GL_TEXTURE_BINDING_2D,&prevTex0);
	GLint    prevArrayBuf;     glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&prevArrayBuf);
	GLint    prevVAO;          glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&prevVAO);
	GLint    prevScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX,prevScissorBox);
	GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
	GLboolean blendEnabled   = glIsEnabled(GL_BLEND);
	GLboolean depthEnabled   = glIsEnabled(GL_DEPTH_TEST);
	GLboolean cullEnabled    = glIsEnabled(GL_CULL_FACE);

	// --- Estado minimo para nuestra pasada ---
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,g_ScreenTex);

	// Copiar el frame renderizado por el cliente al backbuffer -> textura
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,w,h);

	// Programa + uniforms
	glUseProgram(g_Program);

	if(g_LocResolution != -1)  glUniform2f(g_LocResolution,(float)w,(float)h);
	if(g_LocTime != -1)        glUniform1f(g_LocTime,timeSec);
	if(g_LocPlayerHP != -1)    glUniform1f(g_LocPlayerHP,playerHP);
	if(g_LocWeather != -1)     glUniform1f(g_LocWeather,weather);
	if(g_LocTransition != -1)  glUniform1f(g_LocTransition,transitionProgress);
	if(g_LocWarmTone != -1)    glUniform3f(g_LocWarmTone,warmTone[0],warmTone[1],warmTone[2]);
	if(g_LocScreenTex != -1)   glUniform1i(g_LocScreenTex,0);
	// Los inicializadores de uniforms en GLSL se IGNORAN (quedan en 0): hay
	// que setear SIEMPRE los parametros configurable desde la DLL, si no la
	// imagen saldria en grises (u_Saturation=0 -> desaturada).
	if(g_LocSaturation != -1)  glUniform1f(g_LocSaturation,saturation);
	if(g_LocContrast != -1)    glUniform1f(g_LocContrast,contrast);
	if(g_LocBloomThreshold != -1) glUniform1f(g_LocBloomThreshold,bloomThreshold);
	if(g_LocBloomIntensity != -1) glUniform1f(g_LocBloomIntensity,bloomIntensity);
	if(g_LocVignettePower != -1)  glUniform1f(g_LocVignettePower,vignettePower);

	// Efectos globales (siempre activos, cualquier mapa)
	if(g_LocGodRayIntensity != -1)      glUniform1f(g_LocGodRayIntensity,g_Config.godRayIntensity);
	if(g_LocGodRaySamples != -1)        glUniform1f(g_LocGodRaySamples,g_Config.godRaySamples);
	if(g_LocGodRayDecay != -1)          glUniform1f(g_LocGodRayDecay,g_Config.godRayDecay);
	if(g_LocAnamorphicThreshold != -1)  glUniform1f(g_LocAnamorphicThreshold,g_Config.anamorphicThreshold);
	if(g_LocAnamorphicIntensity != -1)  glUniform1f(g_LocAnamorphicIntensity,g_Config.anamorphicIntensity);
	if(g_LocAnamorphicSpread != -1)     glUniform1f(g_LocAnamorphicSpread,g_Config.anamorphicSpread);
	if(g_LocSharpenIntensity != -1)     glUniform1f(g_LocSharpenIntensity,g_Config.sharpenIntensity);
	if(g_LocFilmGrain != -1)            glUniform1f(g_LocFilmGrain,g_Config.filmGrain);
	if(g_LocChromaticAberration != -1)  glUniform1f(g_LocChromaticAberration,g_Config.chromaticAberration);
	if(g_LocToneMap != -1)         glUniform1f(g_LocToneMap,g_Config.toneMap ? 1.0f : 0.0f);
	if(g_LocExposure != -1)        glUniform1f(g_LocExposure,g_Config.exposure);
	if(g_LocBloomBlurRadius != -1) glUniform1f(g_LocBloomBlurRadius,g_Config.bloomBlurRadius);
	if(g_LocAnamorphicTint != -1)  glUniform3f(g_LocAnamorphicTint,g_Config.anamorphicTint[0],g_Config.anamorphicTint[1],g_Config.anamorphicTint[2]);
	if(g_LocTransitionStyle != -1) glUniform1f(g_LocTransitionStyle,(float)g_Config.transitionStyle);

	// Quad fullscreen
	if(g_QuadVAO != 0)
	{
		glBindVertexArray(g_QuadVAO);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER,g_QuadVBO);
		glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,16,0);
		glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,16,(const void*)8);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}

	glDrawArrays(GL_TRIANGLES,0,6);

	// --- Restaurar estado del cliente ---
	if(g_QuadVAO != 0)
	{
		glBindVertexArray(prevVAO);
	}
	else
	{
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER,prevArrayBuf);
	}

	glUseProgram(prevProgram);

	glActiveTexture(prevActiveTex);
	glBindTexture(GL_TEXTURE_2D,prevTex0);

	if(prevArrayBuf != 0 && g_QuadVAO != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER,prevArrayBuf);
	}

	if(scissorEnabled) glEnable(GL_SCISSOR_TEST);
	if(blendEnabled)   glEnable(GL_BLEND);
	if(depthEnabled)   glEnable(GL_DEPTH_TEST);
	if(cullEnabled)    glEnable(GL_CULL_FACE);

	if(scissorEnabled)
	{
		glScissor(prevScissorBox[0],prevScissorBox[1],prevScissorBox[2],prevScissorBox[3]);
	}
}

// ---------------------------------------------------------------------------
// Toggle por tecla (poll en el hook de SwapBuffers)
// ---------------------------------------------------------------------------
void PostProcess_CheckToggle()
{
	if(g_DisabledByFault)
	{
		return; // no reintentar tras una excepcion GL
	}

	// (Sin teclas de preset F1/F2/F3: F1-F12 son skills en MU y pisaban el
	//  look. El preset se fija solo desde Shader.ini -> Preset=Retro.)

	if(g_Config.toggleKey == 0)
	{
		return;
	}

	bool down = ((GetAsyncKeyState(g_Config.toggleKey) & 0x8000) != 0);

	if(down && g_KeyWasDown == false)
	{
		g_KeyWasDown = true;

		PostProcess_SetEnabled(!g_Enabled);

		if(g_Enabled && GLHasCurrentContext())
		{
			// Al re-activar, recargar config + shaders desde disco (permite
			// editar Shader.ini / post.fs y ver el cambio sin reiniciar).
			// Solo si hay contexto GL actual (si no, se queda con lo cargado).
			PostProcess_ReloadConfig();
			PostProcess_ReloadShader();
		}
	}
	else if(down == false)
	{
		g_KeyWasDown = false;
	}
}
