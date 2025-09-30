#include "ParticleConfig.h"

RWStructuredBuffer<ParticleData> Particles : register(u0);
const float Time : register(b0);

float3 IntersectionDepth(ParticleData dynamicParticle, ParticleData staticParticle)
{
    float3 delta = dynamicParticle.position - staticParticle.position;
    float distance = length(delta);
    float radiiSum = dynamicParticle.radius + staticParticle.radius;
    
    if (distance < radiiSum)
    {
        return delta * ((radiiSum - distance) / distance);
    }
    return float3(0.0, 0.0, 0.0);
}


// Function to handle response for a dynamic particle colliding with a static particle
void HandleStaticParticleResponse(inout ParticleData dynamicParticle, ParticleData staticParticle)
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
void HandleDynamicParticleResponse(inout ParticleData dynamicparticle1, inout ParticleData dynamicparticle2)
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
        float impulseScalar = -(1.0 + restitution) * velocityAlongNormal ;
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

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    ParticleData particle = Particles[DTid.x];
    float3 prevPosition = particle.position;
    
    if (particle.dynamic)
    {
        Particles[DTid.x].acceleration = 0.00001 * (float3(0, 0, 0) - particle.position);
        //Particles[DTid.x].emissive = abs(sin((sin(DTid.x + Time * .5) * 0.5)));
        Particles[DTid.x].velocity += particle.acceleration;
        Particles[DTid.x].position += particle.velocity;
    }

    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        particle = Particles[DTid.x];
        if (i != DTid.x)
        {
            ParticleData particle2 = Particles[i];

            if (particle.dynamic && particle2.dynamic) 
            {
                HandleDynamicParticleResponse(particle, particle2);
            }
            else if (particle.dynamic && !particle2.dynamic)
            {
                HandleStaticParticleResponse(particle, particle2);
            }
            Particles[DTid.x] = particle;
        }
    }
    Particles[DTid.x].prevPosition = Particles[DTid.x].position;
}