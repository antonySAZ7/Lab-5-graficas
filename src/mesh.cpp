#include "mesh.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

Mesh makeUvSphere(int stacks, int slices) {
    Mesh mesh;
    mesh.vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));

    for (int stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float latitude = glm::half_pi<float>() - v * glm::pi<float>();
        const float ringRadius = std::cos(latitude);
        const float y = std::sin(latitude);

        for (int slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float longitude = u * glm::two_pi<float>();
            const glm::vec3 position(ringRadius * std::sin(longitude), y,
                                     ringRadius * std::cos(longitude));
            mesh.vertices.push_back({position, position, glm::vec2(u, v)});
        }
    }

    const std::size_t row = static_cast<std::size_t>(slices + 1);
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const std::size_t a = static_cast<std::size_t>(stack) * row + slice;
            const std::size_t b = a + row;
            mesh.faces.push_back({a, b, a + 1});
            mesh.faces.push_back({a + 1, b, b + 1});
        }
    }
    return mesh;
}

Mesh makeRing(float innerRadius, float outerRadius, int segments) {
    Mesh mesh;
    mesh.vertices.reserve(static_cast<std::size_t>((segments + 1) * 2));
    for (int i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = u * glm::two_pi<float>();
        const float x = std::sin(angle);
        const float z = std::cos(angle);
        mesh.vertices.push_back({glm::vec3(x * innerRadius, 0.0f, z * innerRadius),
                                 glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(u, 0.0f)});
        mesh.vertices.push_back({glm::vec3(x * outerRadius, 0.0f, z * outerRadius),
                                 glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(u, 1.0f)});
    }
    for (int i = 0; i < segments; ++i) {
        const std::size_t a = static_cast<std::size_t>(i * 2);
        mesh.faces.push_back({a, a + 2, a + 1});
        mesh.faces.push_back({a + 1, a + 2, a + 3});
    }
    return mesh;
}

