#include "../../ParticleConfig.h"

void initScene0(uint pid)
{
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
        particle.material.emissive = 0;
        particle.material.albedo = 1;
        particle.material.reflection = 0.0;
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
        particle.material.emissive = 0;
        particle.material.albedo = 1;
        particle.material.reflection = 0.85;
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
        particle.material.emissive = 0.0;
        particle.material.albedo = 0.1;
        particle.material.reflection = 1.0;
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
        particle.material.emissive = 0.0;
        particle.material.albedo = .05;
        particle.material.reflection = 0.0;
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
        particle.material.emissive = 0.0;
        particle.material.albedo = 1; //float3(0, 0, 1);
        particle.material.reflection = 0.0;
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
        particle.material.emissive = 0;
        particle.material.albedo = 1;
        particle.material.reflection = 0.0;
        particle.visible = 0;
    }
    else if (pid == 6)
    {
        // camera particle
        particle.radius = 4;
        particle.position = simData.cameraPos;
        particle.elasticity = 0;
        particle.velocity = 0;
        particle.acceleration = 0;
        particle.friction = 0.0;
        particle.dynamic = false;
        particle.material.emissive = 0.0;
        particle.material.albedo = 0; //float3(0, 0, 1);
        particle.material.reflection = 0.0;
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
        particle.material.emissive = rand() < 0.2 ? 1.0 : 0;
        particle.material.albedo = getRandomColor(float(pid) / particleScene.numParticles);// * float3(0.01, 0.03, 1.0);
        particle.material.reflection = 1;
        particle.visible = 1;
    }
    particle.prevPosition = 0;
    particles[pid] = particle;
}

void simScene0(uint pid, float time)
{
    ParticleData particle = particles[pid];
    if (particle.dynamic)
    {
        particles[pid].acceleration = 0.00001 * (float3(0, 0, 0) - particle.position);
        //particles[pid].emissive = abs(sin((sin(pid + time * .5) * 0.5))) * 1.0;
        particles[pid].velocity += particle.acceleration;
        //particles[pid].position += particle.velocity;
        //particles[pid].albedo = getRandomColor(fmod(float(pid) / particleScene.numParticles + time * 0.1, 1.0));
    }
    
    if (pid == 6)
    {
        particles[pid].position = simData.cameraPos;
    }

    for (int i = 0; i < particleScene.numParticles; ++i)
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