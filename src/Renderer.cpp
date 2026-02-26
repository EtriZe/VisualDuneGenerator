// src/Renderer.cpp
#include "Renderer.h"

#include <GL/gl.h>
#include <cmath>
#include <algorithm>

namespace dune {

static void LoadPerspective(float fovyDeg, float aspect, float zNear, float zFar)
{
    const float pi = 3.14159265358979323846f;
    const float fovyRad = fovyDeg * (pi / 180.0f);
    const float f = 1.0f / tanf(fovyRad * 0.5f);

    float m[16] = {};
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m);
}

void Renderer::setupGL(int winW, int winH)
{
    glViewport(0,0,winW,winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    LoadPerspective(60.0f, (float)winW/(float)winH, 0.1f, 4000.0f);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f,0.25f,0.3f,1.f);
}

void Renderer::onResize(int winW, int winH){ setupGL(winW, winH); }

void Renderer::beginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
}

void Renderer::drawAllChunks(const ChunkGrid& grid, int W, int H, const Params& P)
{
    const int cols = std::max(1, P.chunkCols);
    const int rows = std::max(1, P.chunkRows);

    const float stepVisX = P.terrainWidth  + P.chunkGapVisual;
    const float stepVisY = P.terrainLength + P.chunkGapVisual;

    const float centerCols = 0.5f * (cols - 1);
    const float centerRows = 0.5f * (rows - 1);

    for(int r=0;r<rows;++r){
        for(int c=0;c<cols;++c){
            float visX = (c - centerCols) * stepVisX;
            float visY = (r - centerRows) * stepVisY;
            const auto& chunkH = grid.heights[ChunkGrid::index(c,r,cols)];
            drawMesh(chunkH, W, H, P, visX, visY);
        }
    }
}

void Renderer::drawMesh(const std::vector<float>& H, int W, int Hs, const Params& P, float visualOffsetX, float visualOffsetY)
{
    glPushMatrix();
    glTranslatef(P.camPanX, P.camPanY, -P.camZoom);
    glRotatef(P.camRotX,1,0,0);
    glRotatef(P.camRotY,0,1,0);

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

            float z1=H[(size_t)j*(size_t)W + (size_t)i];
            float z2=H[(size_t)(j+1)*(size_t)W + (size_t)i];

            glVertex3f(x, z1, y0);
            glVertex3f(x, z2, y1);
        }
        glEnd();
    }
    glPopMatrix();
}

} // namespace dune