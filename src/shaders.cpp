#include "shaders.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace {
float saturate(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float hash31(const glm::vec3& point) {
    // Hash algebraico: evita el seno por muestra y hace viable el shader en CPU.
    glm::vec3 value = glm::fract(point * 0.1031f);
    value += glm::dot(value, glm::vec3(value.y, value.z, value.x) + 33.33f);
    return glm::fract((value.x + value.y) * value.z);
}

float valueNoise(const glm::vec3& point) {
    const glm::vec3 cell = glm::floor(point);
    glm::vec3 local = glm::fract(point);
    local = local * local * (3.0f - 2.0f * local);

    const float n000 = hash31(cell + glm::vec3(0, 0, 0));
    const float n100 = hash31(cell + glm::vec3(1, 0, 0));
    const float n010 = hash31(cell + glm::vec3(0, 1, 0));
    const float n110 = hash31(cell + glm::vec3(1, 1, 0));
    const float n001 = hash31(cell + glm::vec3(0, 0, 1));
    const float n101 = hash31(cell + glm::vec3(1, 0, 1));
    const float n011 = hash31(cell + glm::vec3(0, 1, 1));
    const float n111 = hash31(cell + glm::vec3(1, 1, 1));
    const float x00 = glm::mix(n000, n100, local.x);
    const float x10 = glm::mix(n010, n110, local.x);
    const float x01 = glm::mix(n001, n101, local.x);
    const float x11 = glm::mix(n011, n111, local.x);
    return glm::mix(glm::mix(x00, x10, local.y),
                    glm::mix(x01, x11, local.y), local.z);
}

float fbm(glm::vec3 point, int octaves = 5) {
    float result = 0.0f;
    float amplitude = 0.52f;
    for (int octave = 0; octave < octaves; ++octave) {
        result += valueNoise(point) * amplitude;
        point = point * 2.03f + glm::vec3(17.1f, 9.2f, 13.7f);
        amplitude *= 0.5f;
    }
    return result;
}

float ridge(glm::vec3 point) {
    return 1.0f - std::abs(fbm(point) * 2.0f - 1.0f);
}

float plasmaNoise(const glm::vec3& point) {
    // Ruido ondulatorio continuo: ideal para plasma y mucho mas barato que
    // interpolar ocho hashes por cada octava del FBM.
    const float warp = std::sin(glm::dot(point, glm::vec3(1.13f, 1.71f, 2.37f)));
    return 0.5f + 0.5f * std::sin(
        glm::dot(point, glm::vec3(-2.17f, 1.31f, 0.83f)) + warp * 1.8f);
}

float diffuseLight(const glm::vec3& normal, float ambient = 0.16f) {
    const glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.55f, 0.72f, 0.9f));
    return ambient + (1.0f - ambient) *
           std::max(0.0f, glm::dot(normal, lightDirection));
}

float sphericalSpot(const glm::vec3& direction, const glm::vec3& center,
                    float radius, float softness) {
    // Los shaders llaman esta funcion con direcciones y centros unitarios.
    const float similarity = glm::dot(direction, center);
    return glm::smoothstep(std::cos(radius + softness),
                           std::cos(radius - softness), similarity);
}
}  // namespace

ShaderResult starShader(const Fragment& fragment) {
    const glm::vec3 p = glm::normalize(fragment.objectPosition);

    // Capa 1: gradiente termico base, amarillo en el centro y rojo al borde.
    const float facing = saturate(fragment.normal.z);
    glm::vec3 color = glm::mix(glm::vec3(0.78f, 0.07f, 0.015f),
                               glm::vec3(1.0f, 0.68f, 0.08f),
                               std::pow(facing, 0.38f));

    // Capa 2: granulos de conveccion a varias escalas.
    const float granules = plasmaNoise(p * 8.0f) * 0.62f +
                           plasmaNoise(p * 19.0f + glm::vec3(3.0f)) * 0.38f;
    color *= 0.78f + 0.48f * granules;

    // Capa 3: filamentos de plasma curvos alrededor del eje.
    const float plasma = 0.5f + 0.5f * std::sin((p.x + p.z * 0.7f) * 13.0f +
                                                p.y * 18.0f +
                                                plasmaNoise(p * 3.5f) * 9.0f);
    const float filament = glm::smoothstep(0.72f, 0.97f, plasma);
    color = glm::mix(color, glm::vec3(1.0f, 0.93f, 0.38f), filament * 0.48f);

    // Capa 4: manchas solares oscuras con un borde incandescente.
    const float spotA = sphericalSpot(p, glm::normalize(glm::vec3(-0.42f, 0.25f, 0.87f)),
                                      0.17f, 0.025f);
    const float spotB = sphericalSpot(p, glm::normalize(glm::vec3(0.48f, -0.32f, 0.81f)),
                                      0.11f, 0.02f);
    const float spots = saturate(spotA + spotB);
    const float spotNoise = 0.55f + 0.45f * plasmaNoise(p * 17.0f);
    color = glm::mix(color, glm::vec3(0.22f, 0.015f, 0.008f), spots * spotNoise);

    // Capa 5: corona fina que perfila el limbo.
    const float rim = std::pow(1.0f - facing, 4.0f);
    color += glm::vec3(0.55f, 0.12f, 0.015f) * rim;
    return {toColor(color), false};
}

ShaderResult rockyShader(const Fragment& fragment) {
    const glm::vec3 p = glm::normalize(fragment.objectPosition);

    // Capa 1: roca basaltica con continentes minerales.
    const float continents = fbm(p * 2.7f + glm::vec3(4.0f, 1.0f, 7.0f));
    glm::vec3 color = glm::mix(glm::vec3(0.035f, 0.045f, 0.065f),
                               glm::vec3(0.24f, 0.18f, 0.15f),
                               glm::smoothstep(0.38f, 0.72f, continents));

    // Capa 2: vetas de oxidos metalicos.
    const float minerals = ridge(p * 7.5f + glm::vec3(3.0f, 8.0f, 2.0f));
    const float mineralMask = glm::smoothstep(0.70f, 0.91f, minerals);
    color = glm::mix(color, glm::vec3(0.48f, 0.16f, 0.055f), mineralMask * 0.72f);

    // Capa 3: red de lava en las grietas mas estrechas del ruido fractal.
    const float cracks = std::abs(fbm(p * 5.5f + glm::vec3(9.0f)) * 2.0f - 1.0f);
    const float lava = 1.0f - glm::smoothstep(0.02f, 0.105f, cracks);
    const glm::vec3 lavaColor = glm::mix(glm::vec3(0.75f, 0.025f, 0.005f),
                                         glm::vec3(1.0f, 0.73f, 0.08f),
                                         saturate(lava * 1.6f));
    color = glm::mix(color, lavaColor, lava * 0.92f);

    // Capa 4: tres crateres, cada uno con cuenca y borde levantado.
    const glm::vec3 centers[] = {
        glm::vec3(-0.38f, 0.42f, 0.82f), glm::vec3(0.52f, -0.18f, 0.84f),
        glm::vec3(0.08f, -0.62f, 0.72f)};
    const float radii[] = {0.18f, 0.12f, 0.09f};
    for (int i = 0; i < 3; ++i) {
        const float similarity = glm::dot(p, glm::normalize(centers[i]));
        const float basin = glm::smoothstep(std::cos(radii[i] * 0.9f),
                                            std::cos(radii[i] * 0.65f), similarity);
        const float ringDistance = std::abs(similarity - std::cos(radii[i]));
        const float ring = 1.0f - glm::smoothstep(0.0015f, 0.006f, ringDistance);
        color *= 1.0f - basin * 0.42f;
        color += glm::vec3(0.24f, 0.12f, 0.06f) * ring;
    }

    // Capa 5: luz direccional fria; la lava conserva su emision en la noche.
    const float light = diffuseLight(fragment.normal, 0.10f);
    color *= glm::mix(light, 1.0f, lava * 0.78f);
    const float rim = std::pow(1.0f - saturate(fragment.normal.z), 3.0f);
    color += glm::vec3(0.05f, 0.045f, 0.08f) * rim;
    return {toColor(color), false};
}

ShaderResult gasGiantShader(const Fragment& fragment) {
    const glm::vec3 p = glm::normalize(fragment.objectPosition);

    // Capa 1: bandas atmosfericas por latitud.
    const float turbulence = fbm(glm::vec3(p.x * 3.0f, p.y * 9.0f, p.z * 3.0f));
    const float warpedLatitude = p.y + (turbulence - 0.5f) * 0.14f;
    const float bands = 0.5f + 0.5f * std::sin(warpedLatitude * 31.0f);
    glm::vec3 color = glm::mix(glm::vec3(0.055f, 0.17f, 0.30f),
                               glm::vec3(0.18f, 0.72f, 0.78f),
                               glm::smoothstep(0.16f, 0.88f, bands));

    // Capa 2: cinturones oscuros de diferente anchura.
    const float belts = std::pow(std::abs(std::sin(warpedLatitude * 15.5f)), 9.0f);
    color = glm::mix(color, glm::vec3(0.025f, 0.085f, 0.18f), belts * 0.65f);

    // Capa 3: nubes claras pequenas arrastradas por los vientos.
    const float clouds = glm::smoothstep(0.58f, 0.82f,
        fbm(glm::vec3(p.x * 8.0f + p.y * 5.0f, p.y * 18.0f, p.z * 8.0f), 4));
    color = glm::mix(color, glm::vec3(0.68f, 0.94f, 0.91f), clouds * 0.50f);

    // Capa 4: la Gran Tormenta Esmeralda, elipse con ojo y remolino.
    const glm::vec3 stormCenter = glm::normalize(glm::vec3(0.38f, -0.23f, 0.90f));
    const float storm = sphericalSpot(p, stormCenter, 0.27f, 0.035f);
    const float eye = sphericalSpot(p, stormCenter, 0.075f, 0.025f);
    const float swirl = 0.5f + 0.5f * std::sin(
        std::atan2(p.y + 0.23f, p.x - 0.38f) * 5.0f + turbulence * 11.0f);
    const glm::vec3 stormColor = glm::mix(glm::vec3(0.10f, 0.92f, 0.62f),
                                          glm::vec3(0.74f, 1.0f, 0.70f), swirl);
    color = glm::mix(color, stormColor, storm * (1.0f - eye * 0.55f));
    color = glm::mix(color, glm::vec3(0.025f, 0.16f, 0.20f), eye * 0.85f);

    // Capa 5: aurora polar y luz direccional.
    const float poles = std::pow(saturate((std::abs(p.y) - 0.68f) / 0.32f), 2.0f);
    color += glm::vec3(0.04f, 0.28f, 0.35f) * poles;
    color *= diffuseLight(fragment.normal, 0.18f);
    return {toColor(color), false};
}

ShaderResult moonShader(const Fragment& fragment) {
    const glm::vec3 p = glm::normalize(fragment.objectPosition);
    const float highlands = fbm(p * 5.0f + glm::vec3(2.0f, 7.0f, 1.0f));
    glm::vec3 color = glm::mix(glm::vec3(0.12f, 0.14f, 0.17f),
                               glm::vec3(0.48f, 0.52f, 0.56f), highlands);
    const float maria = glm::smoothstep(0.48f, 0.70f, fbm(p * 2.2f));
    color = glm::mix(color, glm::vec3(0.11f, 0.16f, 0.20f), maria * 0.62f);
    const float crater1 = sphericalSpot(p, glm::normalize(glm::vec3(-0.25f, 0.30f, 0.92f)),
                                        0.24f, 0.03f);
    const float crater2 = sphericalSpot(p, glm::normalize(glm::vec3(0.46f, -0.18f, 0.86f)),
                                        0.13f, 0.025f);
    color *= 1.0f - (crater1 + crater2) * 0.30f;
    color *= diffuseLight(fragment.normal, 0.12f);
    return {toColor(color), false};
}

ShaderResult ringShader(const Fragment& fragment) {
    const float radius = glm::length(glm::vec2(fragment.objectPosition.x,
                                                fragment.objectPosition.z));
    // Cuatro capas: color radial, bandas finas, divisiones transparentes y radios.
    const float radialGradient = saturate((radius - 1.28f) / (2.05f - 1.28f));
    glm::vec3 color = glm::mix(glm::vec3(0.10f, 0.26f, 0.32f),
                               glm::vec3(0.60f, 0.88f, 0.78f), radialGradient);
    const float fineBands = 0.5f + 0.5f * std::sin(radius * 145.0f);
    color *= 0.48f + 0.52f * fineBands;
    const float gapA = 1.0f - glm::smoothstep(0.012f, 0.030f, std::abs(radius - 1.57f));
    const float gapB = 1.0f - glm::smoothstep(0.008f, 0.020f, std::abs(radius - 1.86f));
    if (gapA > 0.50f || gapB > 0.58f || fineBands < 0.065f) {
        return {Color(), true};
    }
    const float angle = std::atan2(fragment.objectPosition.z,
                                   fragment.objectPosition.x);
    const float spokes = 0.84f + 0.16f * std::sin(angle * 28.0f + radius * 9.0f);
    color *= spokes * diffuseLight(fragment.normal, 0.38f);
    return {toColor(color), false};
}

