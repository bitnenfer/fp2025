#include "ParticleConfig.h"

RWStructuredBuffer<ParticleData> Particles : register(u0);

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
    float3 depthVector = IntersectionDepth(dynamicParticle, staticParticle);
    if (length(depthVector) > 0.0)
    {
        float3 normal = normalize(depthVector);
        float velocityDotNormal = dot(dynamicParticle.velocity, normal);
        float3 reflection = normal * (2.0 * velocityDotNormal);
        float3 newVelocity = dynamicParticle.velocity - reflection;
        dynamicParticle.velocity = newVelocity * dynamicParticle.elasticity;
        float depthDepth = dot(depthVector, normal);
        float bias = 0.01;
        float3 correction = normal * (depthDepth + bias);
        dynamicParticle.position -= correction;
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
        float impulseScalar = -(1.0 + restitution) * velocityAlongNormal / 2.0;
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
        Particles[DTid.x].acceleration = 0.0005 * (float3(0, 0, 0) - particle.position);
        Particles[DTid.x].velocity += particle.acceleration;
        Particles[DTid.x].position += particle.velocity;
        

        for (int i = 0; i < NUM_PARTICLES; ++i)
        {
            if (i != DTid.x)
            {
                ParticleData particle1 = Particles[DTid.x];
                ParticleData particle2 = Particles[i];
                //if (particle1)

                // if (particle1.bDynamic && particle2.bDynamic) 
                // {
                HandleDynamicParticleResponse(particle1, particle2);
                // }
                // else 
                //if (!particle1.bDynamic && particle2.bDynamic)
                // {
                //     HandleStaticParticleResponse(particle2, particle1);
                // }
                // else if (!particle1.bDynamic && particle2.bDynamic)
                // {
                // 	HandleStaticParticleResponse(particle2, particle1);
                // }

                Particles[DTid.x] = particle1;
            }
        }
    }
    Particles[DTid.x].prevPosition = Particles[DTid.x].position;
}