#include "ParticleConfig.h"

StructuredBuffer<ParticleData> particles : register(t0);
RWTexture2D<float4> outputTexture : register(u0);
RWTexture2D<float4> velocityBuffer : register(u1);
RWTexture2D<float4> positionBuffer : register(u2);
RWTexture2D<float4> normalBuffer : register(u3);
RWTexture2D<float> depthBuffer : register(u4);

ConstantBuffer<ConstantBufferData> constantData : register(b0);
ConstantBuffer<ParticleSceneData> particleScene : register(b1);
ConstantBuffer<SimulationData> simData : register(b2);

struct PathtraceOutput
{
    float3 color;
};

float hash(float n)
{
    return frac(sin(n) * 753.5);
}

float4 noised(in float3 x)
{
    float3 p = floor(x);
    float3 w = frac(x);
    float3 u = w * w * (3 - 2 * w);
    float3 du = 6 * w * (1 - w);

    float n = p.x + p.y * 157 + 113 * p.z;

    float a = hash(n);
    float b = hash(n + 1);
    float c = hash(n + 157);
    float d = hash(n + 158);
    float e = hash(n + 113);
    float f = hash(n + 114);
    float g = hash(n + 270);
    float h = hash(n + 271);

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = e - a;
    float k4 = a - b - c + d;
    float k5 = a - c - e + g;
    float k6 = a - b - e + f;
    float k7 = -a + b + c - d + e - f - g + h;

    return float4(k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z,
		du * (float3(k1, k2, k3) + u.yzx * float3(k4, k5, k6) + u.zxy * float3(k6, k4, k5) + k7 * u.yzx * u.zxy));
}

float4 fbmd(in float3 x)
{
    float a = 0,
		b = 0.5,
		f = 1;
    float3 d = float3(0, 0, 0);
	[unroll]
    for (int i = 0; i < 3; i++)
    {
        float4 n = noised(f * x);
        a += b * n.x; // accumulate values      
        d += b * n.yzw * f; // accumulate derivatives
        b *= 0.5; // amplitude decrease
        f *= 1.8; // frequency increase
    }

    return float4(a, d);
}

#include "scenes/scene0/Material0.hlsli"

Material getMaterial(in ParticleData particle, uint pid, inout float3 position, inout float3 normal, uint scene, float time)
{
    Material material = (Material)0;
    if (scene == 0) material = getMaterialScene0(particle, pid, position, normal, time);
    return material;
}


void createRayFromUV(float2 uv, float4x4 invViewProj, float3 cameraPosition, out float3 rayOrigin, out float3 rayDirection)
{
    float4 clipSpacePos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    float4 worldPos = mul(clipSpacePos, invViewProj);
    worldPos /= worldPos.w;
    rayOrigin = cameraPosition;
    rayDirection = normalize(worldPos.xyz - cameraPosition);
}

bool intersectsParticle(float3 rayOrigin, float3 rayDirection, in ParticleData particle, out float3 outPosition, out float3 outNormal, out float outDist, out float3 exitPosition)
{
    float3 oc = rayOrigin - particle.position;
    float a = dot(rayDirection, rayDirection);
    float b = 2.0 * dot(oc, rayDirection);
    float c = dot(oc, oc) - particle.radius * particle.radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
    {
        outPosition = float3(0.0, 0.0, 0.0);
        outNormal = float3(0.0, 0.0, 0.0);
        outDist = 0;
        exitPosition = float3(0.0, 0.0, 0.0);
        return false;
    }
    else
    {
        float sqrtDiscriminant = sqrt(discriminant);
        float t1 = (-b - sqrtDiscriminant) / (2.0 * a);
        float t2 = (-b + sqrtDiscriminant) / (2.0 * a);

        float t = (t1 > 0.0) ? t1 : t2;
        if (t < 0.0)
        {
            outPosition = float3(0.0, 0.0, 0.0);
            outNormal = float3(0.0, 0.0, 0.0);
            outDist = 0;
            exitPosition = float3(0.0, 0.0, 0.0);
            return false;
        }

        outPosition = rayOrigin + t * rayDirection;
        outNormal = normalize(outPosition - particle.position);
        outDist = t;

        // exit point always corresponds to the farther root
        float tExit = max(t1, t2);
        exitPosition = rayOrigin + tExit * rayDirection;

        return true;
    }
}

bool trace(float3 rayOrigin, float3 rayDirection, out ParticleData outParticle, out float3 outPosition, out float3 outNormal, out float3 outExit)
{
    bool hit = false;
    float lastDepth = 3.402823466e+38F;
    for (int i = 0; i < particleScene.numParticles; i++)
    {
        if (!particles[i].visible)
        {
            continue;
        }
        
        float3 hitPosition;
        float3 hitNormal;
        float3 hitExit;
        float dist = 0.0;
        if (intersectsParticle(rayOrigin, rayDirection, particles[i], hitPosition, hitNormal, dist, hitExit))
        {
            if (dist < lastDepth)
            {
                outParticle = particles[i];
                outPosition = hitPosition;
                outNormal = hitNormal;
                outExit = hitExit;
                lastDepth = dist;
            }
            hit = true;
        }
    }
    return hit;
}

static float2 seed2 = float2(1.0f, 1.0f);
float2 rand2n()
{
    seed2 += float2(-1.0f, 1.0f);
    float x = frac(sin(dot(seed2.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
    float y = frac(cos(dot(seed2.xy, float2(4.898f, 7.23f))) * 23421.631f);
    return float2(x, y);
}

float3 ortho(float3 v)
{
    // http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
    return (abs(v.x) > abs(v.z)) ? float3(-v.y, v.x, 0.0f)
                                 : float3(0.0f, -v.z, v.y);
}
static const float PI = 3.14159265358979323846f;
float3 getSampleBiased(float3 dir, float power)
{
    dir = normalize(dir);
    float3 o1 = normalize(ortho(dir));
    float3 o2 = normalize(cross(dir, o1));

    float2 r = rand2n();
    r.x = r.x * 2.0f * PI;
    r.y = pow(r.y, 1.0f / (power + 1.0f));

    float oneminus = sqrt(max(0.0f, 1.0f - r.y * r.y));

    return cos(r.x) * oneminus * o1
         + sin(r.x) * oneminus * o2
         + r.y * dir;
}

float3 getSample(float3 dir)
{
    return getSampleBiased(dir, 0.0f); // unbiased
}

float3 getCosineWeightedSample(float3 dir)
{
    return getSampleBiased(dir, 1.0f);
}

float3 getBackground(float3 dir)
{
    return (float3(0.11, 0.11, 0.18) * pow(((1.0 - dir.y)), 2.0)) * 0;
}

PathtraceOutput pathtrace(float3 rayOrigin, float3 rayDirection)
{
    ParticleData particle;
    float3 luminance = 1;
    float3 hitNormal = 0;
    float3 hitPosition = 0;
    float3 hitExit = 0;
    PathtraceOutput output;
    output.color = 0;
        
    for (int bounce = 0; bounce < 5; ++bounce)
    {
        if (trace(rayOrigin, rayDirection, particle, hitPosition, hitNormal, hitExit))
        {
            Material hitMaterial = getMaterial(particle, particle.id - 1, hitPosition, hitNormal, simData.scene, simData.time);
            if (hitMaterial.transparency > 0.0)
            {
                if (hitMaterial.reflection == 0.0 || rand2n().x > abs(hitMaterial.transparency * hitMaterial.reflection) * 0.5)
                {
                    rayDirection = (normalize(refract(rayDirection, normalize(particle.position - hitExit), hitMaterial.indexOfRefraction)));
                    rayOrigin = hitExit + rayDirection * 1e-3;
                    float3 color = lerp(hitMaterial.albedo, 1, hitMaterial.transparency);
                    luminance *= (color * 2);

                }
                else
                {
                    rayDirection = lerp(getCosineWeightedSample(hitNormal), normalize(reflect(rayDirection, hitNormal)), hitMaterial.reflection);
                    rayOrigin = hitPosition + rayDirection * 1e-3;
                    luminance *= hitMaterial.albedo;
                }
            }
            else
            {
                rayDirection = lerp(getCosineWeightedSample(hitNormal), normalize(reflect(rayDirection, hitNormal)), hitMaterial.reflection);
                rayOrigin = hitPosition + rayDirection * 1e-3;
                luminance *= hitMaterial.albedo;
            }
            
            if (hitMaterial.emissive > 0)
            {
                output.color += luminance * hitMaterial.albedo * hitMaterial.emissive;
            }
        }
        else
        {
            output.color += luminance * getBackground(rayDirection);
            break;
        }
    }

    return output;
}

bool calcPositionNormalAndVelocity(float2 uv, out float3 outPosition, out float3 outNormal, out float2 outVelocity, out ParticleData outParticle)
{
    float3 rayOrigin;
    float3 rayDirection;
    createRayFromUV(uv, constantData.invViewProjMtx, constantData.cameraPos, rayOrigin, rayDirection);
    
    ParticleData particle;
    particle.id = 0;
    float3 hitNormal = -rayDirection;
    float3 hitPosition = rayOrigin + rayDirection * 3.402823466e+38F;
    float3 hitExit = 0;
    bool result = trace(rayOrigin, rayDirection, particle, hitPosition, hitNormal, hitExit);
    
    {
        outPosition = hitPosition;
        outNormal = hitNormal;
        outParticle = particle;
        
        float3 prevRayOrigin;
        float3 prevRayDirection;
        createRayFromUV(uv, constantData.prevInvViewProjMtx, constantData.prevCameraPos, prevRayOrigin, prevRayDirection);
        
        ParticleData prevParticle;
        float3 prevHitNormal = -prevRayDirection;
        float3 prevHitPosition = prevRayOrigin + prevRayDirection * 3.402823466e+38F;
        float3 prevHitExit = 0;
        trace(prevRayOrigin, prevRayDirection, prevParticle, prevHitPosition, prevHitNormal, hitExit);
        
    }
    
    float4 prevPos = mul(constantData.prevViewProjMtx, float4(hitPosition, 1));
    float4 currPos = mul(constantData.viewProjMtx, float4(hitPosition, 1));
        
    float2 prevUv = (prevPos.xy / prevPos.w) * 0.5 + 0.5;
    float2 currUv = (currPos.xy / currPos.w) * 0.5 + 0.5;
    outVelocity = (prevUv - currUv);
    
    return result;
}

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = ((float2(DTid.xy)) / (constantData.resolution).xy);
    float3 rayOrigin = 0;
    float3 rayDirection = 0;
    float2 hitVelocity = 0;
    float3 hitPosition = 0;
    float3 hitNormal = 0;
    float depth = 0.0;
    ParticleData hitParticle;
    PathtraceOutput output;
    output.color = 0;
    
    float offsetTime = constantData.time;
#if 0
    if ((DTid.x % 4) == 0 || (DTid.y % 4) == 0)
    {
        offsetTime = fmod(offsetTime, 1);
    }
#endif
    seed2 = uv + cos(offsetTime);
    
    bool result = calcPositionNormalAndVelocity(uv, hitPosition, hitNormal, hitVelocity, hitParticle);
    {
        float4 clipSpacePos = mul(constantData.viewProjMtx, float4(hitPosition, 1.0));
        float3 ndcHit = clipSpacePos.xyz / clipSpacePos.w;
        depth = result ? ndcHit.z : 1;
        
        float4 prevPos = mul(constantData.viewProjMtx, float4(hitParticle.prevPosition, 1));
        float4 currPos = mul(constantData.viewProjMtx, float4(hitParticle.position, 1));
        
        float2 prevUv = (prevPos.xy / prevPos.w) * 0.5 + 0.5;
        float2 currUv = (currPos.xy / currPos.w) * 0.5 + 0.5;
        hitVelocity += (prevUv - currUv);
    }
    
    const int samples = constantData.sampleCount;
    for (int i = 0; i < samples; i++)
    {
        createRayFromUV(uv, constantData.invViewProjMtx, constantData.cameraPos, rayOrigin, rayDirection);
        PathtraceOutput ptResult = pathtrace(rayOrigin, rayDirection);
        output.color += ptResult.color;
    }
    output.color /= float(samples);
    
    outputTexture[DTid.xy] = float4(output.color, 1);
    velocityBuffer[DTid.xy] = float4(hitVelocity, 0, 1);
    positionBuffer[DTid.xy] = float4(hitPosition, float(hitParticle.id));
    normalBuffer[DTid.xy] = float4(hitNormal, 1);
    depthBuffer[DTid.xy] = depth;
}