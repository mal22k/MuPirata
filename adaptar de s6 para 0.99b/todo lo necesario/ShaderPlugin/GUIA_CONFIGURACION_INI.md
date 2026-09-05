# 🎮 ShaderPlugin — Guía de configuración de `Shader.ini`

El plugin de post-procesado se configura desde `Shader\Shader.ini` (se crea solo la
primera vez, junto al cliente). Todos los cambios se pueden hacer **en caliente**:
editar el archivo y apagar/encender el efecto con la tecla de toggle (**Home** por
defecto) para recargar sin reiniciar el cliente.

---

## 📁 Estructura del INI

```ini
[Shader]
; ── Activación ─────────────────────────────────────────────
Enabled = 1              ; 1 = activado al arrancar | 0 = apagado
ToggleKey = 36           ; tecla VK para encender/apagar en vivo
                         ;   36 = Home | 33 = PageUp | 34 = PageDown
                         ;   35 = End  | 45 = Insert | 0 = sin tecla
InGameOnly = 1           ; 1 = solo dentro del juego | 0 = también login/select

; ── Look por mapa (portado del cliente original) ───────────
UseMapSettings = 1       ; 1 = cada mapa usa su propia paleta (recomendado)
                         ; 0 = todos los mapas usan los valores globales de abajo
AutoTransition = 1       ; 1 = transición pixel de 1.2s al cambiar de mapa
                         ; 0 = sin transición automática
TransitionProgress = 0.0 ; reserva manual 0..1 (solo si AutoTransition = 0)

; ── Look GLOBAL (SOLO se usan si UseMapSettings = 0) ───────
WarmToneR = 1.10         ; tono cálido/frío del color (multiplicador RGB)
WarmToneG = 1.03         ;   >1.0 = más intenso de ese canal
WarmToneB = 0.92         ;   1.0 = neutro | <1.0 = más apagado
Saturation = 1.02        ; saturación de color (1.0 = natural)
Contrast = 1.00          ; contraste (1.0 = neutro)
BloomThreshold = 0.60    ; brillo mínimo para generar bloom (0..1)
BloomIntensity = 0.15    ; fuerza del bloom (0 = sin bloom)
VignettePower = 0.45     ; oscurecimiento de bordes (0 = sin viñeta)

; ── Efectos (SIEMPRE activos, cualquier mapa) ─────────────
GodRayIntensity = 0.15       ; fuerza de los rayos de luz (0 = sin god rays)
GodRaySamples = 40           ; calidad de los rayos (1..64, más alto = mejor y más lento)
GodRayDecay = 0.96           ; atenuación de los rayos (1.0 = rayos eternos)
AnamorphicThreshold = 0.85   ; brillo mínimo para el destello horizontal
AnamorphicIntensity = 0.20   ; fuerza del destello cinematográfico (0 = sin flare)
AnamorphicSpread = 48.0      ; largo del destello horizontal
SharpenIntensity = 0.25      ; nitidez (0 = sin sharpen, cuidado con halos)
FilmGrain = 0.025            ; grano de película (0 = sin grano)
ChromaticAberration = 0.006  ; aberración cromática en bordes (0 = sin aberración)

; ── Tono cinematográfico / bloom / transiciones ───────────
ToneMap = 1                  ; 1 = curva ACES fílmica (look cine, suave) | 0 = curva clásica
Exposure = 1.00              ; brillo antes del tonemap (solo con ToneMap = 1; 1.1-1.2 si se ve oscuro)
BloomBlurRadius = 2.0        ; suavidad del resplandor del bloom (1.0 ajustado, 3.0 muy suave)
AnamorphicTintR/G/B = 0.60/0.85/1.00 ; color del destello horizontal
TransitionStyle = 0          ; 0 = pixelate | 1 = fade | 2 = zoom | 3 = círculo (iris)
TransitionDuration = 1.2     ; duración de la transición de mapa (0.3 a 5 s)
Preset = Retro               ; Cine | Limpio | Retro | Off (ya NO hay teclas F1/F2/F3:
                         ;    F1-F12 son skills del juego y pisaban el preset)

[PresetCine]
; Un preset es una sección [Preset<Nombre>] que sobreescribe los valores de
; efectos/tono del [Shader]. Cualquier clave que no defina, se toma del [Shader].
Exposure = 1.05
FilmGrain = 0.040
ChromaticAberration = 0.010
AnamorphicIntensity = 0.30
SharpenIntensity = 0.30
GodRayIntensity = 0.18

[WeatherByMap]
; Clima LEGACY (SOLO si UseMapSettings = 0)
; 0 = ninguno | 2 = ondas de calor
Map0 = 0
Map12 = 2
Map13 = 2
```

---

## 🔑 Explicación clave por clave

### `Enabled` (0/1) — activación general
Apaga/enciende TODO el post-procesado al arrancar. Con `0`, el juego se ve
original. También podés usar la tecla de toggle en vivo.

### `ToggleKey` (código VK)
Tecla que activa/desactiva el efecto en tiempo real. También **recarga** el INI y
los shaders al reactivar, así que es la forma de aplicar cambios sin reiniciar.
Códigos útiles:
| Tecla    | VK |
|----------|----|
| Home     | 36 |
| PageUp   | 33 |
| PageDown | 34 |
| End      | 35 |
| Insert   | 45 |
| F1–F12   | 112–123 |
| 0 = sin tecla | — |

### `InGameOnly` (0/1)
Con `1` (recomendado) el efecto solo se aplica dentro del juego (estado 5), no en
login/select. Con `0` también post-procesa las pantallas de menú (puede verse raro
en el login, ya que esas escenas usan look neutro en la tabla por mapa).

### `UseMapSettings` (0/1) ⭐ — el sistema por mapa
Con `1`, **cada mapa tiene su propia paleta** (portado 1:1 del cliente original):
Tarkan con ondas de calor y bloom fuerte, Kalima oscuro, Lorencia cálida, Devias
fría, Blood Castle sangriento, etc. Ver la tabla completa abajo.

Con `0`, todos los mapas usan los valores globales (`WarmTone`, `Saturation`, etc.)
y el clima del `[WeatherByMap]`.

### `AutoTransition` (0/1) ⭐ — transición pixel al cambiar de mapa
Con `1`, al cambiar de mapa se reproduce la transición pixelada de 1.2s
(0–0.6s se cierra pixelada, 0.6–1.2s se abre en el mapa nuevo), igual que el
cliente original.

### `TransitionProgress` (0..1)
Reserva manual para forzar el efecto pixel (por si querés dispararlo desde otro
sistema). Solo tiene efecto si `AutoTransition = 0`.

### Valores globales (solo con `UseMapSettings = 0`)
| Clave            | Default | Qué hace |
|------------------|---------|----------|
| `WarmToneR/G/B`  | 1.10 / 1.03 / 0.92 | Tono de color (multiplicador RGB). Cálido = R alto, frío = B alto |
| `Saturation`     | 1.02    | Saturación. 1.0 = natural, 1.3 = vivo, 0.5 = desaturado |
| `Contrast`       | 1.00    | Contraste. 1.2 = más punch, 0.8 = más suave |
| `BloomThreshold` | 0.60    | Qué tan brillante debe ser un pixel para brillar (más bajo = más bloom) |
| `BloomIntensity` | 0.15    | Fuerza del brillo (0.3 = muy notorio) |
| `VignettePower`  | 0.45    | Viñeta en bordes (0 = sin viñeta, 1 = muy oscuro) |

### Tono cinematográfico (ACES) ⭐
| Clave | Default | Qué hace |
|-------|---------|----------|
| `ToneMap` | 1 | `1` = curva fílmica ACES (colores tipo cine, sombras ricas, brillos suaves). `0` = la curva clásica del shader original |
| `Exposure` | 1.00 | Brillo que se aplica ANTES del tonemap ACES. 1.2 = más brillante, 0.9 = más oscuro |

> ACES se aplica **antes** del contraste/saturación (pipeline HDR → tonemap →
> grade), así no apila contraste y el look queda suave. Si aún lo sentís fuerte,
> bajá `Exposure` a 0.9, o bajá el `Contrast` del mapa en `[MapSettings]`.
> Si se ve demasiado oscuro, subí `Exposure` a 1.15–1.25.

### Transiciones (estilo y duración) ⭐
| Clave | Default | Qué hace |
|-------|---------|----------|
| `TransitionStyle` | 0 | `0` = pixelate (el clásico), `1` = fade (fundido), `2` = zoom, `3` = círculo (iris) |
| `TransitionDuration` | 1.2 | Segundos que dura la transición al cambiar de mapa (0.3 a 5) |

### Bloom y flare finos
| Clave | Default | Qué hace |
|-------|---------|----------|
| `BloomBlurRadius` | 2.0 | Qué tan amplio se difumina el brillo del bloom (1.0 = ajustado, 3.0 = etéreo) |
| `AnamorphicTintR/G/B` | 0.60/0.85/1.00 | Color del destello horizontal (azulado por defecto) |

### Efectos globales (siempre activos) ⭐ — antes eran `#define` en `post.fs`
Estos 9 efectos ahora se configuran desde el INI y **no requieren editar el
shader** (ni recompilar). Se aplican siempre, en todos los mapas:

| Clave | Default | Qué hace |
|-------|---------|----------|
| `GodRayIntensity` | 0.15 | Fuerza de los rayos de luz volumétrica (sol, magias). 0 = sin rayos |
| `GodRaySamples` | 40 | Calidad de los rayos (1..64). Más alto = rayos más suaves pero más lento |
| `GodRayDecay` | 0.96 | Qué tanto se atenúan los rayos al alejarse (1.0 = no se atenúan) |
| `AnamorphicThreshold` | 0.85 | Brillo mínimo para generar el destello horizontal |
| `AnamorphicIntensity` | 0.20 | Fuerza del destello cinematográfico (0 = sin flare) |
| `AnamorphicSpread` | 48.0 | Largo del destello horizontal (más alto = raya más larga) |
| `SharpenIntensity` | 0.25 | Nitidez (0 = sin sharpen; muy alto crea halos en bordes) |
| `FilmGrain` | 0.025 | Grano de película (0 = sin grano; 0.05 = bien visible) |
| `ChromaticAberration` | 0.006 | Aberración cromática en los bordes (0 = sin aberración) |

> 💡 **Receta rápida:** querés look más cinematográfico → `AnamorphicIntensity=0.35`,
> `FilmGrain=0.04`, `ChromaticAberration=0.01`. Querés imagen limpia y nítida →
> `FilmGrain=0`, `ChromaticAberration=0`, `SharpenIntensity=0.30`.

### `[WeatherByMap]` (legacy)
Clima por mapa solo cuando `UseMapSettings = 0`. `2` = ondas de calor (efecto de
aire caliente). Con `UseMapSettings = 1` el clima ya viene en la tabla por mapa
(Tarkan=8, Karutan=80/81 y Scorched Canyon=131 traen ondas de calor).

### `[MapSettings]` ⭐⭐ — overrides de paleta por mapa (¡sin recompilar!)
Con `UseMapSettings = 1` podés **ajustar el look de cualquier mapa** escribiendo
su paleta en el INI. Si un mapa tiene entrada en esta sección, se usa ESE valor
en vez de la tabla interna.

```ini
[MapSettings]
; Formato:  MapN = warmR,warmG,warmB,saturation,contrast,bloomThreshold,
;                  bloomIntensity,vignettePower,weatherType
Map8 = 1.15,0.88,0.78,1.25,1.10,0.68,0.35,0.50,2.0   ; Tarkan con mas bloom
Map24 = 0.70,0.68,0.80,0.80,1.10,0.50,0.25,0.65,0.0  ; Kalima mas oscuro
```

Los 9 valores en orden:
| # | Valor | Qué controla |
|---|-------|--------------|
| 1–3 | `warmR, warmG, warmB` | Tono de color (1.0 = neutro, >1 = más intenso) |
| 4 | `saturation` | Saturación (1.0 = natural) |
| 5 | `contrast` | Contraste (1.0 = neutro) |
| 6 | `bloomThreshold` | Brillo mínimo para bloom (0..1) |
| 7 | `bloomIntensity` | Fuerza del bloom (0 = sin bloom) |
| 8 | `vignettePower` | Viñeta de bordes (0 = sin viñeta) |
| 9 | `weatherType` | Clima (0 = ninguno, 2 = ondas de calor) |

Para **restaurar el look original** de un mapa, borrá (o comentá con `;`) su
línea de `[MapSettings]` y recargá con Home.

### `Preset` + `[PresetCine/Limpio/Retro]` ⭐⭐⭐ — look completo en una línea
Un **preset** es un conjunto de valores de efectos/tono que se activa con una
sola clave (`Preset=Cine`). **Por defecto el plugin usa `Preset=Retro`** (curva
clásica, sin ACES — el look más suave y parecido al cliente original). Los
presets NO tocan las paletas por mapa: se suman encima, así que funcionan igual
con `UseMapSettings = 1`.

```ini
[Shader]
Preset = Cine        ; Cine | Limpio | Retro | Off

[PresetCine]         ; sección [Preset<Nombre>]
Exposure = 1.05
FilmGrain = 0.040
ChromaticAberration = 0.010
AnamorphicIntensity = 0.30
SharpenIntensity = 0.30
GodRayIntensity = 0.18
```

**Reglas:**
- El preset sobreescribe solo las claves que defina; el resto usa los valores
  de `[Shader]`. Las claves válidas son: `ToneMap`, `Exposure`,
  `GodRayIntensity/Samples/Decay`, `AnamorphicThreshold/Intensity/Spread`,
  `SharpenIntensity`, `FilmGrain`, `ChromaticAberration`, `BloomBlurRadius`.
- Podés crear tus propios presets con solo agregar una sección `[PresetXxx]`.
- **Sin teclas en vivo:** F1/F2/F3 se ELIMINARON porque en MU Online F1-F12
  son atajos de skills/pociones y cambiaban el look sin avisar. El preset se
  elige solo desde `Preset=` en `[Shader]` (por defecto: `Retro`).
- `Preset=Off` (o borrar la clave) vuelve a los valores de `[Shader]`.

---

## 🗺️ Tabla de paletas por mapa (UseMapSettings = 1)

Formato: **WarmTone(R,G,B) / Saturación / Contraste / BloomThreshold / BloomIntensity / Vignette / Clima**

| Mapa | ID | Look | Valores |
|------|----|------|---------|
| Lorencia | 0 | Tono cálido | (1.10, 1.03, 0.92) / 1.20 / 1.08 / 0.70 / 0.30 / 0.50 |
| Dungeon, Lost Tower | 1, 4 | Oscuro | (0.85, 0.80, 0.75) / 0.90 / 1.05 / 0.60 / 0.15 / 0.60 |
| Devias | 2 | Tono frío | (0.92, 0.95, 1.08) / 1.10 / 1.05 / 0.65 / 0.25 / 0.50 |
| Noria | 3 | Tono verde | (0.95, 1.05, 0.90) / 1.20 / 1.06 / 0.70 / 0.25 / 0.45 |
| Unknown | 5 | Desierto | (1.10, 1.05, 0.85) / 1.15 / 1.04 / 0.72 / 0.30 / 0.50 |
| Stadium | 6 | Neutro | (1.00, 1.00, 1.00) / 1.00 / 1.00 / 0.80 / 0.10 / 0.35 |
| Atlans, Doppelganger 3 | 7, 67 | Océano luminoso | (0.90, 0.95, 1.15) / 1.15 / 1.02 / 0.60 / 0.25 / 0.35 |
| **Tarkan** | **8** | **Calor** 🔥 | (1.15, 0.88, 0.78) / 1.25 / 1.10 / 0.68 / 0.35 / 0.50 + ondas de calor |
| Devil Square | 9 | Infernal | (1.05, 0.85, 0.80) / 1.15 / 1.08 / 0.65 / 0.30 / 0.55 |
| Icarus | 10 | Cielo | (0.95, 0.98, 1.10) / 1.15 / 1.05 / 0.75 / 0.30 / 0.35 |
| Blood Castle 1–7 (+8) | 11–17, 52 | Sangriento | (1.12, 0.82, 0.78) / 1.20 / 1.10 / 0.60 / 0.35 / 0.55 |
| Chaos Castle 1–4 (+7) | 18–21, 53 | Púrpura | (1.05, 0.85, 0.95) / 1.20 / 1.08 / 0.65 / 0.30 / 0.50 |
| Kalima 1–7 | 24–29, 36 | Muy oscuro | (0.78, 0.75, 0.85) / 0.85 / 1.06 / 0.55 / 0.20 / 0.60 |
| Valley of Loren | 30 | Severo | (0.95, 0.88, 0.82) / 1.10 / 1.06 / 0.65 / 0.25 / 0.50 |
| Land of Trial | 31 | Diurno | (1.05, 1.02, 0.95) / 1.10 / 1.04 / 0.72 / 0.25 / 0.40 |
| Aida | 33 | Bosque oscuro | (0.80, 0.90, 0.75) / 0.95 / 1.05 / 0.60 / 0.20 / 0.55 |
| Crywolf | 34–35 | Hielo | (0.88, 0.92, 1.08) / 1.00 / 1.05 / 0.65 / 0.20 / 0.50 |
| Kanturu 1–2 | 37–38 | Ruinas | (0.90, 0.88, 0.82) / 0.95 / 1.04 / 0.65 / 0.20 / 0.50 |
| Kanturu 3 | 39 | Batalla final | (1.10, 0.85, 0.90) / 1.15 / 1.10 / 0.60 / 0.35 / 0.60 |
| Balgass | 41–42 | Infernal | (1.18, 0.82, 0.72) / 1.20 / 1.12 / 0.60 / 0.40 / 0.55 |
| Illusion Temple 1–6 | 45–50 | Maldito | (0.85, 0.95, 0.80) / 1.05 / 1.06 / 0.60 / 0.25 / 0.55 |
| Elbeland | 51 | Cálido | (1.05, 1.02, 0.95) / 1.15 / 1.05 / 0.72 / 0.25 / 0.40 |
| Swamp of Quiet | 56 | Pantano | (0.82, 0.88, 0.78) / 1.00 / 1.05 / 0.60 / 0.20 / 0.55 |
| Raklion (Ice City) | 57–58 | Congelado | (0.85, 0.88, 1.08) / 1.00 / 1.04 / 0.65 / 0.20 / 0.50 |
| Santa Town | 62 | Festivo | (1.08, 0.95, 0.88) / 1.20 / 1.08 / 0.70 / 0.30 / 0.45 |
| Vulcanus (PK Field) | 63 | Sangriento | (1.10, 0.85, 0.80) / 1.20 / 1.10 / 0.60 / 0.35 / 0.55 |
| Duel Arena | 64 | Neutro | (1.00, 1.00, 1.00) / 1.00 / 1.02 / 0.75 / 0.15 / 0.40 |
| Doppelganger 1/2/4 | 65, 66, 68 | Neutro | (1.00, 1.00, 1.00) / 1.10 / 1.05 / 0.70 / 0.25 / 0.45 |
| Imperial Guardian | 69–72 | Severo | (0.92, 0.90, 0.95) / 1.05 / 1.04 / 0.65 / 0.20 / 0.50 |
| Loren Market | 79 | Diurno | (1.05, 1.02, 0.95) / 1.10 / 1.04 / 0.72 / 0.25 / 0.40 |
| **Karutan 1–2** | **80–81** | **Calor** 🔥 | (1.08, 1.02, 0.85) / 1.15 / 1.06 / 0.70 / 0.30 / 0.50 + ondas de calor |
| Cualquier otro | — | Cálido (default) | (1.10, 1.03, 0.92) / 1.20 / 1.08 / 0.70 / 0.30 / 0.50 |

> **Nota sobre IDs:** los números de mapa son los del cliente original (coinciden
> con `Data\MapManager.txt` del servidor S6). Si tu servidor usa otros IDs,
> avisame y ajustamos la tabla.

---

## ⚙️ post.fs: ya no hay nada que editar

Todos los parámetros del shader se configuran desde `Shader.ini`. El archivo
`post.fs` solo se toca si querés reprogramar un efecto nuevo (se recarga con
**Home**).

---

## 🚀 Flujo de trabajo recomendado

1. Abrí el juego con el efecto activo.
2. Presioná **Home** para apagar, editá el INI (o `post.fs`), y presioná **Home**
   de nuevo para encender → se recarga todo al instante.
3. `UseMapSettings = 1` para el look por mapa (recomendado).
4. Si algo se ve raro, revisá `Shader\shader_plugin.log` — ahí se registran
   errores de shader, transiciones de mapa y excepciones GL.

## 🔍 Referencia rápida de preguntas

- **"Quiero el look por mapa del cliente original"** → `UseMapSettings = 1`
- **"Quiero un solo look para todos los mapas"** → `UseMapSettings = 0` y ajustar
  los valores globales
- **"No quiero la transición pixel"** → `AutoTransition = 0`
- **"Quiero más bloom"** → subir `BloomIntensity` (con `UseMapSettings = 0`), o
  bajar `BloomThreshold`
- **"Todo se ve apagado"** → subir `Saturation` y `Contrast`
- **"Quiero desactivar el efecto en el login"** → `InGameOnly = 1`
- **"Quiero más god rays / menos nitidez / sin grano"** → `GodRayIntensity`,
  `SharpenIntensity`, `FilmGrain` (se recargan con Home)
- **"Quiero un look de película al instante"** → `Preset=Cine` en `[Shader]`
  (recargá con Home)
- **"Tiene mucho contraste"** → es ACES apilándose con el contraste del mapa;
  ya se suavizó (se aplica antes del grade). Si aún así: bajá `Exposure` a 0.9
  o el `Contrast` del mapa en `[MapSettings]`, o `ToneMap=0` para la curva clásica
- **"Se ve muy oscuro / muy quemado"** → subir/bajar `Exposure` (con ToneMap=1)
- **"No me gusta el pixelate al cambiar de mapa"** → `TransitionStyle=1` (fade)
  o `TransitionDuration=0.5` (más rápido)
