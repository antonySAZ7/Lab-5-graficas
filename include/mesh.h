#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::array<std::size_t, 3>> faces;
};

Mesh makeUvSphere(int stacks = 48, int slices = 96);
Mesh makeRing(float innerRadius, float outerRadius, int segments = 240);

