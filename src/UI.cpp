// src/UI.cpp
#include "UI.h"

#include <algorithm>

#include "ConfigKV.h"

namespace dune
{

    void UI::drawControls(Params &P, int W, int H, UiState &S, const std::string &cfgPath)
    {
        ImGui::Begin("Dune Studio Controls");

        if (ImGui::CollapsingHeader("Bruit principal", ImGuiTreeNodeFlags_DefaultOpen))
        {
            S.needUpdate |= ImGui::SliderInt("Octaves", &P.octaves, 1, 10);
            S.needUpdate |= ImGui::SliderFloat("Freq", &P.freq, 0.002f, 0.1f);
            S.needUpdate |= ImGui::SliderFloat("Lacunarity", &P.lacunarity, 1.0f, 3.0f);
            S.needUpdate |= ImGui::SliderFloat("Gain", &P.gain, 0.1f, 0.9f);
            S.needUpdate |= ImGui::SliderFloat("Amplitude", &P.amp, 5.f, 500.f);
        }

        if (ImGui::CollapsingHeader("Warp / Déformation"))
        {
            S.needUpdate |= ImGui::Checkbox("Warp Enabled", &P.warpEnabled);
            S.needUpdate |= ImGui::SliderFloat("Warp Freq", &P.warpFreq, 0.001f, 0.02f);
            S.needUpdate |= ImGui::SliderFloat("Warp Amp", &P.warpAmp, 0.f, 20.f);
        }

        if (ImGui::CollapsingHeader("Anisotropie / Orientation"))
        {
            S.needUpdate |= ImGui::SliderFloat("Stretch X", &P.stretchX, 0.5f, 2.f);
            S.needUpdate |= ImGui::SliderFloat("Stretch Y", &P.stretchY, 0.5f, 2.f);
            S.needUpdate |= ImGui::SliderFloat("Rotation", &P.rotationDeg, -90.f, 90.f);
        }

        if (ImGui::CollapsingHeader("Rugosité fine (Ridged)"))
        {
            S.needUpdate |= ImGui::Checkbox("Ridged Mode", &P.ridgedMode);
            S.needUpdate |= ImGui::SliderInt("Ridge Octaves", &P.ridgeOctaves, 1, 6);
            S.needUpdate |= ImGui::SliderFloat("Ridge Lacunarity", &P.ridgeLacunarity, 1.0f, 3.0f);
            S.needUpdate |= ImGui::SliderFloat("Ridge Gain", &P.ridgeGain, 0.2f, 0.9f);
            S.needUpdate |= ImGui::SliderFloat("Ridge Bias", &P.ridgeBias, 0.f, 0.5f);
            S.needUpdate |= ImGui::SliderFloat("Ridge Pow", &P.ridgePow, 0.5f, 3.f);
            S.needUpdate |= ImGui::SliderFloat("Ridge Atten", &P.ridgeAttenuation, 0.2f, 1.f);
        }

        if (ImGui::CollapsingHeader("Affinage des Crêtes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            S.needUpdate |= ImGui::SliderFloat("Crest Smoothing", &P.crestSmoothing, 0.0f, 1.0f);
            S.needUpdate |= ImGui::SliderFloat("Crest Sharpen", &P.crestSharpen, 0.0f, 1.0f);
            S.needUpdate |= ImGui::SliderFloat("Crest Width", &P.crestWidth, 1.0f, 4.0f);
        }

        if (ImGui::CollapsingHeader("Exploration du bruit", ImGuiTreeNodeFlags_DefaultOpen))
        {
            S.needUpdate |= ImGui::SliderFloat("Offset X", &P.noiseOffsetX, -1000.f, 1000.f);
            S.needUpdate |= ImGui::SliderFloat("Offset Y", &P.noiseOffsetY, -1000.f, 1000.f);
            S.needUpdate |= ImGui::SliderFloat("Zoom Bruit", &P.noiseZoom, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Terrain (dimensions world)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            S.needUpdate |= ImGui::SliderFloat("Largeur", &P.terrainWidth, 16.0f, 2000.0f);
            S.needUpdate |= ImGui::SliderFloat("Longueur", &P.terrainLength, 16.0f, 2000.0f);
        }

        if (ImGui::CollapsingHeader("Chunks", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool c1 = ImGui::SliderInt("Chunk Cols", &P.chunkCols, 3, 32);
            bool c2 = ImGui::SliderInt("Chunk Rows", &P.chunkRows, 1, 32);
            bool c3 = ImGui::SliderFloat("Chunk Gap (visual)", &P.chunkGapVisual, 0.0f, 50.0f);
            if (c1 || c2 || c3)
                S.needUpdate = true;

            ImGui::Text("Total chunks: %d", std::max(1, P.chunkCols) * std::max(1, P.chunkRows));

            if (ImGui::Button("Export all chunks PNG"))
            {
                S.requestExportAllChunksPNG = true;
            }
        }

        if (ImGui::CollapsingHeader("Rognage"))
        {
            int maxW = std::max(0, W - 1);
            int maxH = std::max(0, H - 2);
            S.needUpdate |= ImGui::SliderInt("Left", &P.cropLeft, 0, maxW);
            S.needUpdate |= ImGui::SliderInt("Right", &P.cropRight, 0, maxW);
            S.needUpdate |= ImGui::SliderInt("Top", &P.cropTop, 0, maxH);
            S.needUpdate |= ImGui::SliderInt("Bottom", &P.cropBottom, 0, maxH);
        }

        if (ImGui::CollapsingHeader("Résolution"))
        {
            bool r1 = ImGui::SliderInt("Verts X", &P.gridW, 16, 1024);
            bool r2 = ImGui::SliderInt("Verts Y", &P.gridH, 16, 1024);
            if (r1 || r2)
                S.needRebuild = true;
        }

        if (ImGui::CollapsingHeader("Caméra"))
        {
            ImGui::SliderFloat("Rot X", &P.camRotX, -90.f, 90.f);
            ImGui::SliderFloat("Rot Y", &P.camRotY, -180.f, 180.f);
            ImGui::SliderFloat("Zoom", &P.camZoom, 50.f, 5000.f);
            ImGui::SliderFloat("Pan X", &P.camPanX, -2000.f, 2000.f);
            ImGui::SliderFloat("Pan Y", &P.camPanY, -2000.f, 2000.f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Wireframe / Fill", &P.filled);
        S.needUpdate |= ImGui::Checkbox("Invert Z", &P.invertZ);

        if (ImGui::Button("Regenerate"))
            S.needUpdate = true;

        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            P = Params();
            S.needRebuild = true;
            S.needUpdate = true;
        }

        ImGui::Separator();
        ImGui::Text("Configuration");

        if (ImGui::Button("Save config"))
        {
            if (ConfigKV::save(P, cfgPath))
                S.configStatus = "Config sauvegardee: " + cfgPath;
            else
                S.configStatus = "Erreur sauvegarde config";
        }

        ImGui::SameLine();
        if (ImGui::Button("Load config"))
        {
            if (ConfigKV::load(P, cfgPath))
            {
                S.configStatus = "Config chargee: " + cfgPath;
                S.needRebuild = true;
                S.needUpdate = true;
            }
            else
            {
                S.configStatus = "Erreur chargement config (fichier absent ?)";
            }
        }

        ImGui::Checkbox("Auto-load startup config", &S.autoLoadConfigOnStart);
        ImGui::Checkbox("Auto-save on exit", &S.autoSaveConfigOnExit);

        ImGui::TextWrapped("Fichier: %s", cfgPath.c_str());
        if (!S.configStatus.empty())
            ImGui::TextWrapped("%s", S.configStatus.c_str());

        ImGui::End();
    }

    void UI::drawHeightmapWindow(
        const Params &P,
        UiState &S,
        GLuint heightTex,
        int atlasW, int atlasH,
        float minH, float maxH)
    {
        ImGuiIO &io = ImGui::GetIO();
        const float margin = 12.0f;
        const ImVec2 wndSize(420.0f, 380.0f);

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - wndSize.x - margin, margin), ImGuiCond_Always);
        ImGui::SetNextWindowSize(wndSize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("Heightmap", nullptr, flags);

        if (ImGui::Button("Export heightmap (PGM)"))
            S.requestExportPGM = true;
        ImGui::SameLine();
        if (ImGui::Button("Export heightmap (PNG)"))
            S.requestExportPNG = true;
        ImGui::SameLine();
        ImGui::Text("min %.2f  max %.2f", minH, maxH);

        ImGui::Text("Preview: atlas %dx%d chunks", std::max(1, P.chunkCols), std::max(1, P.chunkRows));

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float imgW = avail.x;
        float imgH = avail.y;

        float ar = (atlasH > 0) ? (float)atlasW / (float)atlasH : 1.0f;
        if (imgH > 0.0f && imgW > 0.0f)
        {
            float targetW = imgW;
            float targetH = imgW / ar;
            if (targetH > imgH)
            {
                targetH = imgH;
                targetW = imgH * ar;
            }
            if (heightTex != 0)
                ImGui::Image((ImTextureID)(intptr_t)heightTex, ImVec2(targetW, targetH));
            else
                ImGui::Text("No texture");
        }

        if (!S.lastExportPath.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Dernier export: %s", S.lastExportPath.c_str());
        }

        ImGui::End();
    }

} // namespace dune