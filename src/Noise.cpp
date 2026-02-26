// src/Noise.cpp
#include "Noise.h"

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

namespace dune {

void Noise::init(uint32_t seed)
{
    std::vector<int> perm(256);
    std::mt19937 rng(seed);
    for(int i=0;i<256;i++) perm[i]=i;
    std::shuffle(perm.begin(),perm.end(),rng);
    for(int i=0;i<512;i++) p_[i]=perm[i&255];
}

float Noise::fade(float t){ return t*t*t*(t*(t*6-15)+10); }
float Noise::lerp(float a,float b,float t){ return a + t*(b-a); }

float Noise::grad(int hash,float x,float y){
    int h=hash&7;
    float u=h<4?x:y;
    float v=h<4?y:x;
    return ((h&1)?-u:u)+((h&2)?-2.0f*v:2.0f*v);
}

float Noise::perlin(float x, float y) const
{
    int X=(int)floor(x)&255;
    int Y=(int)floor(y)&255;
    x-=floor(x); y-=floor(y);
    float u=fade(x), v=fade(y);
    int A=p_[X]+Y, B=p_[X+1]+Y;

    return lerp(
        lerp(grad(p_[A],x,y),     grad(p_[B],x-1,y), u),
        lerp(grad(p_[A+1],x,y-1), grad(p_[B+1],x-1,y-1), u),
        v
    );
}

float Noise::fbm(float x,float y,int oct,float lac,float gain) const
{
    float amp=0.5f,freq=1.f,sum=0;
    for(int i=0;i<oct;i++){
        sum+=amp*perlin(x*freq,y*freq);
        freq*=lac; amp*=gain;
    }
    return sum;
}

float Noise::ridgedFBM(float x,float y,int oct,float lac,float gain) const
{
    float amp=0.5f,freq=1.f,sum=0;
    for(int i=0;i<oct;i++){
        float r = 1.f - fabsf(perlin(x*freq,y*freq));
        r*=r;
        sum += r*amp;
        freq*=lac; amp*=gain;
    }
    return std::clamp(sum,0.f,1.f);
}

} // namespace dune