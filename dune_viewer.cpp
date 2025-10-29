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
#include <algorithm>

// === Emprise fixe du plan (la taille ne dépend pas de W/H) ===
static constexpr float PLANE_HALF_X = 64.0f; // demi-largeur monde
static constexpr float PLANE_HALF_Y = 64.0f; // demi-hauteur monde

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
        lerp(grad(p[A],x,y), grad(p[B],x-1,y), u),
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
/** Paramètres dune **/
// =========================================================
struct Params {
    // --- Bruit de base ---
    int   octaves = 5;
    float lacunarity = 1.9f;
    float gain = 0.45f;
    float freq = 0.01f;
    float amp = 40.f;

    // --- Warp / déformation ---
    float warpAmp = 6.f;
    float warpFreq = 0.008f;

    // --- Forme / anisotropie (agit sur la génération, pas sur la taille du plan rendu) ---
    float stretchX = 1.0f;
    float stretchY = 1.0f;
    float rotationDeg = 0.f;

    // --- Rugosité fine ---
    float ridgeBias = 0.25f;
    float ridgePow = 1.5f;
    float ridgeGain = 0.5f;
    float ridgeLacunarity = 2.0f;
    int   ridgeOctaves = 4;
    float ridgeAttenuation = 0.6f; // (conservé si besoin plus tard)

    // --- Exploration du bruit ---
    float noiseOffsetX = 0.f;
    float noiseOffsetY = 0.f;
    float noiseZoom    = 1.0f;

    // --- Rognage (en colonnes/lignes à retirer côté rendu) ---
    int   cropLeft   = 0;
    int   cropRight  = 0;
    int   cropTop    = 0;
    int   cropBottom = 0;

    // --- Résolution (nombre de vertices) ---
    int   gridW = 128;
    int   gridH = 128;

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
};

// =========================================================
/** Dune generation (sans profil de dune, taille fixe) **/
// =========================================================
void generateHeights(std::vector<float>& H,int W,int Hs,const Params& P){
    H.resize(W*Hs);
    float minH=9999,maxH=-9999;

    const float rotRad = P.rotationDeg * M_PI / 180.f;
    const float cosR = cos(rotRad), sinR = sin(rotRad);

    for(int j=0;j<Hs;j++){
        const float v = (Hs>1) ? (float)j / (Hs - 1) : 0.f;
        const float baseY = (v - 0.5f) * (2.f * PLANE_HALF_Y);

        for(int i=0;i<W;i++){
            const float u = (W>1) ? (float)i / (W - 1) : 0.f;
            const float baseX = (u - 0.5f) * (2.f * PLANE_HALF_X);

            // anisotropie (n'affecte pas la taille du plan rendu)
            const float x0 = baseX * P.stretchX;
            const float y0 = baseY * P.stretchY;

            // rotation globale
            float xr = x0*cosR - y0*sinR;
            float yr = x0*sinR + y0*cosR;

            // offsets d’exploration
            xr += P.noiseOffsetX;
            yr += P.noiseOffsetY;

            // warp optionnel
            float wx=0.f, wy=0.f;
            if(P.warpEnabled){
                wx=fbm(xr*P.warpFreq,      yr*P.warpFreq,      3,2.0f,0.5f)*P.warpAmp;
                wy=fbm((xr+100)*P.warpFreq,(yr+100)*P.warpFreq,3,2.0f,0.5f)*P.warpAmp;
            }

            // zoom du bruit (après warp)
            const float nx=(xr+wx)*P.noiseZoom;
            const float ny=(yr+wy)*P.noiseZoom;

            // bruit principal (avec ou sans ridged)
            const float n = P.ridgedMode
                ? ridgedFBM(nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain)
                : fbm      (nx*P.freq, ny*P.freq, P.octaves, P.lacunarity, P.gain);

            // hauteur (plus de profil de dune)
            float z = (n * 0.25f) * P.amp;
            if(P.invertZ) z *= -1.f;

            H[j*W+i]=z;
            if(z<minH)minH=z;
            if(z>maxH)maxH=z;
        }
    }

    // recentrage vertical doux
    const float offset = -(minH+maxH)/4.f;
    for(float& v : H) v += offset;
}

// =========================================================
/** Mesh drawing avec rognage (taille fixe) **/
// =========================================================
void drawMesh(const std::vector<float>& H,int W,int Hs,const Params& P){
    glPushMatrix();
    glTranslatef(P.camPanX, P.camPanY, -P.camZoom);
    glRotatef(P.camRotX,1,0,0);
    glRotatef(P.camRotY,0,1,0);
    glColor3f(0.9f,0.8f,0.6f);
    glPolygonMode(GL_FRONT_AND_BACK, P.filled ? GL_FILL : GL_LINE);

    // Bornes rognées, clamp
    int iStart = std::clamp(P.cropLeft,  0, std::max(0, W-1));
    int iEnd   = std::clamp(W-1-P.cropRight, 0, W-1);        // inclus
    int jStart = std::clamp(P.cropTop,   0, std::max(0, Hs-2));
    int jEnd   = std::clamp(Hs-2-P.cropBottom, 0, Hs-2);     // inclus

    if(iStart >= iEnd || jStart > jEnd){
        glPopMatrix();
        return;
    }

    for(int j=jStart; j<=jEnd; ++j){
        float v0 = (Hs>1) ? (float)j     / (Hs-1) : 0.f;
        float v1 = (Hs>1) ? (float)(j+1) / (Hs-1) : 0.f;
        float y0 = (v0 - 0.5f) * (2.f * PLANE_HALF_Y);
        float y1 = (v1 - 0.5f) * (2.f * PLANE_HALF_Y);

        glBegin(GL_TRIANGLE_STRIP);
        for(int i=iStart; i<=iEnd; ++i){
            float u = (W>1) ? (float)i / (W-1) : 0.f;
            float x = (u - 0.5f) * (2.f * PLANE_HALF_X);

            float z1=H[j    *W+i];
            float z2=H[(j+1)*W+i];

            glVertex3f(x, z1, y0);
            glVertex3f(x, z2, y1);
        }
        glEnd();
    }
    glPopMatrix();
}

// =========================================================
/** Main **/
// =========================================================
int main(){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win=SDL_CreateWindow("Dune Studio — Advanced Viewer",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1400,950,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx=SDL_GL_CreateContext(win);
    glewInit();

    glViewport(0,0,1400,950);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1400.0/950.0, 0.1, 2000.0);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f,0.25f,0.3f,1.f);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(win,ctx);
    ImGui_ImplOpenGL2_Init();
    ImGui::StyleColorsDark();

    // init perlin
    std::vector<int> perm(256);
    std::mt19937 rng(1337);
    for(int i=0;i<256;i++) perm[i]=i;
    std::shuffle(perm.begin(),perm.end(),rng);
    for(int i=0;i<512;i++) p[i]=perm[i&255];

    Params P;
    int W = P.gridW, H = P.gridH;
    std::vector<float> height;
    generateHeights(height,W,H,P);

    bool quit=false,needUpdate=false, needRebuild=false;
    bool dragging=false; int lastX=0,lastY=0;

    while(!quit){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            ImGui_ImplSDL2_ProcessEvent(&e);
            if(e.type==SDL_QUIT) quit=true;
            if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_RIGHT){
                dragging=true; lastX=e.button.x; lastY=e.button.y;
            }
            if(e.type==SDL_MOUSEBUTTONUP && e.button.button==SDL_BUTTON_RIGHT)
                dragging=false;
            if(e.type==SDL_MOUSEMOTION && dragging){
                P.camRotY += (e.motion.x-lastX)*0.4f;
                P.camRotX += (e.motion.y-lastY)*0.4f;
                lastX=e.motion.x; lastY=e.motion.y;
            }
            if(e.type==SDL_MOUSEWHEEL){
                P.camZoom -= e.wheel.y*10.f;
            }
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Dune Studio Controls");

        if(ImGui::CollapsingHeader("Bruit principal", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderInt ("Octaves",&P.octaves,1,8);
            needUpdate |= ImGui::SliderFloat("Freq",&P.freq,0.002f,0.03f);
            needUpdate |= ImGui::SliderFloat("Lacunarity",&P.lacunarity,1.0f,3.0f);
            needUpdate |= ImGui::SliderFloat("Gain",&P.gain,0.1f,0.9f);
            needUpdate |= ImGui::SliderFloat("Amplitude",&P.amp,5.f,100.f);
        }

        if(ImGui::CollapsingHeader("Warp / Déformation")){
            ImGui::Checkbox("Warp Enabled",&P.warpEnabled);
            needUpdate |= ImGui::SliderFloat("Warp Freq",&P.warpFreq,0.001f,0.02f);
            needUpdate |= ImGui::SliderFloat("Warp Amp",&P.warpAmp,0.f,20.f);
        }

        if(ImGui::CollapsingHeader("Anisotropie / Orientation")){
            needUpdate |= ImGui::SliderFloat("Stretch X",&P.stretchX,0.5f,2.f);
            needUpdate |= ImGui::SliderFloat("Stretch Y",&P.stretchY,0.5f,2.f);
            needUpdate |= ImGui::SliderFloat("Rotation",&P.rotationDeg,-90.f,90.f);
        }

        if(ImGui::CollapsingHeader("Rugosité fine (Ridged)")){
            ImGui::Checkbox("Ridged Mode",&P.ridgedMode);
            needUpdate |= ImGui::SliderInt  ("Ridge Octaves",&P.ridgeOctaves,1,6);
            needUpdate |= ImGui::SliderFloat("Ridge Lacunarity",&P.ridgeLacunarity,1.0f,3.0f);
            needUpdate |= ImGui::SliderFloat("Ridge Gain",&P.ridgeGain,0.2f,0.9f);
            needUpdate |= ImGui::SliderFloat("Ridge Bias",&P.ridgeBias,0.f,0.5f);
            needUpdate |= ImGui::SliderFloat("Ridge Pow",&P.ridgePow,0.5f,3.f);
            needUpdate |= ImGui::SliderFloat("Ridge Atten",&P.ridgeAttenuation,0.2f,1.f);
        }

        if(ImGui::CollapsingHeader("Exploration du bruit", ImGuiTreeNodeFlags_DefaultOpen)){
            needUpdate |= ImGui::SliderFloat("Offset X",&P.noiseOffsetX,-1000.f,1000.f);
            needUpdate |= ImGui::SliderFloat("Offset Y",&P.noiseOffsetY,-1000.f,1000.f);
            needUpdate |= ImGui::SliderFloat("Zoom Bruit",&P.noiseZoom,0.1f,5.0f);
            ImGui::TextUnformatted("Raccourcis: clic droit + drag = rotation, molette = zoom cam");
        }

        if(ImGui::CollapsingHeader("Rognage (crop)")){
            int maxW = std::max(0, W-1);
            int maxH = std::max(0, H-2);
            needUpdate |= ImGui::SliderInt("Left",  &P.cropLeft,   0, maxW);
            needUpdate |= ImGui::SliderInt("Right", &P.cropRight,  0, maxW);
            needUpdate |= ImGui::SliderInt("Top",   &P.cropTop,    0, maxH);
            needUpdate |= ImGui::SliderInt("Bottom",&P.cropBottom, 0, maxH);
        }

        if(ImGui::CollapsingHeader("Résolution (maillage)")){
            bool r1 = ImGui::SliderInt("Verts X",&P.gridW, 16, 1024);
            bool r2 = ImGui::SliderInt("Verts Y",&P.gridH, 16, 1024);
            if(r1 || r2) needRebuild = true; // rebuild automatique
        }

        if(ImGui::CollapsingHeader("Caméra")){
            ImGui::SliderFloat("Rot X",&P.camRotX,-90.f,90.f);
            ImGui::SliderFloat("Rot Y",&P.camRotY,-180.f,180.f);
            ImGui::SliderFloat("Zoom",&P.camZoom,50.f,800.f);
            ImGui::SliderFloat("Pan X",&P.camPanX,-200.f,200.f);
            ImGui::SliderFloat("Pan Y",&P.camPanY,-200.f,200.f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Wireframe / Fill",&P.filled);
        ImGui::Checkbox("Invert Z",&P.invertZ);

        if(ImGui::Button("Regenerate")) needUpdate=true;
        ImGui::SameLine();
        if(ImGui::Button("Reset")){
            P = Params();
            needRebuild = true;
        }

        ImGui::End();

        if(needRebuild){
            W = P.gridW; H = P.gridH;
            height.clear();
            generateHeights(height,W,H,P);
            needRebuild=false;
        } else if(needUpdate){
            generateHeights(height,W,H,P);
        }
        needUpdate=false;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        drawMesh(height,W,H,P);

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
