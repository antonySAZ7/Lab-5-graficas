#pragma once

#include <glm/glm.hpp>

#include "shaders.h"

struct RasterVertex {
    glm::vec3 screenPosition;
    glm::vec3 objectPosition;
    glm::vec3 worldPosition;
    glm::vec3 normal;
    glm::vec2 uv;
};

void triangle(const RasterVertex& a, const RasterVertex& b,
              const RasterVertex& c, FragmentShader shader);

