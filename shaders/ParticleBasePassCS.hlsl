#include "ParticleConfig.h"

StructuredBuffer<ParticleData> Particles : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);
RWTexture2D<float4> VelocityBuffer : register(u1);
RWTexture2D<float4> PositionBuffer : register(u2);
RWTexture2D<float4> NormalBuffer : register(u3);
ConstantBuffer<ConstantBufferData> ConstantData : register(b0);

void CreateRayFromUV(float2 uv, float4x4 invViewProj, float3 cameraPosition, out float3 rayOrigin, out float3 rayDirection)
{
    float4 clipSpacePos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    float4 worldPos = mul(clipSpacePos, invViewProj);
    worldPos /= worldPos.w;
    rayOrigin = cameraPosition;
    rayDirection = normalize(worldPos.xyz - cameraPosition);
}

bool IntersectsParticle(float3 RayOrigin, float3 RayDirection, in ParticleData Particle, out float3 OutPosition, out float3 OutNormal)
{
    float3 oc = RayOrigin - Particle.position;
    float a = dot(RayDirection, RayDirection);
    float b = 2.0 * dot(oc, RayDirection);
    float c = dot(oc, oc) - Particle.radius * Particle.radius;
    float discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0)
    {
        OutPosition = float3(0.0, 0.0, 0.0);
        OutNormal = float3(0.0, 0.0, 0.0);
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
            OutPosition = float3(0.0, 0.0, 0.0);
            OutNormal = float3(0.0, 0.0, 0.0);
            return false;
        }
        OutPosition = RayOrigin + t * RayDirection;
        OutNormal = normalize(OutPosition - Particle.position);

        return true;
    }
}

bool Trace(float3 rayOrigin, float3 rayDirection, out ParticleData OutParticle, out float3 OutPosition, out float3 OutNormal)
{
    bool hit = false;
    float lastDepth = 3.402823466e+38F;
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        float3 hitPosition;
        float3 hitNormal;
        if (IntersectsParticle(rayOrigin, rayDirection, Particles[i], hitPosition, hitNormal))
        {
            float dist = length(hitPosition - ConstantData.cameraPos);
            if (dist < lastDepth)
            {
                OutParticle = Particles[i];
                OutPosition = hitPosition;
                OutNormal = hitNormal;
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
float3 GetSampleBiased(float3 dir, float power)
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

float3 GetSample(float3 dir)
{
    return GetSampleBiased(dir, 0.0f); // unbiased
}

float3 GetCosineWeightedSample(float3 dir)
{
    return GetSampleBiased(dir, 1.0f);
}

float3 GetBackground(float3 dir)
{
    return (float3(0.11, 0.11, 0.18) * pow(1.0 - dir.y, 2.0)) * 0;
}

float3 Pathtrace(float3 rayOrigin, float3 rayDirection)
{
    ParticleData particle;
    float3 direct = 0;
    float3 luminance = 1;
    float3 hitNormal;
    float3 hitPosition;

    for (int b = 0; b < 3; ++b)
    {
        if (Trace(rayOrigin, rayDirection, particle, hitPosition, hitNormal))
        {
            rayDirection = lerp(GetCosineWeightedSample(hitNormal), normalize(reflect(rayDirection, hitNormal)), particle.reflection);
            rayOrigin = hitPosition + rayDirection * 0.1;
            if (particle.emissive > 0)
            {
                direct += luminance * particle.albedo * particle.emissive;
            }
            else
            {
                luminance *= particle.albedo;
            }
        }
        else
        {
            direct += luminance * GetBackground(rayDirection);
        }
    }

    return direct;
}

float2 ClipToUV(float4 clip)
{
    float2 ndc = clip.xy / clip.w;
    // If your texture space has (0,0) at top-left (D3D), flip Y:
    return float2(ndc.x * 0.5f + 0.5f,
                  -ndc.y * 0.5f + 0.5f);
    // If no flip needed in your pipeline, use:
    // return ndc * 0.5f + 0.5f;
}

bool CalcPositionNormalAndVelocity(float2 uv, out float3 outPosition, out float3 outNormal, out float2 outVelocity, out ParticleData outParticle)
{
    #if 1
    float3 rayOrigin;
    float3 rayDirection;
    CreateRayFromUV(uv, ConstantData.invViewProjMtx, ConstantData.cameraPos, rayOrigin, rayDirection);
    
    ParticleData particle;
    particle.id = 0;
    float3 hitNormal;
    float3 hitPosition;
    if (Trace(rayOrigin, rayDirection, particle, hitPosition, hitNormal))
    {
        outPosition = hitPosition;
        outNormal = hitNormal;
        outParticle = particle;
        
        float3 prevRayOrigin;
        float3 prevRayDirection;
        CreateRayFromUV(uv, ConstantData.prevInvViewProjMtx, ConstantData.prevCameraPos, prevRayOrigin, prevRayDirection);
        
        ParticleData prevParticle;
        float3 prevHitNormal;
        float3 prevHitPosition;
        Trace(prevRayOrigin, prevRayDirection, prevParticle, prevHitPosition, prevHitNormal);
               
        return true;
    }
    
    float4 prevPos = mul(ConstantData.prevViewProjMtx, float4(hitPosition, 1));
    float4 currPos = mul(ConstantData.viewProjMtx, float4(hitPosition, 1));
        
    float2 prevUv = (prevPos.xy / prevPos.w) * 0.5 + 0.5;
    float2 currUv = (currPos.xy / currPos.w) * 0.5 + 0.5;
    outVelocity = (prevUv - currUv);
    
    return false;

#else
    
    float3 ro, rd;
    CreateRayFromUV(uv, ConstantData.invViewProjMtx, ConstantData.cameraPos, ro, rd);

    ParticleData p = (ParticleData) 0;
    float3 hitP, hitN;
    bool hit = Trace(ro, rd, p, hitP, hitN);

    outParticle = p;
    outPosition = hit ? hitP : 0;
    outNormal = hit ? hitN : 0;
    outVelocity = 0; // default; temporal pass will treat 0 as invalid/no motion

    if (!hit)
        return false;

    float4 clipCurr = mul(ConstantData.viewProjMtx, float4(hitP, 1));
    float4 clipPrev = mul(ConstantData.prevViewProjMtx, float4(hitP, 1));

    if (clipCurr.w <= 0 || clipPrev.w <= 0)
        return true;

    float2 uvCurr = ClipToUV(clipCurr);
    float2 uvPrev = ClipToUV(clipPrev);

    bool inCurr = all(uvCurr >= 0) && all(uvCurr <= 1);
    bool inPrev = all(uvPrev >= 0) && all(uvPrev <= 1);
    if (inCurr && inPrev)
        outVelocity = uvPrev - uvCurr; // curr->prev (UV units)

    return true;
    #endif
}

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = ((float2(DTid.xy)) / (ConstantData.resolution).xy);
    float3 rayOrigin = 0;
    float3 rayDirection = 0;
    float2 hitVelocity = 0;
    float3 hitPosition = 0;
    float3 hitNormal = 0;
    float depth = 0.0;
    float3 color = 0;
    ParticleData hitParticle;
    
    if (CalcPositionNormalAndVelocity(uv, hitPosition, hitNormal, hitVelocity, hitParticle))
    {
        float4 clipSpacePos = mul(ConstantData.viewProjMtx, float4(hitPosition, 1.0));
        float3 ndcHit = clipSpacePos.xyz / clipSpacePos.w;
        depth = ndcHit.z;
       
        
        float4 prevPos = mul(ConstantData.prevViewProjMtx, float4(hitParticle.prevPosition, 1));
        float4 currPos = mul(ConstantData.viewProjMtx, float4(hitParticle.position, 1));
        
        float2 prevUv = (prevPos.xy / prevPos.w) * 0.5 + 0.5;
        float2 currUv = (currPos.xy / currPos.w) * 0.5 + 0.5;
        hitVelocity -= (prevUv - currUv);
        
    }
    
    float offsetTime = ConstantData.time;
#if 0
        if ((DTid.x % 1) == 0 || (DTid.y % 1) == 0)
        {
            offsetTime = 1;
        }
#endif
    seed2 = uv + cos(offsetTime);
    const int samples = 4;
    float2 pxSize = (1 / ConstantData.resolution.xy) * 1;
    for (int i = 0; i < samples; i++)
    {
        float offset = offsetTime + float(i + 1);
        float2 uvOffset = float2(cos(offset), sin(offset)) * pxSize;

        CreateRayFromUV(uv + uvOffset, ConstantData.invViewProjMtx, ConstantData.cameraPos, rayOrigin, rayDirection);
        color += Pathtrace(rayOrigin, rayDirection);
    }
    color /= float(samples);
    
    OutputTexture[DTid.xy] = float4(color, depth);
    VelocityBuffer[DTid.xy] = float4(hitVelocity, 0, 1);
    PositionBuffer[DTid.xy] = float4(hitPosition, float(hitParticle.id));
    NormalBuffer[DTid.xy] = float4(hitNormal, 1);

}