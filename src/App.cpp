// src/App.cpp
#include "App.h"

#include <iostream>
#include <algorithm>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl2.h"

namespace dune
{

    int App::run()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cerr << "SDL_Init failed\n";
            return 1;
        }

        win_ = SDL_CreateWindow("Dune Studio — Advanced Viewer",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 950,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (!win_)
        {
            std::cerr << "SDL_CreateWindow failed\n";
            SDL_Quit();
            return 1;
        }

        ctx_ = SDL_GL_CreateContext(win_);
        if (!ctx_)
        {
            std::cerr << "SDL_GL_CreateContext failed\n";
            SDL_DestroyWindow(win_);
            SDL_Quit();
            return 1;
        }

        glewInit();

        SDL_GetWindowSize(win_, &winW_, &winH_);
        renderer_.setupGL(winW_, winH_);

        setupImGui_();

        noise_.init(1337);

        // Startup config
        if (state_.autoLoadConfigOnStart)
        {
            if (ConfigKV::load(P_, cfgPath_))
                state_.configStatus = "Config chargee au demarrage: " + cfgPath_;
            else
                state_.configStatus = "Aucune config startup (ok): " + cfgPath_;
        }

        P_.clampSafety();
        rebuildAll_(true);

        mainLoop_();

        // Shutdown
        if (heightTex_)
            glDeleteTextures(1, &heightTex_);
        if (state_.autoSaveConfigOnExit)
            ConfigKV::save(P_, cfgPath_);

        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(ctx_);
        SDL_DestroyWindow(win_);
        SDL_Quit();

        return 0;
    }

    void App::setupImGui_()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(win_, ctx_);
        ImGui_ImplOpenGL2_Init();
        ImGui::StyleColorsDark();
    }

    void App::rebuildAll_(bool /*force*/)
    {
        P_.clampSafety();
        W_ = P_.gridW;
        H_ = P_.gridH;

        gen_.generateChunkGrid(chunkGrid_, W_, H_, P_);
        heightChunk00_ = (!chunkGrid_.heights.empty() ? chunkGrid_.heights[0] : std::vector<float>());

        HeightmapIO::buildChunkAtlasRGBA8(chunkGrid_, W_, H_, 2, heightRGBA_, &lastMinH_, &lastMaxH_, &atlasW_, &atlasH_);
        if (!heightRGBA_.empty())
        {
            HeightmapIO::ensureOrUpdateTextureRGBA8(heightTex_, atlasW_, atlasH_, heightRGBA_.data());
        }
    }

    void App::updateHeights_()
    {
        P_.clampSafety();
        gen_.generateChunkGrid(chunkGrid_, W_, H_, P_);
        heightChunk00_ = (!chunkGrid_.heights.empty() ? chunkGrid_.heights[0] : std::vector<float>());

        HeightmapIO::buildChunkAtlasRGBA8(chunkGrid_, W_, H_, 2, heightRGBA_, &lastMinH_, &lastMaxH_, &atlasW_, &atlasH_);
        if (!heightRGBA_.empty())
        {
            HeightmapIO::ensureOrUpdateTextureRGBA8(heightTex_, atlasW_, atlasH_, heightRGBA_.data());
        }
    }

    void App::handleEvents_()
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL2_ProcessEvent(&e);

            if (e.type == SDL_QUIT)
                quit_ = true;

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                winW_ = e.window.data1;
                winH_ = e.window.data2;
                renderer_.onResize(winW_, winH_);
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
            {
                dragging_ = true;
                lastX_ = e.button.x;
                lastY_ = e.button.y;
            }
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
            {
                dragging_ = false;
            }
            if (e.type == SDL_MOUSEMOTION && dragging_)
            {
                P_.camRotY += (e.motion.x - lastX_) * 0.4f;
                P_.camRotX += (e.motion.y - lastY_) * 0.4f;
                lastX_ = e.motion.x;
                lastY_ = e.motion.y;
            }
            if (e.type == SDL_MOUSEWHEEL)
            {
                P_.camZoom -= e.wheel.y * 10.f;
                P_.camZoom = std::max(20.0f, P_.camZoom);
            }
        }
    }

    void App::processExports_()
    {
        // Mono chunk (0,0)
        if (state_.requestExportPGM)
        {
            state_.requestExportPGM = false;
            std::string fname = HeightmapIO::makeTimestampedFilename("heightmap_chunk_r0_c0", "pgm");
            if (!heightChunk00_.empty() && HeightmapIO::exportPGM(heightChunk00_, W_, H_, fname))
                state_.lastExportPath = fname;
            else
                state_.lastExportPath = "Export failed";
        }

        if (state_.requestExportPNG)
        {
            state_.requestExportPNG = false;
            std::string fname = HeightmapIO::makeTimestampedFilename("heightmap_chunk_r0_c0", "png");
            if (!heightChunk00_.empty() && HeightmapIO::exportPNG(heightChunk00_, W_, H_, fname))
                state_.lastExportPath = fname;
            else
                state_.lastExportPath = "Export PNG failed";
        }

        // All chunks
        if (state_.requestExportAllChunksPNG)
        {
            state_.requestExportAllChunksPNG = false;
            std::string prefix = HeightmapIO::makeTimestampedFilename("heightmap_chunks", "tmp");
            if (prefix.size() >= 4)
                prefix = prefix.substr(0, prefix.size() - 4);
            std::string msg;
            HeightmapIO::exportAllChunksPNG(chunkGrid_, W_, H_, prefix, msg);
            state_.lastExportPath = msg + " | prefix: " + prefix;
        }
    }

    void App::mainLoop_()
    {
        while (!quit_)
        {
            handleEvents_();

            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            UI::drawControls(P_, W_, H_, state_, cfgPath_);
            UI::drawHeightmapWindow(P_, state_, (dune::TextureHandle)heightTex_, atlasW_, atlasH_, lastMinH_, lastMaxH_);

            bool doRebuild = state_.needRebuild;
            bool doUpdate = state_.needUpdate;

            state_.needRebuild = false;
            state_.needUpdate = false;

            if (doRebuild)
                rebuildAll_(false);
            else if (doUpdate)
                updateHeights_();

            processExports_();

            renderer_.beginFrame();
            renderer_.drawAllChunks(chunkGrid_, W_, H_, P_);

            ImGui::Render();
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(win_);
        }
    }

} // namespace dune