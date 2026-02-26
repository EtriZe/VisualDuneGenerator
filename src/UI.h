#pragma once

#include <string>
#include <cstdint>
#include "imgui.h"
#include "Params.h"

namespace dune {

using TextureHandle = std::uint32_t; // correspond à GLuint dans la pratique

struct UiState {
    bool needUpdate = false;
    bool needRebuild = false;

    bool requestExportPGM = false;
    bool requestExportPNG = false;
    bool requestExportAllChunksPNG = false;

    bool autoLoadConfigOnStart = true;
    bool autoSaveConfigOnExit  = true;

    std::string configStatus;
    std::string lastExportPath;
};

class UI {
public:
    static void drawControls(Params& P, int W, int H, UiState& S, const std::string& cfgPath);

    static void drawHeightmapWindow(
        const Params& P,
        UiState& S,
        TextureHandle heightTex,
        int atlasW, int atlasH,
        float minH, float maxH);
};

} // namespace dune