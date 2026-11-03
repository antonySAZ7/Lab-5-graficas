#pragma once

#include <glm/glm.hpp>

#include "color.h"

struct Fragment {
    glm::vec3 objectPosition;
    glm::vec3 worldPosition;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct ShaderResult {
    Color color;
    bool discard = false;
};

using FragmentShader = ShaderResult (*)(const Fragment& fragment);

ShaderResult starShader(const Fragment& fragment);
ShaderResult rockyShader(const Fragment& fragment);
ShaderResult gasGiantShader(const Fragment& fragment);
ShaderResult moonShader(const Fragment& fragment);
ShaderResult ringShader(const Fragment& fragment);

