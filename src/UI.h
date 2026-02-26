// src/UI.h
#pragma once

#include <string>
#include "imgui.h"

#include "Params.h"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace dune {

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
        GLuint heightTex,
        int atlasW, int atlasH,
        float minH, float maxH);
};

} // namespace dune