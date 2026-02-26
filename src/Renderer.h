// src/Renderer.h
#pragma once

#include "Params.h"
#include "ChunkGrid.h"

namespace dune {

class Renderer {
public:
    void setupGL(int winW, int winH);
    void onResize(int winW, int winH);
    void beginFrame();
    void drawAllChunks(const ChunkGrid& grid, int W, int H, const Params& P);

private:
    static void drawMesh(const std::vector<float>& H, int W, int Hs, const Params& P, float visualOffsetX, float visualOffsetY);
};

} // namespace dune