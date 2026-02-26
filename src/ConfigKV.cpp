// src/ConfigKV.cpp
#include "ConfigKV.h"

#include <fstream>
#include <algorithm>
#include <cctype>

namespace dune {

bool ConfigKV::save(const Params& P, const std::string& path)
{
    std::ofstream f(path);
    if(!f) return false;

    f << "# Dune Studio config\n";
    f << "version=2\n";

    f << "octaves=" << P.octaves << "\n";
    f << "lacunarity=" << P.lacunarity << "\n";
    f << "gain=" << P.gain << "\n";
    f << "freq=" << P.freq << "\n";
    f << "amp=" << P.amp << "\n";

    f << "warpAmp=" << P.warpAmp << "\n";
    f << "warpFreq=" << P.warpFreq << "\n";

    f << "stretchX=" << P.stretchX << "\n";
    f << "stretchY=" << P.stretchY << "\n";
    f << "rotationDeg=" << P.rotationDeg << "\n";

    f << "ridgeBias=" << P.ridgeBias << "\n";
    f << "ridgePow=" << P.ridgePow << "\n";
    f << "ridgeGain=" << P.ridgeGain << "\n";
    f << "ridgeLacunarity=" << P.ridgeLacunarity << "\n";
    f << "ridgeOctaves=" << P.ridgeOctaves << "\n";
    f << "ridgeAttenuation=" << P.ridgeAttenuation << "\n";

    f << "noiseOffsetX=" << P.noiseOffsetX << "\n";
    f << "noiseOffsetY=" << P.noiseOffsetY << "\n";
    f << "noiseZoom=" << P.noiseZoom << "\n";

    f << "cropLeft=" << P.cropLeft << "\n";
    f << "cropRight=" << P.cropRight << "\n";
    f << "cropTop=" << P.cropTop << "\n";
    f << "cropBottom=" << P.cropBottom << "\n";

    f << "gridW=" << P.gridW << "\n";
    f << "gridH=" << P.gridH << "\n";

    f << "terrainWidth=" << P.terrainWidth << "\n";
    f << "terrainLength=" << P.terrainLength << "\n";

    f << "chunkCols=" << P.chunkCols << "\n";
    f << "chunkRows=" << P.chunkRows << "\n";
    f << "chunkGapVisual=" << P.chunkGapVisual << "\n";

    f << "filled=" << (P.filled ? 1 : 0) << "\n";
    f << "invertZ=" << (P.invertZ ? 1 : 0) << "\n";
    f << "ridgedMode=" << (P.ridgedMode ? 1 : 0) << "\n";
    f << "warpEnabled=" << (P.warpEnabled ? 1 : 0) << "\n";

    f << "camRotX=" << P.camRotX << "\n";
    f << "camRotY=" << P.camRotY << "\n";
    f << "camZoom=" << P.camZoom << "\n";
    f << "camPanX=" << P.camPanX << "\n";
    f << "camPanY=" << P.camPanY << "\n";

    f << "crestSmoothing=" << P.crestSmoothing << "\n";
    f << "crestSharpen=" << P.crestSharpen << "\n";
    f << "crestWidth=" << P.crestWidth << "\n";

    return true;
}

bool ConfigKV::load(Params& P, const std::string& path)
{
    std::ifstream f(path);
    if(!f) return false;

    std::string line;
    while(std::getline(f, line)){
        line = trimCopy(line);
        if(line.empty() || line[0]=='#') continue;

        size_t eq = line.find('=');
        if(eq == std::string::npos) continue;

        std::string key = trimCopy(line.substr(0, eq));
        std::string val = trimCopy(line.substr(eq + 1));

        int iv; float fv; bool bv;

        if(key == "octaves" && parseIntSafe(val, iv)) P.octaves = iv;
        else if(key == "lacunarity" && parseFloatSafe(val, fv)) P.lacunarity = fv;
        else if(key == "gain" && parseFloatSafe(val, fv)) P.gain = fv;
        else if(key == "freq" && parseFloatSafe(val, fv)) P.freq = fv;
        else if(key == "amp" && parseFloatSafe(val, fv)) P.amp = fv;

        else if(key == "warpAmp" && parseFloatSafe(val, fv)) P.warpAmp = fv;
        else if(key == "warpFreq" && parseFloatSafe(val, fv)) P.warpFreq = fv;

        else if(key == "stretchX" && parseFloatSafe(val, fv)) P.stretchX = fv;
        else if(key == "stretchY" && parseFloatSafe(val, fv)) P.stretchY = fv;
        else if(key == "rotationDeg" && parseFloatSafe(val, fv)) P.rotationDeg = fv;

        else if(key == "ridgeBias" && parseFloatSafe(val, fv)) P.ridgeBias = fv;
        else if(key == "ridgePow" && parseFloatSafe(val, fv)) P.ridgePow = fv;
        else if(key == "ridgeGain" && parseFloatSafe(val, fv)) P.ridgeGain = fv;
        else if(key == "ridgeLacunarity" && parseFloatSafe(val, fv)) P.ridgeLacunarity = fv;
        else if(key == "ridgeOctaves" && parseIntSafe(val, iv)) P.ridgeOctaves = iv;
        else if(key == "ridgeAttenuation" && parseFloatSafe(val, fv)) P.ridgeAttenuation = fv;

        else if(key == "noiseOffsetX" && parseFloatSafe(val, fv)) P.noiseOffsetX = fv;
        else if(key == "noiseOffsetY" && parseFloatSafe(val, fv)) P.noiseOffsetY = fv;
        else if(key == "noiseZoom" && parseFloatSafe(val, fv)) P.noiseZoom = fv;

        else if(key == "cropLeft" && parseIntSafe(val, iv)) P.cropLeft = iv;
        else if(key == "cropRight" && parseIntSafe(val, iv)) P.cropRight = iv;
        else if(key == "cropTop" && parseIntSafe(val, iv)) P.cropTop = iv;
        else if(key == "cropBottom" && parseIntSafe(val, iv)) P.cropBottom = iv;

        else if(key == "gridW" && parseIntSafe(val, iv)) P.gridW = iv;
        else if(key == "gridH" && parseIntSafe(val, iv)) P.gridH = iv;

        else if(key == "terrainWidth" && parseFloatSafe(val, fv)) P.terrainWidth = fv;
        else if(key == "terrainLength" && parseFloatSafe(val, fv)) P.terrainLength = fv;

        else if(key == "chunkCols" && parseIntSafe(val, iv)) P.chunkCols = iv;
        else if(key == "chunkRows" && parseIntSafe(val, iv)) P.chunkRows = iv;
        else if(key == "chunkGapVisual" && parseFloatSafe(val, fv)) P.chunkGapVisual = fv;

        else if(key == "filled" && parseBoolSafe(val, bv)) P.filled = bv;
        else if(key == "invertZ" && parseBoolSafe(val, bv)) P.invertZ = bv;
        else if(key == "ridgedMode" && parseBoolSafe(val, bv)) P.ridgedMode = bv;
        else if(key == "warpEnabled" && parseBoolSafe(val, bv)) P.warpEnabled = bv;

        else if(key == "camRotX" && parseFloatSafe(val, fv)) P.camRotX = fv;
        else if(key == "camRotY" && parseFloatSafe(val, fv)) P.camRotY = fv;
        else if(key == "camZoom" && parseFloatSafe(val, fv)) P.camZoom = fv;
        else if(key == "camPanX" && parseFloatSafe(val, fv)) P.camPanX = fv;
        else if(key == "camPanY" && parseFloatSafe(val, fv)) P.camPanY = fv;

        else if(key == "crestSmoothing" && parseFloatSafe(val, fv)) P.crestSmoothing = fv;
        else if(key == "crestSharpen" && parseFloatSafe(val, fv)) P.crestSharpen = fv;
        else if(key == "crestWidth" && parseFloatSafe(val, fv)) P.crestWidth = fv;
    }

    P.clampSafety();
    return true;
}

std::string ConfigKV::trimCopy(const std::string& s)
{
    size_t a=0,b=s.size();
    while(a<b && std::isspace((unsigned char)s[a])) ++a;
    while(b>a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a,b-a);
}

bool ConfigKV::parseIntSafe(const std::string& s, int& out)
{
    try{
        std::string t = trimCopy(s);
        size_t pos=0;
        int v=std::stoi(t,&pos);
        if(pos!=t.size()) return false;
        out=v; return true;
    }catch(...){ return false; }
}

bool ConfigKV::parseFloatSafe(const std::string& s, float& out)
{
    try{
        std::string t = trimCopy(s);
        size_t pos=0;
        float v=std::stof(t,&pos);
        if(pos!=t.size()) return false;
        out=v; return true;
    }catch(...){ return false; }
}

bool ConfigKV::parseBoolSafe(const std::string& s, bool& out)
{
    std::string v = trimCopy(s);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    if(v=="1"||v=="true"||v=="yes"||v=="on"){ out=true; return true; }
    if(v=="0"||v=="false"||v=="no"||v=="off"){ out=false; return true; }
    return false;
}

} // namespace dune