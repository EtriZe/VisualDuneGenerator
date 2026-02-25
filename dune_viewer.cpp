// ========================================================================================
// TON CODE + AJOUTS :
//  - Preview Heightmap (nuances de gris) en haut à droite (texture OpenGL) mise à jour en temps réel
//  - Bouton "Export heightmap" (PGM binaire .pgm, sans dépendance externe)
//  - Paramètres largeur/longueur du terrain (world size), utilisés pour génération + rendu
//  - (bonus utile) gestion resize fenêtre pour viewport/projection
//  - [AJOUT] Chunks (rows/cols + gap visuel), preview atlas, export PNG par chunk
// ========================================================================================

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl2.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>
#include <cctype>
#include <ctime>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// =========================================================
// --- Perlin Noise / FBM ---
// =========================================================
static int p[512];

float fade(float t){ return t*t*t*(t*(t*6-15)+10); }
float lerp(float a,float b,float t){ return a + t*(b-a); }

float grad(int hash,float x,float y){
    int h=hash&7;
    float u=h<4?x:y;
    float v=h<4?y:x;
    return ((h&1)?-u:u)+((h&2)?-2.0f*v:2.0f*v);
}

float perlin(float x,float y){
    int X=(int)floor(x)&255;
    int Y=(int)floor(y)&255;
    x-=floor(x); y-=floor(y);
    float u=fade(x), v=fade(y);
    int A=p[X]+Y, B=p[X+1]+Y;

    return lerp(
        lerp(grad(p[A],x,y),     grad(p[B],x-1,y), u),
        lerp(grad(p[A+1],x,y-1), grad(p[B+1],x-1,y-1), u),
        v
    );
}

float fbm(float x,float y,int oct,float lac,float gain){
    float amp=0.5f,freq=1.f,sum=0;
    for(int i=0;i<oct;i++){
        sum+=amp*perlin(x*freq,y*freq);
        freq*=lac; amp*=gain;
    }
    return sum;
}

float ridged(float n){ return 1.f - fabsf(n); }

float ridgedFBM(float x,float y,int oct,float lac,float gain){
    float amp=0.5f,freq=1.f,sum=0;
    for(int i=0;i<oct;i++){
        float r=ridged(perlin(x*freq,y*freq));
        r*=r;
        sum+=r*amp;
        freq*=lac; amp*=gain;
    }
    return std::clamp(sum,0.f,1.f);
}

// =========================================================
// === Paramètres Dune ===
// =========================================================
struct Params {

    // --- Bruit principal ---
    int   octaves = 5;
    float lacunarity = 1.9f;
    float gain = 0.45f;
    float freq = 0.01f;
    float amp = 40.f;

    // --- Warp ---
    float warpAmp = 6.f;
    float warpFreq = 0.008f;

    // --- Anisotropie ---
    float stretchX = 1.0f;
    float stretchY = 1.0f;
    float rotationDeg = 0.f;

    // --- Rugosité fine ---
    float ridgeBias = 0.25f;
    float ridgePow = 1.5f;
    float ridgeGain = 0.5f;
    float ridgeLacunarity = 2.0f;
    int   ridgeOctaves = 4;
    float ridgeAttenuation = 0.6f;

    // --- Exploration ---
    float noiseOffsetX = 0.f;
    float noiseOffsetY = 0.f;
    float noiseZoom    = 1.0f;

    // --- Crop ---
    int   cropLeft   = 0;
    int   cropRight  = 0;
    int   cropTop    = 0;
    int   cropBottom = 0;

    // --- Résolution ---
    int   gridW = 128;
    int   gridH = 128;

    // --- Taille terrain (monde) PAR CHUNK ---
    float terrainWidth  = 128.0f;
    float terrainLength = 128.0f;

    // --- [AJOUT] Chunks ---
    int   chunkCols = 1;
    int   chunkRows = 1;
    float chunkGapVisual = 1.0f; // espace uniquement visuel entre chunks en 3D

    // --- Effets visuels ---
    bool  filled = false;
    bool  invertZ = false;
    bool  ridgedMode = true;
    bool  warpEnabled = true;

    // --- Caméra ---
    float camRotX = 55.f;
    float camRotY = 35.f;
    float camZoom = 280.f;
    float camPanX = 0.f;
    float camPanY = -30.f;

    // --- Smoothing + Sharpening des crêtes ---
    float crestSmoothing = 0.0f;
    float crestSharpen   = 0.0f;
    float crestWidth     = 1.0f;
};

// =========================================================
// === Chunk structures/helpers ============================
// =========================================================
struct ChunkGrid {
    std::vector<std::vector<float>> heights; // rows*cols
    int cols = 1;
    int rows = 1;
};

static inline int chunkIndex(int c, int r, int cols){
    return r * cols + c;
}

// =========================================================
// === Heightmap helpers (texture + export) ===
// =========================================================
static void buildHeightmapRGBA8(
    const std::vector<float>& heights, int W, int H,
    std::vector<unsigned char>& outRGBA,
    float* outMin = nullptr, float* outMax = nullptr
){
    if((int)heights.size() != W*H){
        outRGBA.clear();
        return;
    }

    float mn = 1e30f, mx = -1e30f;
    for(float v : heights){
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }

    if(outMin) *outMin = mn;
    if(outMax) *outMax = mx;

    float denom = (mx - mn);
    if(denom < 1e-9f) denom = 1.0f;

    outRGBA.resize((size_t)W * (size_t)H * 4);

    for(int y=0; y<H; ++y){
        for(int x=0; x<W; ++x){
            float v = heights[y*W + x];
            float t = (v - mn) / denom;
            t = std::clamp(t, 0.0f, 1.0f);
            unsigned char g = (unsigned char)std::lround(t * 255.0f);
            size_t k = ((size_t)y*W + (size_t)x) * 4;
            outRGBA[k+0] = g;
            outRGBA[k+1] = g;
            outRGBA[k+2] = g;
            outRGBA[k+3] = 255;
        }
    }
}

static void ensureOrUpdateTextureRGBA8(GLuint& tex, int W, int H, const unsigned char* dataRGBA){
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

static std::string makeTimestampedFilename(const char* prefix, const char* ext){
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
    std::snprintf(
        buf, sizeof(buf),
        "%s_%04d%02d%02d_%02d%02d%02d.%s",
        prefix,
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        ext
    );
    return std::string(buf);
}

static bool exportHeightmapPGM(const std::vector<float>& heights, int W, int H, const std::string& filename){
    if((int)heights.size() != W*H) return false;

    float mn = 1e30f, mx = -1e30f;
    for(float v : heights){
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }
    float denom = (mx - mn);
    if(denom < 1e-9f) denom = 1.0f;

    std::ofstream f(filename, std::ios::binary);
    if(!f) return false;

    f << "P5\n" << W << " " << H << "\n255\n";

    std::vector<unsigned char> line((size_t)W);
    for(int y=0; y<H; ++y){
        for(int x=0; x<W; ++x){
            float v = heights[y*W + x];
            float t = (v - mn) / denom;
            t = std::clamp(t, 0.0f, 1.0f);
            line[(size_t)x] = (unsigned char)std::lround(t * 255.0f);
        }
        f.write((const char*)line.data(), (std::streamsize)line.size());
    }
    return true;
}

static bool exportHeightmapPNG(const std::vector<float>& heights, int W, int H, const std::string& filename){
    if((int)heights.size() != W*H) return false;

    float mn = 1e30f, mx = -1e30f;
    for(float v : heights){
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }

    float denom = (mx - mn);
    if(denom < 1e-9f) denom = 1.0f;

    std::vector<unsigned char> pixels((size_t)W * (size_t)H);

    for(int y=0; y<H; ++y){
        for(int x=0; x<W; ++x){
            float v = heights[y*W + x];
            float t = (v - mn) / denom;
            t = std::clamp(t, 0.0f, 1.0f);
            pixels[(size_t)y*W + (size_t)x] = (unsigned char)std::lround(t * 255.0f);
        }
    }

    int ok = stbi_write_png(filename.c_str(), W, H, 1, pixels.data(), W);
    return ok != 0;
}

// Atlas preview de tous les chunks (nuances de gris)
static void buildChunkAtlasRGBA8(
    const ChunkGrid& grid, int W, int H,
    int gapPx,
    std::vector<unsigned char>& outRGBA,
    float* outMin = nullptr,
    float* outMax = nullptr,
    int* outAtlasW = nullptr,
    int* outAtlasH = nullptr
){
    if(grid.cols <= 0 || grid.rows <= 0 || grid.heights.empty()){
        outRGBA.clear();
        if(outAtlasW) *outAtlasW = 0;
        if(outAtlasH) *outAtlasH = 0;
        return;
    }

    float mn = 1e30f, mx = -1e30f;
    for(const auto& chunk : grid.heights){
        for(float v : chunk){
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
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

    for(size_t i = 0; i < outRGBA.size(); i += 4){
        outRGBA[i+0] = 20;
        outRGBA[i+1] = 20;
        outRGBA[i+2] = 20;
        outRGBA[i+3] = 255;
    }

    for(int r = 0; r < grid.rows; ++r){
        for(int c = 0; c < grid.cols; ++c){
            const auto& chunk = grid.heights[chunkIndex(c, r, grid.cols)];
            int ox = c * (W + gapPx);
            int oy = r * (H + gapPx);

            for(int y = 0; y < H; ++y){
                for(int x = 0; x < W; ++x){
                    float v = chunk[y*W + x];
                    float t = (v - mn) / denom;
                    t = std::clamp(t, 0.0f, 1.0f);
                    unsigned char g = (unsigned char)std::lround(t * 255.0f);

                    size_t k = ((size_t)(oy + y) * atlasW + (size_t)(ox + x)) * 4;
                    outRGBA[k+0] = g;
                    outRGBA[k+1] = g;
                    outRGBA[k+2] = g;
                    outRGBA[k+3] = 255;
                }
            }
        }
    }
}

static bool exportAllChunksPNG(
    const ChunkGrid& grid, int W, int H,
    const std::string& basePrefix,
    std::string& outMessage
){
    if(grid.heights.empty()){
        outMessage = "Aucun chunk a exporter";
        return false;
    }

    bool allOk = true;
    int okCount = 0;
    int total = grid.cols * grid.rows;

    for(int r = 0; r < grid.rows; ++r){
        for(int c = 0; c < grid.cols; ++c){
            const auto& h = grid.heights[chunkIndex(c, r, grid.cols)];
            std::string filename = basePrefix + "_r" + std::to_string(r) + "_c" + std::to_string(c) + ".png";
            if(exportHeightmapPNG(h, W, H, filename)) okCount++;
            else allOk = false;
        }
    }

    outMessage = "Export PNG chunks: " + std::to_string(okCount) + "/" + std::to_string(total);
    return allOk;
}

// =========================================================
// === generateHeights (post-process crêtes + offset chunk) ==
// =========================================================
void generateHeights(std::vector<float>& H,int W,int Hs,const Params& P, float chunkWorldOffsetX, float chunkWorldOffsetY){

    H.resize(W*Hs);
    float minH=9999,maxH=-9999;

    const float rotRad = P.rotationDeg * (float)M_PI / 180.f;
    const float cosR = cos(rotRad), sinR = sin(rotRad);

    const float halfX = 0.5f * P.terrainWidth;
    const float halfY = 0.5f * P.terrainLength;

    for(int j=0;j<Hs;j++){
        float v = (Hs>1) ? (float)j / (Hs - 1) : 0.f;
        float localBaseY = (v - 0.5f) * (2.f * halfY);

        for(int i=0;i<W;i++){
            float u = (W>1) ? (float)i / (W - 1) : 0.f;
            float localBaseX = (u - 0.5f) * (2.f * halfX);

            // Offset monde du chunk (pour continuité entre chunks)
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
                wx=fbm(xr*P.warpFreq,      yr*P.warpFreq,      3,2.0f,0.5f)*P.warpAmp;
                wy=fbm((xr+100)*P.warpFreq,(yr+100)*P.warpFreq,3,2.0f,0.5f)*P.warpAmp;
            }

            float nx=(xr+wx)*P.noiseZoom;
            float ny=(yr+wy)*P.noiseZoom;

            float n = P.ridgedMode ?
                ridgedFBM(nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain)
                :
                fbm      (nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain);

            float z = (n * 0.25f) * P.amp;
            if(P.invertZ) z *= -1.f;

            H[j*W+i]=z;
            if(z<minH)minH=z;
            if(z>maxH)maxH=z;
        }
    }

    // recentrage vertical (par chunk)
    float offset = -(minH+maxH)/4.f;
    for(float& v : H) v += offset;

    // === CREST POST-PROCESSING ===
    auto idx = [&](int x,int y){ return y*W + x; };
    std::vector<float> copy = H;

    float smooth = P.crestSmoothing;
    float sharp  = P.crestSharpen;
    float radius = std::max(1.f, P.crestWidth);

    if(smooth > 0.001f || sharp > 0.001f){
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

                if(smooth > 0.001f){
                    float t = smooth * 0.5f;
                    H[idx(x,y)] = h*(1.f-t) + localAvg*t;
                }

                if(sharp > 0.001f){
                    float sharpened = h + (h - localAvg) * sharp * 0.8f;
                    H[idx(x,y)] = sharpened;
                }
            }
        }
    }
}

// Compat mono-chunk
static inline void generateHeights(std::vector<float>& H,int W,int Hs,const Params& P){
    generateHeights(H, W, Hs, P, 0.0f, 0.0f);
}

// Génération de tous les chunks
static void generateChunkGrid(ChunkGrid& grid, int W, int H, const Params& P){
    grid.cols = std::max(1, P.chunkCols);
    grid.rows = std::max(1, P.chunkRows);
    grid.heights.resize((size_t)grid.cols * (size_t)grid.rows);

    const float stepX = P.terrainWidth;   // pas logique en monde (sans gap visuel)
    const float stepY = P.terrainLength;

    const float centerCols = 0.5f * (grid.cols - 1);
    const float centerRows = 0.5f * (grid.rows - 1);

    for(int r = 0; r < grid.rows; ++r){
        for(int c = 0; c < grid.cols; ++c){
            float offsetX = (c - centerCols) * stepX;
            float offsetY = (r - centerRows) * stepY;
            generateHeights(grid.heights[chunkIndex(c, r, grid.cols)], W, H, P, offsetX, offsetY);
        }
    }
}

// =========================================================
// === Mesh drawing ===
// =========================================================
void drawMesh(const std::vector<float>& H,int W,int Hs,const Params& P, float visualOffsetX, float visualOffsetY){
    glPushMatrix();
    glTranslatef(P.camPanX, P.camPanY, -P.camZoom);
    glRotatef(P.camRotX,1,0,0);
    glRotatef(P.camRotY,0,1,0);

    // Placement visuel du chunk dans la grille
    glTranslatef(visualOffsetX, 0.0f, visualOffsetY);

    glColor3f(0.9f,0.8f,0.6f);
    glPolygonMode(GL_FRONT_AND_BACK, P.filled ? GL_FILL : GL_LINE);

    int iStart = std::clamp(P.cropLeft,  0, std::max(0, W-1));
    int iEnd   = std::clamp(W-1-P.cropRight, 0, W-1);
    int jStart = std::clamp(P.cropTop,   0, std::max(0, Hs-2));
    int jEnd   = std::clamp(Hs-2-P.cropBottom, 0, Hs-2);

    if(iStart >= iEnd || jStart > jEnd){
        glPopMatrix();
        return;
    }

    const float halfX = 0.5f * P.terrainWidth;
    const float halfY = 0.5f * P.terrainLength;

    for(int j=jStart; j<=jEnd; ++j){
        float v0 = (float)j     / (Hs-1);
        float v1 = (float)(j+1) / (Hs-1);
        float y0 = (v0 - 0.5f) * (2.f * halfY);
        float y1 = (v1 - 0.5f) * (2.f * halfY);

        glBegin(GL_TRIANGLE_STRIP);
        for(int i=iStart; i<=iEnd; ++i){
            float u = (float)i / (W-1);
            float x = (u - 0.5f) * (2.f * halfX);

            float z1=H[j*W+i];
            float z2=H[(j+1)*W+i];

            glVertex3f(x, z1, y0);
            glVertex3f(x, z2, y1);
        }
        glEnd();
    }
    glPopMatrix();
}

// Compat mono-chunk
static inline void drawMesh(const std::vector<float>& H,int W,int Hs,const Params& P){
    drawMesh(H, W, Hs, P, 0.0f, 0.0f);
}

// =========================================================
// === Config file (key=value) =============================
// =========================================================
static std::string trimCopy(const std::string& s){
    size_t a = 0, b = s.size();
    while(a < b && std::isspace((unsigned char)s[a])) ++a;
    while(b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b - a);
}

static bool parseIntSafe(const std::string& s, int& out){
    try{
        std::string t = trimCopy(s);
        size_t pos = 0;
        int v = std::stoi(t, &pos);
        if(pos != t.size()) return false;
        out = v;
        return true;
    }catch(...){ return false; }
}

static bool parseFloatSafe(const std::string& s, float& out){
    try{
        std::string t = trimCopy(s);
        size_t pos = 0;
        float v = std::stof(t, &pos);
        if(pos != t.size()) return false;
        out = v;
        return true;
    }catch(...){ return false; }
}

static bool parseBoolSafe(const std::string& s, bool& out){
    std::string v = trimCopy(s);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    if(v == "1" || v == "true"  || v == "yes" || v == "on"){ out = true;  return true; }
    if(v == "0" || v == "false" || v == "no"  || v == "off"){ out = false; return true; }
    return false;
}

static bool saveConfigFile(const Params& P, const std::string& path){
    std::ofstream f(path);
    if(!f) return false;

    f << "# Dune Studio config\n";
    f << "version=2\n";

    // Bruit principal
    f << "octaves=" << P.octaves << "\n";
    f << "lacunarity=" << P.lacunarity << "\n";
    f << "gain=" << P.gain << "\n";
    f << "freq=" << P.freq << "\n";
    f << "amp=" << P.amp << "\n";

    // Warp
    f << "warpAmp=" << P.warpAmp << "\n";
    f << "warpFreq=" << P.warpFreq << "\n";

    // Anisotropie
    f << "stretchX=" << P.stretchX << "\n";
    f << "stretchY=" << P.stretchY << "\n";
    f << "rotationDeg=" << P.rotationDeg << "\n";

    // Ridged fine
    f << "ridgeBias=" << P.ridgeBias << "\n";
    f << "ridgePow=" << P.ridgePow << "\n";
    f << "ridgeGain=" << P.ridgeGain << "\n";
    f << "ridgeLacunarity=" << P.ridgeLacunarity << "\n";
    f << "ridgeOctaves=" << P.ridgeOctaves << "\n";
    f << "ridgeAttenuation=" << P.ridgeAttenuation << "\n";

    // Exploration
    f << "noiseOffsetX=" << P.noiseOffsetX << "\n";
    f << "noiseOffsetY=" << P.noiseOffsetY << "\n";
    f << "noiseZoom=" << P.noiseZoom << "\n";

    // Crop
    f << "cropLeft=" << P.cropLeft << "\n";
    f << "cropRight=" << P.cropRight << "\n";
    f << "cropTop=" << P.cropTop << "\n";
    f << "cropBottom=" << P.cropBottom << "\n";

    // Résolution
    f << "gridW=" << P.gridW << "\n";
    f << "gridH=" << P.gridH << "\n";

    // Taille terrain
    f << "terrainWidth=" << P.terrainWidth << "\n";
    f << "terrainLength=" << P.terrainLength << "\n";

    // Chunks
    f << "chunkCols=" << P.chunkCols << "\n";
    f << "chunkRows=" << P.chunkRows << "\n";
    f << "chunkGapVisual=" << P.chunkGapVisual << "\n";

    // Visual flags
    f << "filled=" << (P.filled ? 1 : 0) << "\n";
    f << "invertZ=" << (P.invertZ ? 1 : 0) << "\n";
    f << "ridgedMode=" << (P.ridgedMode ? 1 : 0) << "\n";
    f << "warpEnabled=" << (P.warpEnabled ? 1 : 0) << "\n";

    // Caméra
    f << "camRotX=" << P.camRotX << "\n";
    f << "camRotY=" << P.camRotY << "\n";
    f << "camZoom=" << P.camZoom << "\n";
    f << "camPanX=" << P.camPanX << "\n";
    f << "camPanY=" << P.camPanY << "\n";

    // Crêtes
    f << "crestSmoothing=" << P.crestSmoothing << "\n";
    f << "crestSharpen=" << P.crestSharpen << "\n";
    f << "crestWidth=" << P.crestWidth << "\n";

    return true;
}

static bool loadConfigFile(Params& P, const std::string& path){
    std::ifstream f(path);
    if(!f) return false;

    std::string line;
    while(std::getline(f, line)){
        line = trimCopy(line);
        if(line.empty()) continue;
        if(line[0] == '#') continue;

        size_t eq = line.find('=');
        if(eq == std::string::npos) continue;

        std::string key = trimCopy(line.substr(0, eq));
        std::string val = trimCopy(line.substr(eq + 1));

        int iv; float fv; bool bv;

        // Bruit principal
        if(key == "octaves" && parseIntSafe(val, iv)) P.octaves = iv;
        else if(key == "lacunarity" && parseFloatSafe(val, fv)) P.lacunarity = fv;
        else if(key == "gain" && parseFloatSafe(val, fv)) P.gain = fv;
        else if(key == "freq" && parseFloatSafe(val, fv)) P.freq = fv;
        else if(key == "amp" && parseFloatSafe(val, fv)) P.amp = fv;

        // Warp
        else if(key == "warpAmp" && parseFloatSafe(val, fv)) P.warpAmp = fv;
        else if(key == "warpFreq" && parseFloatSafe(val, fv)) P.warpFreq = fv;

        // Anisotropie
        else if(key == "stretchX" && parseFloatSafe(val, fv)) P.stretchX = fv;
        else if(key == "stretchY" && parseFloatSafe(val, fv)) P.stretchY = fv;
        else if(key == "rotationDeg" && parseFloatSafe(val, fv)) P.rotationDeg = fv;

        // Ridged
        else if(key == "ridgeBias" && parseFloatSafe(val, fv)) P.ridgeBias = fv;
        else if(key == "ridgePow" && parseFloatSafe(val, fv)) P.ridgePow = fv;
        else if(key == "ridgeGain" && parseFloatSafe(val, fv)) P.ridgeGain = fv;
        else if(key == "ridgeLacunarity" && parseFloatSafe(val, fv)) P.ridgeLacunarity = fv;
        else if(key == "ridgeOctaves" && parseIntSafe(val, iv)) P.ridgeOctaves = iv;
        else if(key == "ridgeAttenuation" && parseFloatSafe(val, fv)) P.ridgeAttenuation = fv;

        // Exploration
        else if(key == "noiseOffsetX" && parseFloatSafe(val, fv)) P.noiseOffsetX = fv;
        else if(key == "noiseOffsetY" && parseFloatSafe(val, fv)) P.noiseOffsetY = fv;
        else if(key == "noiseZoom" && parseFloatSafe(val, fv)) P.noiseZoom = fv;

        // Crop
        else if(key == "cropLeft" && parseIntSafe(val, iv)) P.cropLeft = iv;
        else if(key == "cropRight" && parseIntSafe(val, iv)) P.cropRight = iv;
        else if(key == "cropTop" && parseIntSafe(val, iv)) P.cropTop = iv;
        else if(key == "cropBottom" && parseIntSafe(val, iv)) P.cropBottom = iv;

        // Résolution
        else if(key == "gridW" && parseIntSafe(val, iv)) P.gridW = iv;
        else if(key == "gridH" && parseIntSafe(val, iv)) P.gridH = iv;

        // Taille terrain
        else if(key == "terrainWidth" && parseFloatSafe(val, fv)) P.terrainWidth = fv;
        else if(key == "terrainLength" && parseFloatSafe(val, fv)) P.terrainLength = fv;

        // Chunks
        else if(key == "chunkCols" && parseIntSafe(val, iv)) P.chunkCols = iv;
        else if(key == "chunkRows" && parseIntSafe(val, iv)) P.chunkRows = iv;
        else if(key == "chunkGapVisual" && parseFloatSafe(val, fv)) P.chunkGapVisual = fv;

        // Flags
        else if(key == "filled" && parseBoolSafe(val, bv)) P.filled = bv;
        else if(key == "invertZ" && parseBoolSafe(val, bv)) P.invertZ = bv;
        else if(key == "ridgedMode" && parseBoolSafe(val, bv)) P.ridgedMode = bv;
        else if(key == "warpEnabled" && parseBoolSafe(val, bv)) P.warpEnabled = bv;

        // Caméra
        else if(key == "camRotX" && parseFloatSafe(val, fv)) P.camRotX = fv;
        else if(key == "camRotY" && parseFloatSafe(val, fv)) P.camRotY = fv;
        else if(key == "camZoom" && parseFloatSafe(val, fv)) P.camZoom = fv;
        else if(key == "camPanX" && parseFloatSafe(val, fv)) P.camPanX = fv;
        else if(key == "camPanY" && parseFloatSafe(val, fv)) P.camPanY = fv;

        // Crêtes
        else if(key == "crestSmoothing" && parseFloatSafe(val, fv)) P.crestSmoothing = fv;
        else if(key == "crestSharpen" && parseFloatSafe(val, fv)) P.crestSharpen = fv;
        else if(key == "crestWidth" && parseFloatSafe(val, fv)) P.crestWidth = fv;
    }

    // Clamp minimums de sécurité
    P.gridW = std::max(2, P.gridW);
    P.gridH = std::max(2, P.gridH);
    P.terrainWidth  = std::max(1.0f, P.terrainWidth);
    P.terrainLength = std::max(1.0f, P.terrainLength);
    P.noiseZoom = std::max(0.0001f, P.noiseZoom);
    P.crestWidth = std::max(1.0f, P.crestWidth);
    P.chunkCols = std::max(1, P.chunkCols);
    P.chunkRows = std::max(1, P.chunkRows);
    P.chunkGapVisual = std::max(0.0f, P.chunkGapVisual);

    return true;
}

// =========================================================
// === MAIN ===
// =========================================================
int main(){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win=SDL_CreateWindow("Dune Studio — Advanced Viewer",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1400,950,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GLContext ctx=SDL_GL_CreateContext(win);
    glewInit();

    int winW = 1400, winH = 950;
    SDL_GetWindowSize(win, &winW, &winH);

    glViewport(0,0,winW,winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)winW/(double)winH, 0.1, 4000.0);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f,0.25f,0.3f,1.f);

    // imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(win,ctx);
    ImGui_ImplOpenGL2_Init();
    ImGui::StyleColorsDark();

    // init permutation
    std::vector<int> perm(256);
    std::mt19937 rng(1337);
    for(int i=0;i<256;i++) perm[i]=i;
    std::shuffle(perm.begin(),perm.end(),rng);
    for(int i=0;i<512;i++) p[i]=perm[i&255];

    // params
    Params P;

    const std::string kDefaultConfigPath = "dune_last_config.cfg";
    bool autoLoadConfigOnStart = true;
    bool autoSaveConfigOnExit  = true;
    std::string configStatus;

    if(autoLoadConfigOnStart){
        if(loadConfigFile(P, kDefaultConfigPath)){
            configStatus = "Config chargee au demarrage: " + kDefaultConfigPath;
        }else{
            configStatus = "Aucune config startup (ok): " + kDefaultConfigPath;
        }
    }

    int W = P.gridW;
    int H = P.gridH;

    // Données chunks
    ChunkGrid chunkGrid;
    generateChunkGrid(chunkGrid, W, H, P);

    // Pour compat export mono-chunk (premier chunk)
    std::vector<float> height = (!chunkGrid.heights.empty() ? chunkGrid.heights[0] : std::vector<float>());

    // Texture preview atlas chunks
    GLuint heightTex = 0;
    std::vector<unsigned char> heightRGBA;
    float lastMinH = 0.0f, lastMaxH = 0.0f;
    int previewAtlasW = W, previewAtlasH = H;
    buildChunkAtlasRGBA8(chunkGrid, W, H, 2, heightRGBA, &lastMinH, &lastMaxH, &previewAtlasW, &previewAtlasH);
    if(!heightRGBA.empty()){
        ensureOrUpdateTextureRGBA8(heightTex, previewAtlasW, previewAtlasH, heightRGBA.data());
    }

    bool quit=false, needUpdate=false, needRebuild=false;
    bool dragging=false;
    int lastX=0,lastY=0;

    bool requestExport = false;
    bool requestExportPNG = false;
    bool requestExportAllChunksPNG = false;
    std::string lastExportPath;

    while(!quit){

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            ImGui_ImplSDL2_ProcessEvent(&e);
            if(e.type==SDL_QUIT) quit=true;

            if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED){
                winW = e.window.data1;
                winH = e.window.data2;
                glViewport(0,0,winW,winH);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                gluPerspective(60.0, (double)winW/(double)winH, 0.1, 4000.0);
                glMatrixMode(GL_MODELVIEW);
            }

            if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_RIGHT){
                dragging=true; lastX=e.button.x; lastY=e.button.y;
            }
            if(e.type==SDL_MOUSEBUTTONUP && e.button.button==SDL_BUTTON_RIGHT){
                dragging=false;
            }
            if(e.type==SDL_MOUSEMOTION && dragging){
                P.camRotY += (e.motion.x-lastX)*0.4f;
                P.camRotX += (e.motion.y-lastY)*0.4f;
                lastX=e.motion.x; lastY=e.motion.y;
            }
            if(e.type==SDL_MOUSEWHEEL){
                P.camZoom -= e.wheel.y*10.f;
                P.camZoom = std::max(20.0f, P.camZoom);
            }
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // =========================
        // Controls window
        // =========================
        ImGui::Begin("Dune Studio Controls");

        if(ImGui::CollapsingHeader("Bruit principal", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderInt ("Octaves",&P.octaves,1,8);
            needUpdate |= ImGui::SliderFloat("Freq",&P.freq,0.002f,0.03f);
            needUpdate |= ImGui::SliderFloat("Lacunarity",&P.lacunarity,1.0f,3.0f);
            needUpdate |= ImGui::SliderFloat("Gain",&P.gain,0.1f,0.9f);
            needUpdate |= ImGui::SliderFloat("Amplitude",&P.amp,5.f,100.f);
        }

        if(ImGui::CollapsingHeader("Warp / Déformation")){
            needUpdate |= ImGui::Checkbox("Warp Enabled",&P.warpEnabled);
            needUpdate |= ImGui::SliderFloat("Warp Freq",&P.warpFreq,0.001f,0.02f);
            needUpdate |= ImGui::SliderFloat("Warp Amp",&P.warpAmp,0.f,20.f);
        }

        if(ImGui::CollapsingHeader("Anisotropie / Orientation")){
            needUpdate |= ImGui::SliderFloat("Stretch X",&P.stretchX,0.5f,2.f);
            needUpdate |= ImGui::SliderFloat("Stretch Y",&P.stretchY,0.5f,2.f);
            needUpdate |= ImGui::SliderFloat("Rotation",&P.rotationDeg,-90.f,90.f);
        }

        if(ImGui::CollapsingHeader("Rugosité fine (Ridged)")){
            needUpdate |= ImGui::Checkbox("Ridged Mode",&P.ridgedMode);
            needUpdate |= ImGui::SliderInt  ("Ridge Octaves",&P.ridgeOctaves,1,6);
            needUpdate |= ImGui::SliderFloat("Ridge Lacunarity",&P.ridgeLacunarity,1.0f,3.0f);
            needUpdate |= ImGui::SliderFloat("Ridge Gain",&P.ridgeGain,0.2f,0.9f);
            needUpdate |= ImGui::SliderFloat("Ridge Bias",&P.ridgeBias,0.f,0.5f);
            needUpdate |= ImGui::SliderFloat("Ridge Pow",&P.ridgePow,0.5f,3.f);
            needUpdate |= ImGui::SliderFloat("Ridge Atten",&P.ridgeAttenuation,0.2f,1.f);
        }

        if(ImGui::CollapsingHeader("Affinage des Crêtes", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderFloat("Crest Smoothing", &P.crestSmoothing, 0.0f, 1.0f);
            needUpdate |= ImGui::SliderFloat("Crest Sharpen",   &P.crestSharpen,   0.0f, 1.0f);
            needUpdate |= ImGui::SliderFloat("Crest Width",     &P.crestWidth,     1.0f, 4.0f);
        }

        if(ImGui::CollapsingHeader("Exploration du bruit", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderFloat("Offset X",&P.noiseOffsetX,-1000.f,1000.f);
            needUpdate |= ImGui::SliderFloat("Offset Y",&P.noiseOffsetY,-1000.f,1000.f);
            needUpdate |= ImGui::SliderFloat("Zoom Bruit",&P.noiseZoom,0.1f,5.0f);
        }

        if(ImGui::CollapsingHeader("Terrain (dimensions world)", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderFloat("Largeur",  &P.terrainWidth,  16.0f, 2000.0f);
            needUpdate |= ImGui::SliderFloat("Longueur", &P.terrainLength, 16.0f, 2000.0f);
        }

        // ===== AJOUT CHUNKS =====
        if(ImGui::CollapsingHeader("Chunks", ImGuiTreeNodeFlags_DefaultOpen)){
            bool c1 = ImGui::SliderInt("Chunk Cols", &P.chunkCols, 1, 16);
            bool c2 = ImGui::SliderInt("Chunk Rows", &P.chunkRows, 1, 16);
            bool c3 = ImGui::SliderFloat("Chunk Gap (visual)", &P.chunkGapVisual, 0.0f, 50.0f);
            if(c1 || c2 || c3) needUpdate = true;

            ImGui::Text("Total chunks: %d", std::max(1, P.chunkCols) * std::max(1, P.chunkRows));

            if(ImGui::Button("Export all chunks PNG")){
                requestExportAllChunksPNG = true;
            }
        }

        if(ImGui::CollapsingHeader("Rognage")){
            int maxW = std::max(0, W-1);
            int maxH = std::max(0, H-2);

            needUpdate |= ImGui::SliderInt("Left",  &P.cropLeft,   0, maxW);
            needUpdate |= ImGui::SliderInt("Right", &P.cropRight,  0, maxW);
            needUpdate |= ImGui::SliderInt("Top",   &P.cropTop,    0, maxH);
            needUpdate |= ImGui::SliderInt("Bottom",&P.cropBottom, 0, maxH);
        }

        if(ImGui::CollapsingHeader("Résolution")){
            bool r1 = ImGui::SliderInt("Verts X",&P.gridW, 16, 1024);
            bool r2 = ImGui::SliderInt("Verts Y",&P.gridH, 16, 1024);
            if(r1 || r2) needRebuild = true;
        }

        if(ImGui::CollapsingHeader("Caméra")){
            ImGui::SliderFloat("Rot X",&P.camRotX,-90.f,90.f);
            ImGui::SliderFloat("Rot Y",&P.camRotY,-180.f,180.f);
            ImGui::SliderFloat("Zoom",&P.camZoom,50.f,5000.f);
            ImGui::SliderFloat("Pan X",&P.camPanX,-2000.f,2000.f);
            ImGui::SliderFloat("Pan Y",&P.camPanY,-2000.f,2000.f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Wireframe / Fill",&P.filled);
        needUpdate |= ImGui::Checkbox("Invert Z",&P.invertZ);

        if(ImGui::Button("Regenerate")) needUpdate=true;

        ImGui::SameLine();
        if(ImGui::Button("Reset")){
            P = Params();
            needRebuild = true;
            needUpdate = true;
        }

        ImGui::Separator();
        ImGui::Text("Configuration");

        if(ImGui::Button("Save config")){
            if(saveConfigFile(P, kDefaultConfigPath)){
                configStatus = "Config sauvegardee: " + kDefaultConfigPath;
            }else{
                configStatus = "Erreur sauvegarde config";
            }
        }

        ImGui::SameLine();

        if(ImGui::Button("Load config")){
            if(loadConfigFile(P, kDefaultConfigPath)){
                configStatus = "Config chargee: " + kDefaultConfigPath;
                needRebuild = true;
                needUpdate  = true;
            }else{
                configStatus = "Erreur chargement config (fichier absent ?)";
            }
        }

        ImGui::Checkbox("Auto-load startup config", &autoLoadConfigOnStart);
        ImGui::Checkbox("Auto-save on exit", &autoSaveConfigOnExit);

        ImGui::TextWrapped("Fichier: %s", kDefaultConfigPath.c_str());
        if(!configStatus.empty()){
            ImGui::TextWrapped("%s", configStatus.c_str());
        }

        ImGui::End();

        // =========================
        // Heightmap window (top-right)
        // =========================
        {
            ImGuiIO& io = ImGui::GetIO();
            const float margin = 12.0f;
            const ImVec2 wndSize(420.0f, 380.0f);

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - wndSize.x - margin, margin), ImGuiCond_Always);
            ImGui::SetNextWindowSize(wndSize, ImGuiCond_Always);

            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("Heightmap", nullptr, flags);

            // Exports mono chunk (chunk 0,0)
            if(ImGui::Button("Export heightmap (PGM)")){
                requestExport = true;
            }

            ImGui::SameLine();

            if(ImGui::Button("Export heightmap (PNG)")){
                requestExportPNG = true;
            }

            ImGui::SameLine();
            ImGui::Text("min %.2f  max %.2f", lastMinH, lastMaxH);

            ImGui::Text("Preview: atlas %dx%d chunks", std::max(1,P.chunkCols), std::max(1,P.chunkRows));

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float imgW = avail.x;
            float imgH = avail.y;

            float ar = (previewAtlasH > 0) ? (float)previewAtlasW / (float)previewAtlasH : 1.0f;
            if(imgH > 0.0f && imgW > 0.0f){
                float targetW = imgW;
                float targetH = imgW / ar;
                if(targetH > imgH){
                    targetH = imgH;
                    targetW = imgH * ar;
                }
                if(heightTex != 0){
                    ImGui::Image((ImTextureID)(intptr_t)heightTex, ImVec2(targetW, targetH));
                }else{
                    ImGui::Text("No texture");
                }
            }

            if(!lastExportPath.empty()){
                ImGui::Separator();
                ImGui::TextWrapped("Dernier export: %s", lastExportPath.c_str());
            }

            ImGui::End();
        }

        // =========================
        // Rebuild / Update
        // =========================
        bool heightChanged = false;

        if(needRebuild){
            W = P.gridW;
            H = P.gridH;
            generateChunkGrid(chunkGrid, W, H, P);
            needRebuild=false;
            heightChanged = true;
        }
        else if(needUpdate){
            generateChunkGrid(chunkGrid, W, H, P);
            heightChanged = true;
        }
        needUpdate=false;

        // chunk 0,0 -> compat export mono-chunk
        if(!chunkGrid.heights.empty()){
            height = chunkGrid.heights[0];
        }else{
            height.clear();
        }

        // Mise à jour texture preview atlas si nécessaire
        if(heightChanged){
            buildChunkAtlasRGBA8(chunkGrid, W, H, 2, heightRGBA, &lastMinH, &lastMaxH, &previewAtlasW, &previewAtlasH);
            if(!heightRGBA.empty()){
                ensureOrUpdateTextureRGBA8(heightTex, previewAtlasW, previewAtlasH, heightRGBA.data());
            }
        }

        // Export mono-chunk (0,0)
        if(requestExport){
            requestExport = false;
            std::string fname = makeTimestampedFilename("heightmap_chunk_r0_c0", "pgm");
            if(!height.empty() && exportHeightmapPGM(height, W, H, fname)){
                lastExportPath = fname;
            }else{
                lastExportPath = "Export failed";
            }
        }

        if(requestExportPNG){
            requestExportPNG = false;
            std::string fname = makeTimestampedFilename("heightmap_chunk_r0_c0", "png");
            if(!height.empty() && exportHeightmapPNG(height, W, H, fname)){
                lastExportPath = fname;
            }else{
                lastExportPath = "Export PNG failed";
            }
        }

        // Export tous les chunks PNG
        if(requestExportAllChunksPNG){
            requestExportAllChunksPNG = false;
            std::string prefix = makeTimestampedFilename("heightmap_chunks", "tmp");
            if(prefix.size() >= 4) prefix = prefix.substr(0, prefix.size()-4);
            std::string msg;
            exportAllChunksPNG(chunkGrid, W, H, prefix, msg);
            lastExportPath = msg + " | prefix: " + prefix;
        }

        // =========================
        // Render scene
        // =========================
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // Dessine tous les chunks
        {
            const int cols = std::max(1, P.chunkCols);
            const int rows = std::max(1, P.chunkRows);

            const float stepVisX = P.terrainWidth  + P.chunkGapVisual;
            const float stepVisY = P.terrainLength + P.chunkGapVisual;

            const float centerCols = 0.5f * (cols - 1);
            const float centerRows = 0.5f * (rows - 1);

            for(int r = 0; r < rows; ++r){
                for(int c = 0; c < cols; ++c){
                    float visX = (c - centerCols) * stepVisX;
                    float visY = (r - centerRows) * stepVisY;

                    const auto& chunkH = chunkGrid.heights[chunkIndex(c, r, cols)];
                    drawMesh(chunkH, W, H, P, visX, visY);
                }
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    if(heightTex){
        glDeleteTextures(1, &heightTex);
    }

    if(autoSaveConfigOnExit){
        saveConfigFile(P, kDefaultConfigPath);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
}