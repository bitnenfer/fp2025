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
 
    #define vec3 float3
    float3 color = palette(float(DTid.x) / NUM_PARTICLES, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.10, 0.20));
    seed += DTid.x + Time;
    ParticleData particle = Particles[DTid.x];
    const float bigRadius = 999.0;
    const float offset = 25.0;
    if (DTid.x == 0)
    {
        particle.radius = bigRadius;
        particle.position = float3(0, particle.radius + offset, 0);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.6;
        particle.dynamic = false;
        particle.emissive = 0;
        particle.albedo = 1;
        particle.reflection = 0.0;
        particle.visible = 1;
    }
    else if (DTid.x == 1)
    {
        particle.radius = bigRadius;
        particle.position = float3(0, -particle.radius - offset, 0);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.6;
        particle.dynamic = false;
        particle.emissive = 0;
        particle.albedo = 1;
        particle.reflection = 0.85;
        particle.visible = 1;
    }
    else if (DTid.x == 2)
    {
        particle.radius = bigRadius;
        particle.position = float3(-particle.radius - offset, 0, 0);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.0;
        particle.dynamic = false;
        particle.emissive = 0.0;
        particle.albedo = float3(1, 0, 0);
        particle.reflection = 0.0;
        particle.visible = 1;
    }
    else if (DTid.x == 3)
    {
        particle.radius = bigRadius;
        particle.position = float3(particle.radius + offset, 0, 0);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.0;
        particle.dynamic = false;
        particle.emissive = 0.0;
        particle.albedo = float3(0, 1, 0);
        particle.reflection = 0.0;
        particle.visible = 1;
    }
    else if (DTid.x == 4)
    {
        particle.radius = bigRadius;
        particle.position = float3(0, 0, particle.radius + offset);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.0;
        particle.dynamic = false;
        particle.emissive = 0.0;
        particle.albedo = float3(0, 0, 1);
        particle.reflection = 0.0;
        particle.visible = 1;
    }
    else if (DTid.x == 5)
    {
        particle.radius = bigRadius;
        particle.position = float3(0, 0, -particle.radius - offset);
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.0;
        particle.dynamic = false;
        particle.emissive = 0;
        particle.albedo = 1;
        particle.reflection = 0.0;
        particle.visible = 0;
    }
    else
    {
        particle.radius = 0.5 + rand() * 4.0;
        particle.position = (float3(rand(), rand(), rand()) * 2.0 - 1.0) * particle.radius * 10;
        particle.elasticity = rand() * 0.6;
        particle.velocity = particle.position * -0.005;
        particle.acceleration = float3(0, 0.098, 0);
        particle.friction = 0.5;
        particle.dynamic = true;
        particle.emissive = rand() > 0.2 ? 1.0 : 0;
        particle.albedo = color;
        particle.reflection = 0.6 + rand() * 0.4;
        particle.visible = 1;
    }
    particle.id = DTid.x + 1;
    particle.prevPosition = 0;
    Particles[DTid.x] = particle;
    
}