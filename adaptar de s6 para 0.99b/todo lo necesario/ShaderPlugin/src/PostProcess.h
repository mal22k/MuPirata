#pragma once

#include <windows.h>

// ============================================================================
// PostProcess.h - modulo de post-procesado (independiente de Lua).
//
// Lee Shader\Shader.ini (config) y Shader\post.vs / Shader\post.fs (GLSL 330),
// compila el programa y aplica la pasada de post-procesado sobre el backbuffer
// justo antes del SwapBuffers. Si la GPU no soporta GLSL 3.3+ se desactiva con
// un log en Shader\shader_plugin.log.
// ============================================================================

// Log a Shader\shader_plugin.log (junto al cliente)
void ShaderLog(const char* fmt, ...);

// Crea Shader\, escribe Shader.ini por defecto si no existe y lee la config.
// No necesita contexto GL.
void PostProcess_Init();

// Re-lee Shader.ini (para cambiar parametros en caliente).
void PostProcess_ReloadConfig();

// Carga las funciones GL y compila los shaders (REQUIERE contexto GL actual,
// es decir: llamar desde dentro del hook de SwapBuffers la primera vez).
// Si falla por falta de contexto GL (transitorio) no marca fallo permanente:
// se puede volver a llamar el proximo frame.
void PostProcess_LoadShader();

// true si aun se puede intentar cargar el GL (no hay fallo permanente).
bool PostProcess_GLCanTry();

// true cuando el programa esta compilado y listo para dibujar.
bool PostProcess_IsReady();

// Filtro de excepciones SEH del hook de SwapBuffers: loguea el codigo,
// desactiva el efecto de forma permanente y devuelve EXCEPTION_EXECUTE_HANDLER
// para que el CLIENTE NUNCA crashee por culpa del post-procesado.
int PostProcess_ExceptionFilter(DWORD code);

// Recompila los shaders desde disco (se llama al activar el toggle).
void PostProcess_ReloadShader();

// Aplica la pasada de post-procesado (REQUIERE contexto GL actual).
// playerHP: 0..1 ratio de vida del personaje. currentMap: id de mapa.
void PostProcess_Frame(float playerHP, int currentMap);

// Estado en runtime (toggle por tecla / config)
bool PostProcess_IsEnabled();
void PostProcess_SetEnabled(bool on);

// Poll de la tecla de toggle (llamar por frame; al re-activar recarga los
// shaders desde disco para ver cambios en post.fs sin reiniciar).
void PostProcess_CheckToggle();

// Config actual
struct ShaderConfig
{
	bool  enabled;             // activado al arrancar (ini [Shader] Enabled)
	int   toggleKey;           // VK de la tecla de toggle (0 = sin tecla)
	bool  inGameOnly;          // 1 = solo aplicar dentro del juego
	bool  useMapSettings;      // 1 = usar config visual POR MAPA (tabla interna, como el cliente original)
	bool  autoTransition;      // 1 = transicion pixelate automatica al cambiar de mapa
	float transitionProgress;  // 0..1 transicion pixelate (reserva manual si autoTransition=0)
	float warmTone[3];         // tono de color (u_WarmTone) [base global, si useMapSettings=0]
	float saturation;          // u_Saturation [base global]
	float contrast;            // u_Contrast [base global]
	float bloomThreshold;      // u_BloomThreshold [base global]
	float bloomIntensity;      // u_BloomIntensity [base global]
	float vignettePower;       // u_VignettePower [base global]
	// Efectos (uniforms de post.fs) — SIEMPRE activos, cualquier mapa:
	// antes vivian como #define en post.fs; ahora se configuran desde el INI.
	float godRayIntensity;     // u_GodRayIntensity (fuerza de los rayos)
	float godRaySamples;       // u_GodRaySamples (calidad, 1..64)
	float godRayDecay;         // u_GodRayDecay (atenuacion)
	float anamorphicThreshold; // u_AnamorphicThreshold (brillo minimo destello)
	float anamorphicIntensity; // u_AnamorphicIntensity (fuerza destello)
	float anamorphicSpread;    // u_AnamorphicSpread (largo destello)
	float sharpenIntensity;    // u_SharpenIntensity (nitidez)
	float filmGrain;           // u_FilmGrain (grano de pelicula)
	float chromaticAberration; // u_ChromaticAberration (aberracion cromatica)
	// Tono cinematico, bloom/flare finos y transiciones
	bool  toneMap;             // 1 = curva ACES filmica | 0 = curva clasica
	float exposure;            // brillo antes del tonemap (ACES)
	float bloomBlurRadius;     // u_BloomBlurRadius (suavidad del bloom)
	float anamorphicTint[3];   // u_AnamorphicTint (color del destello)
	int   transitionStyle;     // 0 pixelate | 1 fade | 2 zoom | 3 circulo
	float transitionDuration;  // segundos de la transicion de mapa (0.3..5)
	char  preset[32];          // preset activo ("Off" o vacio = ninguno)
	float weather[32];         // clima por mapa legacy (solo si useMapSettings=0)
};

const ShaderConfig& ShaderConfig_Get();
