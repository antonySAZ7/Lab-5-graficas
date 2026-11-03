#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "framebuffer.h"
#include "mesh.h"
#include "shaders.h"
#include "triangle.h"

namespace {
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Mesh sphere;
Mesh rings;
float rotationY = glm::radians(-12.0f);
int activeScene = 4;
constexpr float PIXELS_PER_UNIT = 105.0f;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error al iniciar SDL: " << SDL_GetError() << '\n';
        return false;
    }
    window = SDL_CreateWindow("Lab 5 - Sistema Ignis (1/2/3/4)",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Error al crear la ventana: " << SDL_GetError() << '\n';
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    return renderer != nullptr;
}

void drawStars() {
    std::mt19937 generator(24710U);
    std::uniform_int_distribution<int> xDistribution(0, SCREEN_WIDTH - 1);
    std::uniform_int_distribution<int> yDistribution(0, SCREEN_HEIGHT - 1);
    std::uniform_int_distribution<int> lightDistribution(80, 220);
    for (int i = 0; i < 310; ++i) {
        const int light = lightDistribution(generator);
        const Color color(static_cast<std::uint8_t>(light * 0.72f),
                          static_cast<std::uint8_t>(light * 0.82f),
                          static_cast<std::uint8_t>(light));
        const int x = xDistribution(generator);
        const int y = yDistribution(generator);
        backgroundPoint(x, y, color);
        if (i % 29 == 0) {
            backgroundPoint(x + 1, y, color);
            backgroundPoint(x, y + 1, color);
        }
    }
}

void drawMesh(const Mesh& mesh, const glm::mat4& modelMatrix,
              FragmentShader shader, bool cullBackFaces = true) {
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
    const glm::mat4 spin = glm::rotate(glm::mat4(1.0f), rotationY,
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    for (const auto& face : mesh.faces) {
        RasterVertex rasterVertices[3];
        for (int i = 0; i < 3; ++i) {
            const Vertex& vertex = mesh.vertices[face[static_cast<std::size_t>(i)]];
            const glm::vec3 objectPosition = glm::vec3(spin * glm::vec4(vertex.position, 1.0f));
            const glm::vec3 worldPosition = glm::vec3(modelMatrix *
                                                       glm::vec4(objectPosition, 1.0f));
            rasterVertices[i].screenPosition = glm::vec3(
                SCREEN_WIDTH * 0.5f + worldPosition.x * PIXELS_PER_UNIT,
                SCREEN_HEIGHT * 0.5f - worldPosition.y * PIXELS_PER_UNIT,
                -worldPosition.z);
            rasterVertices[i].objectPosition = objectPosition;
            rasterVertices[i].worldPosition = worldPosition;
            rasterVertices[i].normal = glm::normalize(normalMatrix *
                glm::vec3(spin * glm::vec4(vertex.normal, 0.0f)));
            rasterVertices[i].uv = vertex.uv;
        }
        if (cullBackFaces &&
            rasterVertices[0].normal.z + rasterVertices[1].normal.z +
                    rasterVertices[2].normal.z <= 0.0f) {
            continue;
        }
        triangle(rasterVertices[0], rasterVertices[1], rasterVertices[2], shader);
    }
}

glm::mat4 transform(const glm::vec3& position, float scale) {
    return glm::translate(glm::mat4(1.0f), position) *
           glm::scale(glm::mat4(1.0f), glm::vec3(scale));
}

void renderStar(bool isolated) {
    const glm::vec3 position = isolated ? glm::vec3(0.0f, 0.0f, 0.0f)
                                       : glm::vec3(-3.25f, 0.70f, 0.05f);
    drawMesh(sphere, transform(position, isolated ? 2.45f : 1.38f), starShader);
}

void renderRocky(bool isolated) {
    const glm::vec3 planetPosition = isolated ? glm::vec3(-0.35f, 0.0f, 0.0f)
                                             : glm::vec3(0.0f, -1.15f, 0.0f);
    const float scale = isolated ? 2.30f : 0.92f;
    drawMesh(sphere, transform(planetPosition, scale), rockyShader);

    // Luna: siempre es otra instancia/modelo, no parte de la esfera del planeta.
    const glm::vec3 moonPosition = planetPosition +
        (isolated ? glm::vec3(2.55f, 1.25f, 0.35f)
                  : glm::vec3(-1.10f, -1.05f, 0.25f));
    drawMesh(sphere, transform(moonPosition, isolated ? 0.42f : 0.23f), moonShader);
}

void renderGasGiant(bool isolated) {
    const glm::vec3 position = isolated ? glm::vec3(0.0f, 0.0f, 0.0f)
                                       : glm::vec3(2.80f, 0.72f, 0.0f);
    const float scale = isolated ? 2.20f : 1.17f;

    // Los anillos son una malla separada y se dibujan antes y despues del planeta
    // gracias al z-buffer; su shader descarta las divisiones transparentes.
    glm::mat4 ringTransform = glm::translate(glm::mat4(1.0f), position) *
        glm::rotate(glm::mat4(1.0f), glm::radians(67.0f), glm::vec3(1, 0, 0)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-13.0f), glm::vec3(0, 0, 1)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    drawMesh(rings, ringTransform, ringShader, false);
    drawMesh(sphere, transform(position, scale), gasGiantShader);
}

void renderScene(int scene) {
    clear();
    drawStars();
    if (scene == 1) {
        renderStar(true);
    } else if (scene == 2) {
        renderRocky(true);
    } else if (scene == 3) {
        renderGasGiant(true);
    } else {
        renderStar(false);
        renderRocky(false);
        renderGasGiant(false);
    }
}

bool saveScene(int scene, const std::filesystem::path& path) {
    renderScene(scene);
    std::filesystem::create_directories(path.parent_path());
    if (!saveBufferAsPNG(path.string().c_str())) {
        std::cerr << "No se pudo guardar " << path.string() << '\n';
        return false;
    }
    std::cout << "Captura creada: " << path.string() << '\n';
    return true;
}
}  // namespace

int main(int argc, char* argv[]) {
    sphere = makeUvSphere();
    rings = makeRing(1.28f, 2.05f);

    if (argc > 1 && std::string(argv[1]) == "--screenshots") {
        bool ok = true;
        ok = saveScene(1, "screenshots/estrella_ignis.png") && ok;
        ok = saveScene(2, "screenshots/planeta_rocoso_cinder.png") && ok;
        ok = saveScene(3, "screenshots/gigante_gaseoso_aurelia.png") && ok;
        ok = saveScene(4, "screenshots/sistema_ignis.png") && ok;
        return ok ? 0 : 1;
    }
    if (argc > 2 && std::string(argv[1]) == "--screenshot") {
        const int scene = std::clamp(std::stoi(argv[2]), 1, 4);
        return saveScene(scene, "captura_actual.png") ? 0 : 1;
    }

    if (!init()) {
        SDL_Quit();
        return 1;
    }

    bool running = true;
    bool redraw = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                const SDL_Keycode key = event.key.keysym.sym;
                if (key >= SDLK_1 && key <= SDLK_4) {
                    activeScene = static_cast<int>(key - SDLK_0);
                    redraw = true;
                } else if (key == SDLK_SPACE) {
                    activeScene = activeScene % 4 + 1;
                    redraw = true;
                } else if (key == SDLK_LEFT) {
                    rotationY -= glm::radians(5.0f);
                    redraw = true;
                } else if (key == SDLK_RIGHT) {
                    rotationY += glm::radians(5.0f);
                    redraw = true;
                } else if (key == SDLK_s) {
                    saveScene(activeScene, "captura_actual.png");
                    redraw = true;
                }
            }
        }

        if (redraw) {
            renderScene(activeScene);
            renderBuffer(renderer);
            redraw = false;
        }
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

