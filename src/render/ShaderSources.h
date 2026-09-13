#pragma once
// =============================================================================
//  ShaderSources.h — Shaders GLSL 330 core intégrés (raw string literals)
//
//  Trois paires vertex/fragment :
//    1. Phong  — éclairage directionnel pour profilés 3D, nœuds, appuis
//    2. Heatmap — cartographie de contraintes colorées (Turbo / Viridis)
//    3. Flat   — rendu à plat pour grille, axes, diagrammes, wireframe
// =============================================================================

namespace shaders {

// ─────────────────────────────────────────────────────────────────────────────
//  1. PHONG — éclairage Blinn-Phong bi-face
// ─────────────────────────────────────────────────────────────────────────────
inline constexpr const char* PHONG_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal  = normalize(uNormalMatrix * aNormal);
    gl_Position = uProjection * uView * worldPos;
}
)glsl";

inline constexpr const char* PHONG_FRAG = R"glsl(
#version 330 core
in vec3 vFragPos;
in vec3 vNormal;

uniform vec3  uObjectColor;
uniform vec3  uLightDir;     // direction VERS la lumière (normalisée)
uniform vec3  uViewPos;
uniform float uAlpha;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);

    // Bi-face : on éclaire aussi le côté arrière
    float NdotL = dot(N, L);
    float diff  = max(NdotL, 0.0) + max(-NdotL, 0.0) * 0.5;

    float NdotH = abs(dot(N, H));
    float spec  = pow(NdotH, 64.0) * 0.35;

    float ambient = 0.18;

    vec3 color = (ambient + diff * 0.72 + spec) * uObjectColor;
    FragColor = vec4(color, uAlpha);
}
)glsl";

// ─────────────────────────────────────────────────────────────────────────────
//  2. HEATMAP — vertex-colored avec rampe de couleur scientifique
// ─────────────────────────────────────────────────────────────────────────────
inline constexpr const char* HEATMAP_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aValue;   // valeur normalisée 0..1

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3  vFragPos;
out vec3  vNormal;
out float vValue;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal  = normalize(uNormalMatrix * aNormal);
    vValue   = aValue;
    gl_Position = uProjection * uView * worldPos;
}
)glsl";

inline constexpr const char* HEATMAP_FRAG = R"glsl(
#version 330 core
in vec3  vFragPos;
in vec3  vNormal;
in float vValue;

uniform vec3  uLightDir;
uniform vec3  uViewPos;
uniform int   uColormapType;  // 0 = Turbo, 1 = Viridis

out vec4 FragColor;

// Google Turbo colormap (polynomial approximation)
vec3 turbo(float t) {
    const vec4 kR4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 kG4 = vec4(0.09140261, 2.19418839,   4.84296658, -14.18503333);
    const vec4 kB4 = vec4(0.10667330, 12.64194608,-60.58204836, 110.36276771);
    const vec2 kR2 = vec2(-152.94239396, 59.28637943);
    const vec2 kG2 = vec2(   4.27729857,  2.82956604);
    const vec2 kB2 = vec2( -89.90310912, 27.34824973);
    t = clamp(t, 0.0, 1.0);
    vec4 v4 = vec4(1.0, t, t*t, t*t*t);
    vec2 v2 = v4.zw * v4.z;
    return clamp(vec3(
        dot(v4, kR4) + dot(v2, kR2),
        dot(v4, kG4) + dot(v2, kG2),
        dot(v4, kB4) + dot(v2, kB2)
    ), 0.0, 1.0);
}

// Viridis colormap (polynomial approximation)
vec3 viridis(float t) {
    const vec3 c0 = vec3(0.2777, 0.0054, 0.3340);
    const vec3 c1 = vec3(0.1050, 0.6039, 0.6635);
    const vec3 c2 = vec3(-0.3308, 1.2167, -0.6789);
    const vec3 c3 = vec3(-4.6342,-5.7991, 3.5898);
    const vec3 c4 = vec3( 6.2282, 14.1799,-5.5290);
    const vec3 c5 = vec3( 4.7763,-13.7452, 1.8537);
    const vec3 c6 = vec3(-5.4354, 4.6459, 0.7581);
    t = clamp(t, 0.0, 1.0);
    return clamp(c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6))))), 0.0, 1.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(abs(dot(N, L)), 0.0);
    float lighting = 0.22 + diff * 0.78;

    vec3 ramp = (uColormapType == 0) ? turbo(vValue) : viridis(vValue);
    FragColor = vec4(ramp * lighting, 1.0);
}
)glsl";

// ─────────────────────────────────────────────────────────────────────────────
//  3. FLAT — couleur par vertex, pas d'éclairage (grille, axes, diagrammes)
// ─────────────────────────────────────────────────────────────────────────────
inline constexpr const char* FLAT_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)glsl";

inline constexpr const char* FLAT_FRAG = R"glsl(
#version 330 core
in vec3 vColor;

uniform float uAlpha;

out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, uAlpha);
}
)glsl";

} // namespace shaders
