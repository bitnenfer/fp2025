#include "ParticleConfig.h"

RWStructuredBuffer<ParticleData> particles : register(u0);
ConstantBuffer<SimulationData> simData : register(b0);
ConstantBuffer<ParticleSceneData> particleScene : register(b1);

void handleStaticParticleResponse(inout ParticleData dynamicParticle, ParticleData staticParticle);
void handleDynamicParticleResponse(inout ParticleData dynamicparticle1, inout ParticleData dynamicparticle2);
static float3 seed = float3(1, 1, 1);
float rand();
float3 palette(in float t, in float3 a, in float3 b, in float3 c, in float3 d);
float3 getRandomColor(float t)
{
#define vec3 float3
    float3 color = palette(t, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.10, 0.20));
#undef vec3
    return color;
}

#include "scenes/scene0/Sim0.hlsli"

void initScene(uint pid, uint scene)
{
    if (scene == 0) initScene0(pid);
}

void simulateScene(uint pid, uint scene, float time)
{
    if (scene == 0) simScene0(pid, time);
}

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    seed += DTid.x + simData.time;
    // Always run this...
    particles[DTid.x].id = DTid.x + 1;
    particles[DTid.x].prevPosition = particles[DTid.x].position;
    
    if (simData.frame > 0)
    {
        simulateScene(DTid.x, simData.scene, simData.time);
    }
    else
    {
        initScene(DTid.x, simData.scene);
    }
}

float3 palette(in float t, in float3 a, in float3 b, in float3 c, in float3 d)
{
    return a + b * cos(6.283185 * (c * t + d));
}

float rand()
{
    seed = frac(seed * 1.6180339887 + 0.123456789);
    float3 s = seed;
    float n = dot(s, float3(12.9898, 78.233, 37.719));
    return frac(sin(n) * 43758.5453);
}

// Function to handle response for a dynamic particle colliding with a static particle
void handleStaticParticleResponse(inout ParticleData dynamicParticle, ParticleData staticParticle)
{
    float3 delta = dynamicParticle.position - staticParticle.position;
    float dist = length(delta);
    float sumR = dynamicParticle.radius + staticParticle.radius;

    if (dist < sumR)
    {
        float3 n = (dist > 1e-6) ? (delta / dist) : float3(0, 1, 0);
        float penetration = sumR - dist;

        const float percent = 0.8f;
        const float slop = 1e-3f;
        float corr = percent * max(penetration - slop, 0.0f);
        dynamicParticle.position += n * corr;

        float vN = dot(dynamicParticle.velocity, n);
        if (vN < 0.0f)
        {
            float e = saturate(dynamicParticle.elasticity);
            float mu = saturate(dynamicParticle.friction * staticParticle.friction);

            float3 vNvec = vN * n;
            float3 vT = dynamicParticle.velocity - vNvec;
            float3 vN_post = -e * vNvec;
            float3 vT_post = vT * (1.0f - mu);

            dynamicParticle.velocity = vN_post + vT_post;
        }
    }
}

// Function to handle dynamic particle collision response
void handleDynamicParticleResponse(inout ParticleData dynamicparticle1, inout ParticleData dynamicparticle2)
{
    float3 delta = dynamicparticle2.position - dynamicparticle1.position;
    float distance = length(delta);
    float radiiSum = dynamicparticle1.radius + dynamicparticle2.radius;
    if (distance < radiiSum)
    {
        float3 normal = normalize(delta);
        float interDepth = radiiSum - distance;
        float3 relativeVelocity = dynamicparticle1.velocity - dynamicparticle2.velocity;
        float velocityAlongNormal = dot(relativeVelocity, normal);
        float restitution = min(dynamicparticle1.elasticity, dynamicparticle2.elasticity);
        float impulseScalar = -(1.0 + restitution) * velocityAlongNormal;
        float3 impulse = normal * impulseScalar;
        float3 correction = normal * (interDepth / (dynamicparticle1.radius + dynamicparticle2.radius));

        if (dynamicparticle1.dynamic)
        {
            dynamicparticle1.velocity += impulse * dynamicparticle2.friction;
            dynamicparticle1.position -= correction * dynamicparticle2.radius;
        }

        if (dynamicparticle2.dynamic)
        {
            dynamicparticle2.velocity -= impulse * dynamicparticle2.friction;
            dynamicparticle2.position += correction * dynamicparticle1.radius;
        }
        
    }
}