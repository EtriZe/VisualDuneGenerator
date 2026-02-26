// src/ChunkGrid.h
#pragma once

#include <vector>

namespace dune {

struct ChunkGrid {
    std::vector<std::vector<float>> heights; // size rows*cols, each is W*H
    int cols = 1;
    int rows = 1;

    static inline int index(int c, int r, int cols){ return r * cols + c; }
    bool empty() const { return heights.empty() || cols<=0 || rows<=0; }
};

} // namespace dune