#include "ParticleConfig.h"

void initScene0(uint pid)
{
#define vec3 float3
    float3 color = palette(float(pid) / NUM_PARTICLES, vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.10, 0.20));
#undef vec3
    ParticleData particle = particles[pid];
    const float bigRadius = 999.0;
    const float offset = 25.0;
    if (pid == 0)
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
    else if (pid == 1)
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
    else if (pid == 2)
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
    else if (pid == 3)
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
    else if (pid == 4)
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
    else if (pid == 5)
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
    particle.id = pid + 1;
    particle.prevPosition = 0;
    particles[pid] = particle;
}

void simScene0(uint pid, float time)
{
    ParticleData particle = particles[pid];
    if (particle.dynamic)
    {
        particles[pid].acceleration = 0.00001 * (float3(0, 0, 0) - particle.position);
            //particles[DTid.x].emissive = abs(sin((sin(DTid.x + Time * .5) * 0.5)));
        particles[pid].velocity += particle.acceleration;
        particles[pid].position += particle.velocity;
    }

    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        particle = particles[pid];
        if (i != pid)
        {
            ParticleData particle2 = particles[i];

            if (particle.dynamic && particle2.dynamic)
            {
                handleDynamicParticleResponse(particle, particle2);
            }
            else if (particle.dynamic && !particle2.dynamic)
            {
                handleStaticParticleResponse(particle, particle2);
            }
            particles[pid] = particle;
        }
    }
}