#include "ParticleConfig.h"

static float3 seed = float3(1, 1, 1);
float rand()
{
    seed = frac(seed * 1.6180339887 + 0.123456789);
    float3 s = seed;
    float n = dot(s, float3(12.9898, 78.233, 37.719));
    return frac(sin(n) * 43758.5453);
}

RWStructuredBuffer<ParticleData> Particles : register(u0);
const float Time : register(b0);

float3 palette(in float t, in float3 a, in float3 b, in float3 c, in float3 d)
{
    return a + b * cos(6.283185 * (c * t + d));
}

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    
    float3 color = palette(float(DTid.x) / NUM_PARTICLES, float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5), float3(1.0, 0.7, 0.4), float3(0.0, 0.15, 0.20));
    seed += DTid.x + Time;
    ParticleData particle = Particles[DTid.x];
    if (DTid.x == 0)
    {
        particle.radius = 100.0;
        particle.position = float3(0, particle.radius + 15.0, 0);
        particle.elasticity = 1.0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 1.0;
        particle.dynamic = false;
        particle.emissive = 0;
        particle.albedo = 1;
        particle.reflection = 0.0;
    }
    else
    {
        particle.radius = 0.5 + rand() * 4.0;
        particle.position = (float3(rand(), rand(), rand()) * 2.0 - 1.0) * 150.0;
        particle.elasticity = 0.7 + rand()*0.3;
        particle.velocity = particle.position*-0.005;
        particle.acceleration = particle.position*-0.001;
        particle.friction = 0.5;
        particle.dynamic = true;
        particle.emissive = rand() > 0.2 ?  2.0 : 0;
        particle.albedo = color;
        particle.reflection = rand();
    }
    particle.id = DTid.x + 1;
    particle.prevPosition = 0;
    Particles[DTid.x] = particle;
    
}