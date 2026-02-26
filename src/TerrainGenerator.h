// src/TerrainGenerator.h
#pragma once

#include <vector>

#include "Params.h"
#include "ChunkGrid.h"
#include "Noise.h"

namespace dune {

class TerrainGenerator {
public:
    explicit TerrainGenerator(const Noise& noise);

    void generateChunkGrid(ChunkGrid& grid, int W, int H, const Params& P) const;

    void generateHeights(
        std::vector<float>& out, int W, int Hs, const Params& P,
        float chunkWorldOffsetX, float chunkWorldOffsetY) const;

private:
    const Noise& noise_;

    static void crestPostProcess(std::vector<float>& H, int W, int Hs, const Params& P);
};

} // namespace dune