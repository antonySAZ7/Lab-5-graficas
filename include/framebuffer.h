#pragma once

#include <SDL.h>

#include "color.h"

constexpr int SCREEN_WIDTH = 1000;
constexpr int SCREEN_HEIGHT = 760;

void clear(const Color& color = Color(5, 7, 18));
void point(int x, int y, float depth, const Color& color);
void backgroundPoint(int x, int y, const Color& color);
void renderBuffer(SDL_Renderer* renderer);
bool saveBufferAsPNG(const char* filename);

