#include "../../ParticleConfig.h"

Material getMaterialScene0(in ParticleData particle, uint pid, in float3 position, in float3 normal, float time)
{
    Material material;
    material.albedo = particle.albedo;
    material.emissive = particle.emissive;
    material.reflection = particle.reflection;
    if (pid == 0)
    {
        float3 checkerP = position * 0.1;
        float checker = abs(fmod(floor(checkerP.x) + floor(checkerP.y) + floor(checkerP.z), 2.0));
        material.reflection = checker;
    }
    else if (pid == 3 || pid == 2)
    {
        float4 fb = fbmd(position * 0.08);
        float s = pow(abs(length(fb)), 0.8);
        material.reflection = s;
    }
    else if (pid == 4)
    {
        //float4 fb = fbmd((position + float3(0, 0, time * 10.0)) * 0.08);
        //float s = pow(abs(length(fb)), 0.8);
        //material.albedo = lerp(float3(1, 0, 0), float3(1, 1, 0), s);
        //material.emissive = s * 1.0;
    }
    return material;
}