// src/Params.h
#pragma once

#include <algorithm>

namespace dune {

struct Params {
    // Bruit principal
    int   octaves = 5;
    float lacunarity = 1.9f;
    float gain = 0.45f;
    float freq = 0.01f;
    float amp = 40.f;

    // Warp
    float warpAmp = 6.f;
    float warpFreq = 0.008f;

    // Anisotropie
    float stretchX = 1.0f;
    float stretchY = 1.0f;
    float rotationDeg = 0.f;

    // Rugosité fine
    float ridgeBias = 0.25f;
    float ridgePow = 1.5f;
    float ridgeGain = 0.5f;
    float ridgeLacunarity = 2.0f;
    int   ridgeOctaves = 4;
    float ridgeAttenuation = 0.6f;

    // Exploration
    float noiseOffsetX = 0.f;
    float noiseOffsetY = 0.f;
    float noiseZoom    = 1.0f;

    // Crop
    int cropLeft = 0, cropRight = 0, cropTop = 0, cropBottom = 0;

    // Résolution
    int gridW = 128;
    int gridH = 128;

    // Taille terrain (monde) PAR CHUNK
    float terrainWidth  = 128.0f;
    float terrainLength = 128.0f;

    // Chunks
    int   chunkCols = 1;
    int   chunkRows = 1;
    float chunkGapVisual = 1.0f;

    // Flags
    bool filled = false;
    bool invertZ = false;
    bool ridgedMode = true;
    bool warpEnabled = true;

    // Caméra
    float camRotX = 55.f;
    float camRotY = 35.f;
    float camZoom = 280.f;
    float camPanX = 0.f;
    float camPanY = -30.f;

    // Crêtes
    float crestSmoothing = 0.0f;
    float crestSharpen   = 0.0f;
    float crestWidth     = 1.0f;

    void clampSafety()
    {
        gridW = std::max(2, gridW);
        gridH = std::max(2, gridH);
        terrainWidth  = std::max(1.0f, terrainWidth);
        terrainLength = std::max(1.0f, terrainLength);
        noiseZoom = std::max(0.0001f, noiseZoom);
        crestWidth = std::max(1.0f, crestWidth);
        chunkCols = std::max(1, chunkCols);
        chunkRows = std::max(1, chunkRows);
        chunkGapVisual = std::max(0.0f, chunkGapVisual);

        octaves = std::clamp(octaves, 1, 16);
        ridgeOctaves = std::clamp(ridgeOctaves, 1, 16);
    }
};

} // namespace dune