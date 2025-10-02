
#include "ParticleConfig.h"

Texture2D<float> depthBuffer : register(t0);
Texture2D<float4> Frame : register(t1);
ConstantBuffer<DepthOfFieldData> constantData : register(b0);

SamplerState Sampler : register(s0);

/* http://tuxedolabs.blogspot.com/2018/05/bokeh-depth-of-field-in-single-pass.html */
static const float uFar = 1000.0; // Far plane
static const float GOLDEN_ANGLE = 7.39996323;
static const float MAX_BLUR_SIZE = 30.0;
static const float RAD_SCALE = 1.5; // Smaller = nicer blur, larger = faster
static const float MAX_DIST = 100.0;
#define NEAR_PLANE 0.1
#define FAR_PLANE 1000.0

float getBlurSize(float depth, float focusPoint, float focusScale)
{
    float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
    return abs(coc) * abs(constantData.blurSize);
}

float linearizeDepth(float zDev)
{
    return (NEAR_PLANE * FAR_PLANE) / max(FAR_PLANE - zDev * (FAR_PLANE - NEAR_PLANE), 1e-6);
}

float3 depthOfField(float2 texCoord, float focusPoint, float focusScale)
{
    float4 mainSample = Frame.SampleLevel(Sampler, texCoord, 0);
    float centerDepth = linearizeDepth(depthBuffer.SampleLevel(Sampler, texCoord, 0)) / constantData.maxDist * uFar;
    float centerSize = getBlurSize(centerDepth, focusPoint, focusScale);
    float3 color = mainSample.rgb;
    float tot = 1.0;
    float radius = constantData.radiusScale;
    float2 uPixelSize = 1.0 / constantData.resolution.xy;
    float Count = 0.0;
    for (float ang = 0.0; radius < abs(constantData.blurSize); ang += GOLDEN_ANGLE)
    {
        float2 tc = texCoord + float2(cos(ang), sin(ang)) * uPixelSize * radius;
        tc = clamp(tc, 0.0, 0.995);
        float4 samp = Frame.SampleLevel(Sampler, tc, 0);
        float Depth = linearizeDepth(depthBuffer.SampleLevel(Sampler, tc, 0));
        if (Depth < -1)
            return 0;
        float sampleDepth = Depth / constantData.maxDist * uFar;
        float sampleSize = getBlurSize(sampleDepth, focusPoint, focusScale);
        if (sampleDepth > centerDepth || sampleDepth < -1)
            sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);

        float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
        color += lerp(color / tot, samp.rgb, m);
        tot += 1.0;
        radius += constantData.radiusScale / radius;
        if (Count > 1)
            break;
    }
    return color /= tot;
}

float4 main(float4 pos : SV_Position) : SV_Target
{
    float2 Uv = pos.xy / constantData.resolution.xy;
    
    #if 0
    float centerDepth = linearizeDepth(depthBuffer.SampleLevel(Sampler, Uv, 0)) / constantData.maxDist * uFar;
    float centerSize = getBlurSize(centerDepth, constantData.focusPoint, constantData.focusScale) * 0.05;
    return float4(centerSize, centerSize, centerSize, 1);
    #endif
    
    if (depthBuffer.SampleLevel(Sampler, Uv, 0).r < -1)
        return Frame.Sample(Sampler, Uv);

    return float4(depthOfField(Uv, constantData.focusPoint, constantData.focusScale), 1);
}
