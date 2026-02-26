// src/Noise.h
#pragma once

#include <cstdint>

namespace dune {

class Noise {
public:
    void init(uint32_t seed = 1337);

    float perlin(float x, float y) const;
    float fbm(float x,float y,int oct,float lac,float gain) const;
    float ridgedFBM(float x,float y,int oct,float lac,float gain) const;

private:
    int p_[512]{};

    static float fade(float t);
    static float lerp(float a,float b,float t);
    static float grad(int hash,float x,float y);
};

} // namespace dune