// src/TerrainGenerator.cpp
#include "TerrainGenerator.h"

#include <cmath>
#include <algorithm>

namespace dune {

TerrainGenerator::TerrainGenerator(const Noise& noise)
: noise_(noise)
{}

void TerrainGenerator::generateChunkGrid(ChunkGrid& grid, int W, int H, const Params& P) const
{
    grid.cols = std::max(1, P.chunkCols);
    grid.rows = std::max(1, P.chunkRows);
    grid.heights.resize((size_t)grid.cols * (size_t)grid.rows);

    const float stepX = P.terrainWidth;
    const float stepY = P.terrainLength;
    const float centerCols = 0.5f * (grid.cols - 1);
    const float centerRows = 0.5f * (grid.rows - 1);

    for(int r=0;r<grid.rows;++r){
        for(int c=0;c<grid.cols;++c){
            float offsetX = (c - centerCols) * stepX;
            float offsetY = (r - centerRows) * stepY;
            generateHeights(grid.heights[ChunkGrid::index(c,r,grid.cols)], W, H, P, offsetX, offsetY);
        }
    }
}

void TerrainGenerator::generateHeights(
    std::vector<float>& out, int W, int Hs, const Params& P,
    float chunkWorldOffsetX, float chunkWorldOffsetY) const
{
    out.resize((size_t)W*(size_t)Hs);

    float minH= 1e30f, maxH = -1e30f;

    const float pi = 3.14159265358979323846f;
    const float rotRad = P.rotationDeg * pi / 180.f;
    const float cosR = cos(rotRad), sinR = sin(rotRad);

    const float halfX = 0.5f * P.terrainWidth;
    const float halfY = 0.5f * P.terrainLength;

    for(int j=0;j<Hs;j++){
        float v = (Hs>1) ? (float)j / (Hs - 1) : 0.f;
        float localBaseY = (v - 0.5f) * (2.f * halfY);

        for(int i=0;i<W;i++){
            float u = (W>1) ? (float)i / (W - 1) : 0.f;
            float localBaseX = (u - 0.5f) * (2.f * halfX);

            float baseX = localBaseX + chunkWorldOffsetX;
            float baseY = localBaseY + chunkWorldOffsetY;

            float x0 = baseX * P.stretchX;
            float y0 = baseY * P.stretchY;

            float xr = x0*cosR - y0*sinR;
            float yr = x0*sinR + y0*cosR;

            xr += P.noiseOffsetX;
            yr += P.noiseOffsetY;

            float wx=0.f, wy=0.f;
            if(P.warpEnabled){
                wx = noise_.fbm(xr*P.warpFreq,      yr*P.warpFreq,      3,2.0f,0.5f)*P.warpAmp;
                wy = noise_.fbm((xr+100)*P.warpFreq,(yr+100)*P.warpFreq,3,2.0f,0.5f)*P.warpAmp;
            }

            float nx=(xr+wx)*P.noiseZoom;
            float ny=(yr+wy)*P.noiseZoom;

            float n = P.ridgedMode
                ? noise_.ridgedFBM(nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain)
                : noise_.fbm      (nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain);

            float z = (n * 0.25f) * P.amp;
            if(P.invertZ) z *= -1.f;

            out[(size_t)j*(size_t)W + (size_t)i] = z;
            minH = std::min(minH, z);
            maxH = std::max(maxH, z);
        }
    }

    // recentrage vertical (par chunk)
    float offset = -(minH+maxH)/4.f;
    for(float& v : out) v += offset;

    crestPostProcess(out, W, Hs, P);
}

void TerrainGenerator::crestPostProcess(std::vector<float>& H, int W, int Hs, const Params& P)
{
    const float smooth = P.crestSmoothing;
    const float sharp  = P.crestSharpen;
    const float radius = std::max(1.f, P.crestWidth);

    if(smooth <= 0.001f && sharp <= 0.001f) return;

    auto idx = [&](int x,int y){ return y*W + x; };
    std::vector<float> copy = H;

    for(int y=1; y<Hs-1; ++y){
        for(int x=1; x<W-1; ++x){
            float h  = copy[idx(x,y)];
            float n1 = copy[idx(x-1,y)];
            float n2 = copy[idx(x+1,y)];
            float n3 = copy[idx(x,y-1)];
            float n4 = copy[idx(x,y+1)];
            float maxN = std::max(std::max(n1,n2), std::max(n3,n4));
            bool isCrest = (h > maxN);
            if(!isCrest) continue;

            float sum=0; int count=0;
            int r = (int)radius;
            for(int dy=-r; dy<=r; dy++){
                for(int dx=-r; dx<=r; dx++){
                    if(x+dx < 0 || x+dx >= W) continue;
                    if(y+dy < 0 || y+dy >= Hs) continue;
                    sum += copy[idx(x+dx,y+dy)];
                    count++;
                }
            }
            float localAvg = sum / (float)std::max(1, count);

            float outH = h;
            if(smooth > 0.001f){
                float t = smooth * 0.5f;
                outH = outH*(1.f-t) + localAvg*t;
            }
            if(sharp > 0.001f){
                float sharpened = outH + (outH - localAvg) * sharp * 0.8f;
                outH = sharpened;
            }
            H[idx(x,y)] = outH;
        }
    }
}

} // namespace dune