#version 330 core

// #############################################################################
// MU ONLINE - POST-PROCESSING SHADER (ULTRA NATURAL + GOD RAYS + SHARPENING)
// Portado del proyecto de referencia (OpenGL) a GLSL 330 core.
// FIXES vs original: los array constructors se cambiaron de vec2[]/float[]
// (sintaxis GLSL 4.0+) a vec2[N]/float[N] (GLSL 1.20+ / 330) para que
// compile en OpenGL 3.3.
// #############################################################################

// --- Configuracion rapida (valores base; TODO lo demas se configura desde
// Shader.ini, ya no hace falta editar este archivo) ---
#define BLOOM_THRESHOLD  0.60
#define BLOOM_INTENSITY  0.15
#define VIGNETTE_POWER   0.45
#define SATURATION       1.02
#define CONTRAST         1.00

// CONFIGURACION DE GOD RAYS (Rayos de Luz)
// Maximo de samples en TIEMPO DE COMPILACION (limite del loop); el valor real
// se configura desde Shader.ini (u_GodRaySamples, 1..64).
#define GOD_RAY_MAX_SAMPLES  64

// CONFIGURACION DE SHARPENING (Nitidez) - se configura desde Shader.ini
// (SharpenIntensity -> u_SharpenIntensity)

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform vec2 u_Resolution;
uniform float u_Time;

// === Parametros configurables por mapa ===
uniform vec3  u_WarmTone   = vec3(1.10, 1.03, 0.92);
uniform float u_Saturation = SATURATION;
uniform float u_Contrast   = CONTRAST;
uniform float u_BloomThreshold = BLOOM_THRESHOLD;
uniform float u_BloomIntensity = BLOOM_INTENSITY;
uniform float u_VignettePower  = VIGNETTE_POWER;
uniform float u_PlayerHP = 1.0;

// === Progreso de la transicion entre mapas ===
uniform float u_TransitionProgress = 0.0;

// === Tipo de clima (0=ninguno, 2=ondas de calor) ===
uniform float u_WeatherType = 0.0;

// === Efectos configurables desde Shader.ini ===
// (god rays, destello anamorfico, nitidez, grano y aberracion cromatica:
//  ya no hace falta editar post.fs para ajustarlos)
uniform float u_GodRayIntensity     = 0.15;   // fuerza de los rayos de luz
uniform float u_GodRaySamples       = 40.0;   // calidad de los rayos (1..64)
uniform float u_GodRayDecay         = 0.96;   // atenuacion de los rayos
uniform float u_AnamorphicThreshold = 0.85;   // brillo minimo del destello
uniform float u_AnamorphicIntensity = 0.20;   // fuerza del destello horizontal
uniform float u_AnamorphicSpread    = 48.0;   // largo del destello
uniform float u_SharpenIntensity    = 0.25;   // nitidez (cuidado con halos)
uniform float u_FilmGrain           = 0.025;  // grano de pelicula (0 = off)
uniform float u_ChromaticAberration = 0.006;  // aberracion cromatica (0 = off)

// === Tono cinematico, bloom/flare finos y transiciones (desde Shader.ini) ===
uniform float u_ToneMap           = 1.0;    // 1 = curva ACES filmica | 0 = curva clasica
uniform float u_Exposure          = 1.0;    // brillo antes del tonemap (ACES)
uniform float u_BloomBlurRadius   = 2.0;    // suavidad del resplandor del bloom
uniform vec3  u_AnamorphicTint    = vec3(0.6, 0.85, 1.0); // color del destello
uniform float u_TransitionStyle   = 0.0;    // 0 pixelate | 1 fade | 2 zoom | 3 circulo

// ============================================================================
// SOFT THRESHOLD (Umbral Suave)
// ============================================================================
float calcBloomFactor(vec3 c)
{
    float bright = max(max(c.r, c.g), c.b);
    return smoothstep(u_BloomThreshold * 0.5, u_BloomThreshold, bright);
}

// ============================================================================
// BLOOM CON BLUR GAUSSIANO
// ============================================================================
vec3 applyBloom(vec3 color)
{
    vec2 texelSize = 1.0 / u_Resolution;
    float radius = u_BloomBlurRadius;

    vec3 bloomColor = vec3(0.0);
    float totalWeight = 0.0;

    float cb = calcBloomFactor(color);
    bloomColor += color * cb * 0.20;
    totalWeight += 0.20;

    vec2 cardOffsets[8] = vec2[8](
        vec2( 1.5,  0.0), vec2(-1.5,  0.0), vec2( 0.0,  1.5), vec2( 0.0, -1.5),
        vec2( 3.5,  0.0), vec2(-3.5,  0.0), vec2( 0.0,  3.5), vec2( 0.0, -3.5)
    );
    float cardWeights[2] = float[2](0.12, 0.05);
    for (int i = 0; i < 8; i++) {
        int ring = i / 4;
        vec3 s = texture(screenTexture, TexCoord + cardOffsets[i] * texelSize * radius).rgb;
        float bf = calcBloomFactor(s);
        bloomColor += s * bf * cardWeights[ring];
        totalWeight += cardWeights[ring];
    }

    vec2 diagOffsets[8] = vec2[8](
        vec2( 1.5,  1.5), vec2(-1.5,  1.5), vec2( 1.5, -1.5), vec2(-1.5, -1.5),
        vec2( 3.5,  3.5), vec2(-3.5,  3.5), vec2( 3.5, -3.5), vec2(-3.5, -3.5)
    );
    float diagWeights[2] = float[2](0.04, 0.01);
    for (int i = 0; i < 8; i++) {
        int ring = i / 4;
        vec3 s = texture(screenTexture, TexCoord + diagOffsets[i] * texelSize * radius).rgb;
        float bf = calcBloomFactor(s);
        bloomColor += s * bf * diagWeights[ring];
        totalWeight += diagWeights[ring];
    }

    vec3 finalBloom = (bloomColor / max(totalWeight, 0.001)) * u_BloomIntensity;
    return color + finalBloom;
}

// ============================================================================
// ANAMORPHIC FLARE
// ============================================================================
vec3 applyAnamorphicFlare(vec3 color)
{
    vec2 texelSize = 1.0 / u_Resolution;
    vec3 flareColor = vec3(0.0);
    float totalWeight = 0.0;

    const int SAMPLES = 16;
    float weights[16] = float[16](
        0.02, 0.04, 0.07, 0.11, 0.15, 0.18, 0.20, 0.22,
        0.22, 0.20, 0.18, 0.15, 0.11, 0.07, 0.04, 0.02
    );

    for (int i = 0; i < SAMPLES; i++)
    {
        float offset = float(i - 8) * (u_AnamorphicSpread / 16.0);
        vec2 sampleCoord = TexCoord + vec2(offset * texelSize.x, 0.0);

        if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0)
        {
            vec3 sampled = texture(screenTexture, sampleCoord).rgb;
            float bright = max(max(sampled.r, sampled.g), sampled.b);

            float flareFactor = smoothstep(u_AnamorphicThreshold, 1.0, bright);
            flareColor += sampled * flareFactor * weights[i] * u_AnamorphicTint;
            totalWeight += weights[i];
        }
    }

    if (totalWeight > 0.0)
    {
        flareColor /= totalWeight;
    }

    return color + flareColor * u_AnamorphicIntensity;
}

// ============================================================================
// GOD RAYS (Rayos de Luz Volumetrica)
// ============================================================================
vec3 applyGodRays(vec3 color, vec2 uv)
{
    // Fuente de luz: Parte superior central (como el sol)
    vec2 lightPos = vec2(0.5, 1.05);
    vec2 deltaTexCoord = (uv - lightPos) * 0.012;
    vec2 currentTexCoord = uv;

    vec3 rayColor = vec3(0.0);
    float illuminationDecay = 1.0;

    int samples = int(clamp(u_GodRaySamples, 1.0, float(GOD_RAY_MAX_SAMPLES)));
    for(int i = 0; i < GOD_RAY_MAX_SAMPLES; i++)
    {
        if(i >= samples) break;

        currentTexCoord -= deltaTexCoord;
        vec3 sampleColor = texture(screenTexture, currentTexCoord).rgb;
        float brightness = max(max(sampleColor.r, sampleColor.g), sampleColor.b);

        // Solo generar rayos si el pixel es MUY brillante (sol, magias fuertes)
        if(brightness > 0.80)
        {
            rayColor += sampleColor * illuminationDecay * 0.04;
        }
        illuminationDecay *= u_GodRayDecay;
    }

    return color + rayColor * u_GodRayIntensity;
}

// ============================================================================
// SHARPENING (Nitidez HD / Unsharp Mask)
// ============================================================================
vec3 applySharpen(vec3 color, vec2 uv)
{
    vec2 texelSize = 1.0 / u_Resolution;
    vec3 sum = vec3(0.0);

    sum += texture(screenTexture, uv + vec2(-1.0, -1.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2( 0.0, -1.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2( 1.0, -1.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2(-1.0,  0.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2( 0.0,  0.0) * texelSize) *  2.0;
    sum += texture(screenTexture, uv + vec2( 1.0,  0.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2(-1.0,  1.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2( 0.0,  1.0) * texelSize) * -0.25;
    sum += texture(screenTexture, uv + vec2( 1.0,  1.0) * texelSize) * -0.25;

    return mix(color, sum, u_SharpenIntensity);
}

// ============================================================================
// TONO CINEMATOGRAFICO (ACES Filmic - Narkowicz 2015)
// ============================================================================
vec3 ACESFilm(vec3 x)
{
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

// ============================================================================
// CLIMA: ONDAS DE CALOR
// ============================================================================
vec3 applyHeatHaze(vec3 color, vec2 uv, float time)
{
    float heatStrength = (1.0 - uv.y) * 0.008;
    vec2 distortion = vec2(
        sin(uv.y * 20.0 + time * 2.0) * heatStrength,
        cos(uv.x * 15.0 + time * 1.5) * heatStrength * 0.5
    );
    vec2 distortedUV = uv + distortion;
    vec3 distortedColor = texture(screenTexture, distortedUV).rgb;
    float mixAmount = (1.0 - uv.y) * 0.6;
    return mix(color, distortedColor, mixAmount);
}

// ============================================================================
// MAIN
// ============================================================================
void main()
{
    // === ABERRACION CROMATICA ===
    vec2 dir = TexCoord - 0.5;
    float dist = length(dir);
    float aberrationAmount = dist * dist * u_ChromaticAberration;

    vec4 color;
    color.r = texture(screenTexture, TexCoord + dir * aberrationAmount).r;
    color.g = texture(screenTexture, TexCoord).g;
    color.b = texture(screenTexture, TexCoord - dir * aberrationAmount).b;
    color.a = 1.0;

    // === BLOOM SUAVE ===
    color.rgb = applyBloom(color.rgb);

    // === ANAMORPHIC FLARE ===
    color.rgb = applyAnamorphicFlare(color.rgb);

    // === GOD RAYS ===
    color.rgb = applyGodRays(color.rgb, TexCoord);

    // === TONO CINEMATOGRAFICO (ACES) - aplicado en HDR, ANTES del color grade ===
    // Pipeline estandar (HDR -> tonemap -> grade): asi la curva filmica NO se
    // apila con el contraste/saturacion posteriores y el resultado es suave,
    // sin el contraste excesivo de aplicar ACES despues del contraste.
    if (u_ToneMap > 0.5)
    {
        color.rgb = ACESFilm(color.rgb * u_Exposure);
    }

    // === FILM GRAIN ===
    float grainNoise = fract(sin(dot(TexCoord * u_Resolution + u_Time * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    color.rgb += (grainNoise - 0.5) * u_FilmGrain;

    // === Tono de color ===
    color.rgb *= u_WarmTone;

    // === Saturacion ===
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = mix(vec3(gray), color.rgb, u_Saturation);

    // === Contraste Natural ===
    // (con ACES activo el tonemap ya se hizo arriba: aca solo queda el grade;
    //  con ToneMap=0 se conserva exactamente la curva clasica del original)
    color.rgb = (color.rgb - 0.5) * u_Contrast + 0.5;
    if (u_ToneMap < 0.5)
    {
        color.rgb = color.rgb * 0.84 + 0.08;
    }

    // === Vineta ===
    vec2 center = TexCoord - 0.5;
    float vignetteDist = length(center);
    float vignette = 1.0 - vignetteDist * vignetteDist * u_VignettePower;

    // === EFECTO HP BAJO ===
    float hpRatio = u_PlayerHP;
    float lowHp = 1.0 - smoothstep(0.15, 0.35, hpRatio);

    if (lowHp > 0.001)
    {
        float heartRate = 4.0 * (1.0 + (1.0 - hpRatio) * 3.0);
        float heartBeat = sin(u_Time * heartRate);
        float pulse = max(0.0, heartBeat) * 0.7 + 0.3;
        float redIntensity = lowHp * 0.30 * pulse;
        color.rgb = mix(color.rgb, vec3(1.0, 0.15, 0.15), redIntensity);
        vignette *= 1.0 - lowHp * 0.25 * pulse;
    }

    color.rgb *= vignette;

    // === TRANSICION ENTRE MAPAS ===
    // u_TransitionStyle: 0 = pixelate, 1 = fade, 2 = zoom, 3 = circulo (iris)
    // Curva monotona 0->1: la transicion cierra y abre la pantalla.
    if (u_TransitionProgress > 0.001)
    {
        float t = clamp(u_TransitionProgress, 0.0, 1.0);
        float smoothT = t * t * (3.0 - 2.0 * t);
        float wave = sin(smoothT * 1.5707963);
        vec3 magicTint = vec3(0.8, 0.85, 1.0);

        if (u_TransitionStyle < 0.5)          // 0: PIXELATE
        {
            float maxPixelSize = 38.0;
            float pixelSize = 1.0 + (maxPixelSize - 1.0) * wave;

            vec2 pixelCoord = floor(TexCoord * u_Resolution / pixelSize) * pixelSize / u_Resolution;
            vec3 pixelatedColor = texture(screenTexture, pixelCoord).rgb;

            pixelatedColor *= (1.0 - wave * 0.6);
            pixelatedColor = mix(pixelatedColor, magicTint, wave * 0.15);

            color.rgb = pixelatedColor;
        }
        else if (u_TransitionStyle < 1.5)     // 1: FADE
        {
            color.rgb = mix(color.rgb, magicTint, wave * 0.8);
            color.rgb *= (1.0 - wave * 0.45);
        }
        else if (u_TransitionStyle < 2.5)     // 2: ZOOM
        {
            float zoom = 1.0 + 0.4 * smoothT;
            vec2 zoomedUV = (TexCoord - 0.5) / zoom + 0.5;
            vec3 zoomedColor = texture(screenTexture, zoomedUV).rgb;
            color.rgb = mix(color.rgb, zoomedColor, smoothT);
            color.rgb *= (1.0 - smoothT * 0.35);
        }
        else                                  // 3: CIRCULO (iris)
        {
            float d = distance(TexCoord, vec2(0.5, 0.5));
            float radius = 0.75 - smoothT * 0.72;
            float edge = smoothstep(radius + 0.04, radius, d);
            color.rgb = mix(color.rgb, vec3(0.05, 0.08, 0.12), edge * 0.9);
        }
    }

    // === CLIMA: SOLO ONDAS DE CALOR ===
    if (u_WeatherType > 1.5 && u_WeatherType < 2.5)
    {
        color.rgb = applyHeatHaze(color.rgb, TexCoord, u_Time);
    }

    // === SHARPENING - SOLO SI NO HAY TRANSICION ===
    if (u_TransitionProgress < 0.001)
    {
        color.rgb = applySharpen(color.rgb, TexCoord);
    }

    // === SALIDA FINAL ===
    FragColor = clamp(vec4(color.rgb, 1.0), 0.0, 1.0);
}
