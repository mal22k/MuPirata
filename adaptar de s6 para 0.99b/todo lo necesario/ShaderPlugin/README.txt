============================================================================
ShaderPlugin - DLL de post-procesado (shaders GLSL) para MU Online EX603
============================================================================

QUE ES
------
Plugin INDEPENDIENTE del sistema LuaPlugin. Aplica un post-procesado al
frame renderizado por el cliente (OpenGL) justo antes del SwapBuffers:
  - Bloom suave
  - Anamorphic flare (destello cinematografico)
  - God rays (rayos de luz volumetrica)
  - Film grain
  - Tono de color / saturacion / contraste
  - Vineta
  - Efecto de HP bajo (corazon pulsante rojo en los bordes)
  - Transicion pixelate (cambio de mapa, desactivada por defecto)
  - Clima: ondas de calor (configurable por mapa)

Los shaders viven en Shaders\post.vs / Shaders\post.fs (GLSL 330) y se
pueden editar SIN recompilar la DLL (portados del proyecto OpenGL de
referencia: post.fs con bloom, flare, god rays y sharpening).

REGISTRO (metodo GetMainInfo, igual que LuaPlugin)
---------------------------------------------------
1) Compila con build.bat  ->  Output\Shader.dll
2) Copia Shader.dll a la carpeta del cliente (junto a main.exe)
3) Crea la carpeta Shader\ junto al cliente con:
     Shader\post.vs
     Shader\post.fs
     Shader\Shader.ini   (opcional, se crea con valores por defecto)
4) MainInfo.ini de GetMainInfo:
     [MainInfo]
     PluginName1 = Lua.dll
     PluginName2 = Shader.dll
5) Ejecuta GetMainInfo.exe para regenerar ServerInfo.sse (CRC de la DLL)
6) Abre el cliente. Veras el efecto de post-procesado al entrar al juego.

CONFIG (Shader\Shader.ini)
--------------------------
[Shader]
Enabled = 1              ; 0/1 activar el post-procesado al arrancar
ToggleKey = 36           ; tecla VK para activar/desactivar en vivo
                         ;  36 = VK_HOME (libre en el cliente)
                         ;  0  = sin tecla
InGameOnly = 1           ; 1 = solo dentro del juego (estado 5)
UseMapSettings = 1       ; 1 = config visual POR MAPA (tabla interna portada
                         ;     del cliente original: cada mapa tiene su propia
                         ;     paleta de tono/saturacion/bloom/vineta/clima)
                         ; 0 = usar los valores globales WarmTone/Saturation/...
AutoTransition = 1       ; 1 = transicion pixelate automatica (1.2s) al
                         ;     cambiar de mapa (como el cliente original)
TransitionProgress = 0.0 ; reserva manual 0..1 (solo si AutoTransition = 0)
; Los valores globales siguientes SOLO se usan si UseMapSettings = 0:
WarmToneR = 1.10         ; tono de color (se multiplica sobre la imagen)
WarmToneG = 1.03
WarmToneB = 0.92
Saturation = 1.02        ; saturacion (1.0 = neutro)
Contrast = 1.00          ; contraste (1.0 = neutro)
BloomThreshold = 0.60    ; brillo minimo para generar bloom
BloomIntensity = 0.15    ; fuerza del bloom
VignettePower = 0.45     ; fuerza de la vineta (0 = sin vineta)

[WeatherByMap]
; Clima por mapa (SOLO si UseMapSettings = 0): 0 = ninguno, 2 = ondas de calor.
; Con UseMapSettings = 1 el clima sale de la tabla interna por mapa
; (Tarkan=8, Karutan=80/81, Scorched Canyon=131 con ondas de calor).

NOTA: los parametros se pueden cambiar en caliente: edita Shader.ini,
apaga/enciende con Home y el efecto usa los valores nuevos.

SHADERS POR MAPA (que hace UseMapSettings = 1)
---------------------------------------------
Portado de la funcion GetMapShaderSettings() del cliente original (Main5.2).
Cada mapa del juego tiene su propia identidad visual:
  - Lorencia calida, Dungeon/Lost Tower oscuros, Devias fria, Noria verde
  - Tarkan y Karutan con ondas de calor, Kalima muy oscuro, Blood Castle
    sangriento, Icarus celeste, Atlans oceano luminoso, etc.
  - Las escenas de login/character usan look neutro.
Los IDs de mapa usados son los del enum WD_* del cliente original
(MapManager.h), que coinciden con los del servidor S6.

FIX DEL CRASH (v0.1.1) - IMPORTANTE
----------------------------------
La v0.1.0 crasheaba el main.exe al primer frame. Causa: el SetCompleteHook
clasico de SSeMU NO salva los bytes originales, y el hook llamaba a
"g_OriginalSwapBuffers" que era la MISMA direccion parcheada -> recursion
infinita -> stack overflow. Arreglado con un TRAMPOLIN real (memoria
ejecutable via VirtualAlloc) que copia las instrucciones completas del
prologo y salta de vuelta. Maneja los prologos reales de SwapBuffers:
  - Windows 10/11:  FF 25 <disp32>  (thunk a win32u!SwapBuffers)
  - Hotpatch:       8B FF 55 8B EC
  - Clasico:        55 8B EC 83 EC xx
Si el prologo no se puede decodificar o SwapBuffers ya esta hookeado por
otro modulo, la DLL se omite sola (log en Shader\shader_plugin.log) en vez
de romper la cadena.

Ademas, toda la pasada GL corre dentro de __try/__except (SEH): si algo
falla en runtime (contexto raro, driver, offset), se loguea el codigo de
excepcion, el efecto se apaga solo y EL CLIENTE NUNCA CRASHEA por el
post-procesado. Las lecturas de memoria del cliente (ventana, HP, mapa)
usan VirtualQuery para validar punteros antes de dereferenciar, y el
lazy-init de GL reintenta cada frame hasta que hay contexto.

NOTAS
-----
- El efecto de HP bajo se alimenta SOLO de la memoria del cliente
  (Life/MaxLife del personaje) - sin puente con Lua.
- Si tu GPU no soporta GLSL 3.3+, la DLL se desactiva sola y lo registra
  en Shader\shader_plugin.log.
- Test: test\glsl_check.exe compila los shaders en un contexto GL real
  de tu maquina (util para verificar cambios en post.fs).
- Test: test\test_trampoline.exe (regresion del crash) carga la DLL,
  instala el hook y llama a SwapBuffers 200 veces: verifica que NO hay
  recursion (con el bug, reventaba la pila en las primeras llamadas).
