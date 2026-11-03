#include "triangle.h"

#include <algorithm>
#include <cmath>

#include "framebuffer.h"

namespace {
float edge(const glm::vec3& a, const glm::vec3& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}
}  // namespace

void triangle(const RasterVertex& a, const RasterVertex& b,
              const RasterVertex& c, FragmentShader shader) {
    const float signedArea = edge(a.screenPosition, b.screenPosition,
                                  c.screenPosition.x, c.screenPosition.y);
    if (std::abs(signedArea) < 0.0001f) {
        return;
    }

    const int minX = std::max(0, static_cast<int>(std::floor(std::min(
                                     {a.screenPosition.x, b.screenPosition.x,
                                      c.screenPosition.x}))));
    const int maxX = std::min(SCREEN_WIDTH - 1, static_cast<int>(std::ceil(std::max(
                                                    {a.screenPosition.x,
                                                     b.screenPosition.x,
                                                     c.screenPosition.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min(
                                     {a.screenPosition.y, b.screenPosition.y,
                                      c.screenPosition.y}))));
    const int maxY = std::min(SCREEN_HEIGHT - 1, static_cast<int>(std::ceil(std::max(
                                                     {a.screenPosition.y,
                                                      b.screenPosition.y,
                                                      c.screenPosition.y}))));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float sampleX = static_cast<float>(x) + 0.5f;
            const float sampleY = static_cast<float>(y) + 0.5f;
            const float w0 = edge(b.screenPosition, c.screenPosition, sampleX,
                                  sampleY) / signedArea;
            const float w1 = edge(c.screenPosition, a.screenPosition, sampleX,
                                  sampleY) / signedArea;
            const float w2 = edge(a.screenPosition, b.screenPosition, sampleX,
                                  sampleY) / signedArea;

            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                const float depth = w0 * a.screenPosition.z +
                                    w1 * b.screenPosition.z +
                                    w2 * c.screenPosition.z;
                Fragment fragment;
                fragment.objectPosition = w0 * a.objectPosition +
                                          w1 * b.objectPosition +
                                          w2 * c.objectPosition;
                fragment.worldPosition = w0 * a.worldPosition +
                                         w1 * b.worldPosition +
                                         w2 * c.worldPosition;
                fragment.normal = glm::normalize(w0 * a.normal + w1 * b.normal +
                                                 w2 * c.normal);
                fragment.uv = w0 * a.uv + w1 * b.uv + w2 * c.uv;
                const ShaderResult result = shader(fragment);
                if (!result.discard) {
                    point(x, y, depth, result.color);
                }
            }
        }
    }
}

