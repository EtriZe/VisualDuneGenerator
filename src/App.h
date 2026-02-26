// src/App.h
#pragma once

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <string>
#include <vector>

#include "Params.h"
#include "Noise.h"
#include "TerrainGenerator.h"
#include "Renderer.h"
#include "HeightmapIO.h"
#include "ConfigKV.h"
#include "UI.h"
#include "ChunkGrid.h"

namespace dune {

class App {
public:
    int run();

private:
    void setupImGui_();
    void rebuildAll_(bool force);
    void updateHeights_();
    void handleEvents_();
    void processExports_();
    void mainLoop_();

private:
    // SDL/GL
    SDL_Window*   win_ = nullptr;
    SDL_GLContext ctx_ = nullptr;
    int winW_ = 1400, winH_ = 950;

    // Core
    Params  P_{};
    UiState state_{};
    Noise   noise_{};
    TerrainGenerator gen_{noise_};
    Renderer renderer_{};

    // Data
    int W_ = 128;
    int H_ = 128;
    ChunkGrid chunkGrid_{};
    std::vector<float> heightChunk00_{};

    // Heightmap preview atlas
    GLuint heightTex_ = 0;
    std::vector<unsigned char> heightRGBA_{};
    float lastMinH_ = 0.0f, lastMaxH_ = 0.0f;
    int atlasW_ = 0, atlasH_ = 0;

    // Input state
    bool quit_ = false;
    bool dragging_ = false;
    int lastX_ = 0, lastY_ = 0;

    // Config
    const std::string cfgPath_ = "dune_last_config.cfg";
};

} // namespace dune