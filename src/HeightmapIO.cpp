// src/HeightmapIO.cpp
#include "HeightmapIO.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstdio>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace dune {

void HeightmapIO::buildRGBA8(
    const std::vector<float>& heights, int W, int H,
    std::vector<unsigned char>& outRGBA,
    float* outMin, float* outMax)
{
    if((int)heights.size() != W*H){ outRGBA.clear(); return; }

    float mn = 1e30f, mx = -1e30f;
    for(float v : heights){ mn = std::min(mn, v); mx = std::max(mx, v); }

    if(outMin) *outMin = mn;
    if(outMax) *outMax = mx;

    float denom = (mx - mn);
    if(denom < 1e-9f) denom = 1.0f;

    outRGBA.resize((size_t)W*(size_t)H*4);
    for(int y=0; y<H; ++y){
        for(int x=0; x<W; ++x){
            float v = heights[y*W + x];
            float t = (v - mn) / denom;
            t = std::clamp(t, 0.0f, 1.0f);
            unsigned char g = (unsigned char)std::lround(t * 255.0f);
            size_t k = ((size_t)y*W + (size_t)x)*4;
            outRGBA[k+0]=g; outRGBA[k+1]=g; outRGBA[k+2]=g; outRGBA[k+3]=255;
        }
    }
}

void HeightmapIO::buildChunkAtlasRGBA8(
    const ChunkGrid& grid, int W, int H, int gapPx,
    std::vector<unsigned char>& outRGBA,
    float* outMin, float* outMax,
    int* outAtlasW, int* outAtlasH)
{
    if(grid.cols <= 0 || grid.rows <= 0 || grid.heights.empty()){
        outRGBA.clear();
        if(outAtlasW) *outAtlasW = 0;
        if(outAtlasH) *outAtlasH = 0;
        return;
    }

    float mn = 1e30f, mx = -1e30f;
    for(const auto& chunk : grid.heights){
        for(float v : chunk){ mn = std::min(mn, v); mx = std::max(mx, v); }
    }
    if(outMin) *outMin = mn;
    if(outMax) *outMax = mx;

    float denom = (mx - mn);
    if(denom < 1e-9f) denom = 1.0f;

    const int atlasW = grid.cols * W + (grid.cols - 1) * gapPx;
    const int atlasH = grid.rows * H + (grid.rows - 1) * gapPx;
    if(outAtlasW) *outAtlasW = atlasW;
    if(outAtlasH) *outAtlasH = atlasH;

    outRGBA.assign((size_t)atlasW * (size_t)atlasH * 4, 0);
    for(size_t i=0;i<outRGBA.size();i+=4){
        outRGBA[i+0]=20; outRGBA[i+1]=20; outRGBA[i+2]=20; outRGBA[i+3]=255;
    }

    for(int r = 0; r < grid.rows; ++r){
        for(int c = 0; c < grid.cols; ++c){
            const auto& chunk = grid.heights[ChunkGrid::index(c,r,grid.cols)];
            int ox = c * (W + gapPx);
            int oy = r * (H + gapPx);

            for(int y=0;y<H;++y){
                for(int x=0;x<W;++x){
                    float v = chunk[y*W+x];
                    float t = (v - mn) / denom;
                    t = std::clamp(t, 0.0f, 1.0f);
                    unsigned char g = (unsigned char)std::lround(t*255.0f);
                    size_t k = ((size_t)(oy+y)*atlasW + (size_t)(ox+x))*4;
                    outRGBA[k+0]=g; outRGBA[k+1]=g; outRGBA[k+2]=g; outRGBA[k+3]=255;
                }
            }
        }
    }
}

void HeightmapIO::ensureOrUpdateTextureRGBA8(GLuint& tex, int W, int H, const unsigned char* dataRGBA)
{
    if(tex == 0){
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataRGBA);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataRGBA);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::string HeightmapIO::makeTimestampedFilename(const char* prefix, const char* ext)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s_%04d%02d%02d_%02d%02d%02d.%s",
        prefix,
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        ext
    );
    return std::string(buf);
}

bool HeightmapIO::exportPGM(const std::vector<float>& heights, int W, int H, const std::string& filename)
{
    if((int)heights.size() != W*H) return false;

    float mn=1e30f,mx=-1e30f;
    for(float v:heights){ mn=std::min(mn,v); mx=std::max(mx,v); }
    float denom = (mx-mn);
    if(denom < 1e-9f) denom = 1.0f;

    std::ofstream f(filename, std::ios::binary);
    if(!f) return false;
    f << "P5\n" << W << " " << H << "\n255\n";

    std::vector<unsigned char> line((size_t)W);
    for(int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            float v = heights[y*W+x];
            float t = (v - mn) / denom;
            t = std::clamp(t,0.0f,1.0f);
            line[(size_t)x] = (unsigned char)std::lround(t*255.0f);
        }
        f.write((const char*)line.data(), (std::streamsize)line.size());
    }
    return true;
}

bool HeightmapIO::exportPNG(const std::vector<float>& heights, int W, int H, const std::string& filename)
{
    if((int)heights.size() != W*H) return false;

    float mn=1e30f,mx=-1e30f;
    for(float v:heights){ mn=std::min(mn,v); mx=std::max(mx,v); }
    float denom = (mx-mn);
    if(denom < 1e-9f) denom = 1.0f;

    std::vector<unsigned char> pixels((size_t)W*(size_t)H);
    for(int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            float v = heights[y*W+x];
            float t = (v - mn) / denom;
            t = std::clamp(t,0.0f,1.0f);
            pixels[(size_t)y*W+(size_t)x] = (unsigned char)std::lround(t*255.0f);
        }
    }
    int ok = stbi_write_png(filename.c_str(), W, H, 1, pixels.data(), W);
    return ok != 0;
}

bool HeightmapIO::exportAllChunksPNG(
    const ChunkGrid& grid, int W, int H,
    const std::string& basePrefix,
    std::string& outMessage)
{
    if(grid.heights.empty()){
        outMessage = "Aucun chunk a exporter";
        return false;
    }

    bool allOk=true;
    int okCount=0;
    int total = grid.cols * grid.rows;

    for(int r=0;r<grid.rows;++r){
        for(int c=0;c<grid.cols;++c){
            const auto& h = grid.heights[ChunkGrid::index(c,r,grid.cols)];
            std::string filename = basePrefix + "_r" + std::to_string(r) + "_c" + std::to_string(c) + ".png";
            if(exportPNG(h,W,H,filename)) okCount++;
            else allOk=false;
        }
    }

    outMessage = "Export PNG chunks: " + std::to_string(okCount) + "/" + std::to_string(total);
    return allOk;
}

} // namespace dune