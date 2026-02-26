// src/HeightmapIO.h
#pragma once

#include <string>
#include <vector>
#include <GL/gl.h>

#include "ChunkGrid.h"

namespace dune {

class HeightmapIO {
public:
    static void buildRGBA8(
        const std::vector<float>& heights, int W, int H,
        std::vector<unsigned char>& outRGBA,
        float* outMin=nullptr, float* outMax=nullptr);

    static void buildChunkAtlasRGBA8(
        const ChunkGrid& grid, int W, int H, int gapPx,
        std::vector<unsigned char>& outRGBA,
        float* outMin=nullptr, float* outMax=nullptr,
        int* outAtlasW=nullptr, int* outAtlasH=nullptr);

    static void ensureOrUpdateTextureRGBA8(GLuint& tex, int W, int H, const unsigned char* dataRGBA);

    static std::string makeTimestampedFilename(const char* prefix, const char* ext);

    static bool exportPGM(const std::vector<float>& heights, int W, int H, const std::string& filename);
    static bool exportPNG(const std::vector<float>& heights, int W, int H, const std::string& filename);

    static bool exportAllChunksPNG(
        const ChunkGrid& grid, int W, int H,
        const std::string& basePrefix,
        std::string& outMessage);
};

} // namespace dune