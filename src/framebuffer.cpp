#include "framebuffer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace {
std::vector<std::uint32_t> pixels(
    static_cast<std::size_t>(SCREEN_WIDTH) * SCREEN_HEIGHT);
std::vector<float> depthBuffer(
    static_cast<std::size_t>(SCREEN_WIDTH) * SCREEN_HEIGHT);
SDL_Texture* framebufferTexture = nullptr;

std::uint32_t crc32(const std::uint8_t* data, std::size_t length) {
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> values{};
        for (std::uint32_t i = 0; i < values.size(); ++i) {
            std::uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value >> 1U) ^
                        (0xEDB88320U & static_cast<std::uint32_t>(
                            -(static_cast<int>(value & 1U))));
            }
            values[i] = value;
        }
        return values;
    }();
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0; i < length; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

void writeU32(std::ofstream& file, std::uint32_t value) {
    const std::array<char, 4> bytes = {
        static_cast<char>((value >> 24U) & 0xFFU),
        static_cast<char>((value >> 16U) & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU),
        static_cast<char>(value & 0xFFU)};
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeChunk(std::ofstream& file, const char type[4],
                const std::vector<std::uint8_t>& data) {
    writeU32(file, static_cast<std::uint32_t>(data.size()));
    std::vector<std::uint8_t> crcData;
    crcData.reserve(4 + data.size());
    for (int i = 0; i < 4; ++i) {
        crcData.push_back(static_cast<std::uint8_t>(type[i]));
    }
    crcData.insert(crcData.end(), data.begin(), data.end());
    file.write(type, 4);
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
    writeU32(file, crc32(crcData.data(), crcData.size()));
}
}  // namespace

void clear(const Color& color) {
    std::fill(pixels.begin(), pixels.end(), color.toARGB());
    std::fill(depthBuffer.begin(), depthBuffer.end(),
              std::numeric_limits<float>::infinity());
}

void point(int x, int y, float depth, const Color& color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    const auto index = static_cast<std::size_t>(y) * SCREEN_WIDTH + x;
    if (depth < depthBuffer[index]) {
        depthBuffer[index] = depth;
        pixels[index] = color.toARGB();
    }
}

void backgroundPoint(int x, int y, const Color& color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    pixels[static_cast<std::size_t>(y) * SCREEN_WIDTH + x] = color.toARGB();
}

void renderBuffer(SDL_Renderer* renderer) {
    if (framebufferTexture == nullptr) {
        framebufferTexture = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    SDL_UpdateTexture(framebufferTexture, nullptr, pixels.data(),
                      SCREEN_WIDTH * static_cast<int>(sizeof(std::uint32_t)));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, framebufferTexture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool saveBufferAsPNG(const char* filename) {
    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(SCREEN_HEIGHT) * (1 + SCREEN_WIDTH * 3));
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        raw.push_back(0);  // PNG filter: None.
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            const std::uint32_t pixel =
                pixels[static_cast<std::size_t>(y) * SCREEN_WIDTH + x];
            raw.push_back(static_cast<std::uint8_t>((pixel >> 16U) & 0xFFU));
            raw.push_back(static_cast<std::uint8_t>((pixel >> 8U) & 0xFFU));
            raw.push_back(static_cast<std::uint8_t>(pixel & 0xFFU));
        }
    }

    std::vector<std::uint8_t> zlib = {0x78, 0x01};
    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t blockSize = std::min<std::size_t>(65535, raw.size() - offset);
        const bool last = offset + blockSize == raw.size();
        zlib.push_back(last ? 0x01 : 0x00);
        const std::uint16_t length = static_cast<std::uint16_t>(blockSize);
        const std::uint16_t inverse = static_cast<std::uint16_t>(~length);
        zlib.push_back(static_cast<std::uint8_t>(length & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>((length >> 8U) & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>(inverse & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>((inverse >> 8U) & 0xFFU));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset),
                    raw.begin() + static_cast<std::ptrdiff_t>(offset + blockSize));
        offset += blockSize;
    }

    std::uint32_t s1 = 1;
    std::uint32_t s2 = 0;
    for (const std::uint8_t byte : raw) {
        s1 = (s1 + byte) % 65521U;
        s2 = (s2 + s1) % 65521U;
    }
    const std::uint32_t adler = (s2 << 16U) | s1;
    zlib.push_back(static_cast<std::uint8_t>((adler >> 24U) & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((adler >> 16U) & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((adler >> 8U) & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>(adler & 0xFFU));

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    const std::array<unsigned char, 8> signature =
        {137, 80, 78, 71, 13, 10, 26, 10};
    file.write(reinterpret_cast<const char*>(signature.data()), 8);

    std::vector<std::uint8_t> ihdr(13, 0);
    const auto putDimension = [&ihdr](int position, std::uint32_t value) {
        ihdr[position] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
        ihdr[position + 1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
        ihdr[position + 2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        ihdr[position + 3] = static_cast<std::uint8_t>(value & 0xFFU);
    };
    putDimension(0, SCREEN_WIDTH);
    putDimension(4, SCREEN_HEIGHT);
    ihdr[8] = 8;
    ihdr[9] = 2;
    writeChunk(file, "IHDR", ihdr);
    writeChunk(file, "IDAT", zlib);
    writeChunk(file, "IEND", {});
    return static_cast<bool>(file);
}

