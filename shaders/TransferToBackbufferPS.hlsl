
#include "ParticleConfig.h"

Texture2D<float4> frameInput;
ConstantBuffer<FinalPassData> finalPassData : register(b0);
SamplerState linearSampler;

float3 ACES_Tonemap(float3 color)
{
    // ACES input/output matrices
    const float3x3 m1 = float3x3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );

    const float3x3 m2 = float3x3(
        1.60475, -0.10208, -0.00327,
       -0.53108, 1.10813, -0.07276,
       -0.07367, -0.00605, 1.07602
    );

    float3 v = mul(m1, color);
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;

    float3 x = mul(m2, a / b);
    return pow(saturate(x), 1.0 / 2.2);
}

// Global gamma value (set this somewhere in your shader)
static float gamma = 2.2;

// --- Linear Tone Mapping ---
float3 LinearToneMapping(float3 color)
{
    float exposure = 1.0;
    color = saturate(exposure * color);
    color = pow(color, 1.0 / gamma);
    return color;
}

// --- Simple Reinhard Tone Mapping ---
float3 SimpleReinhardToneMapping(float3 color)
{
    float exposure = 1.5;
    color *= exposure / (1.0 + color / exposure);
    color = pow(color, 1.0 / gamma);
    return color;
}

// --- Luma-based Reinhard Tone Mapping ---
float3 LumaBasedReinhardToneMapping(float3 color)
{
    float3 lumaWeights = float3(0.2126, 0.7152, 0.0722);
    float luma = dot(color, lumaWeights);
    float toneMappedLuma = luma / (1.0 + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, 1.0 / gamma);
    return color;
}

// --- White-Preserving Luma-based Reinhard Tone Mapping ---
float3 WhitePreservingLumaBasedReinhardToneMapping(float3 color)
{
    float white = 2.0;
    float3 lumaWeights = float3(0.2126, 0.7152, 0.0722);
    float luma = dot(color, lumaWeights);
    float toneMappedLuma = luma * (1.0 + luma / (white * white)) / (1.0 + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, 1.0 / gamma);
    return color;
}

// --- RomBinDaHouse Tone Mapping ---
float3 RomBinDaHouseToneMapping(float3 color)
{
    color = exp(-1.0 / (2.72 * color + 0.15));
    color = pow(color, 1.0 / gamma);
    return color;
}

// --- Filmic Tone Mapping ---
float3 FilmicToneMapping(float3 color)
{
    color = max(0.0.xxx, color - 0.004.xxx);
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

// --- Uncharted 2 Tone Mapping ---
float3 Uncharted2ToneMapping(float3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposure = 2.0;

    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) /
            (color * (A * color + B) + D * F)) - E / F;

    float white = ((W * (A * W + C * B) + D * E) /
                  (W * (A * W + B) + D * F)) - E / F;

    color /= white;
    color = pow(color, 1.0 / gamma);
    return color;
}



float4 main(float4 pos : SV_Position) : SV_Target
{
    float2 src = finalPassData.resolution;
    float2 native = finalPassData.nativeResolution;

    float srcAspect = src.x / max(src.y, 1e-6);
    float dstAspect = native.x / max(native.y, 1e-6);

    float2 disp;
    if (srcAspect > dstAspect)
    {
        disp.x = native.x;
        disp.y = native.x / srcAspect;
    }
    else
    {
        disp.y = native.y;
        disp.x = native.y * srcAspect;
    }

    float2 tl = 0.5 * (native - disp);
    float2 br = tl + disp;

    if (pos.x < tl.x || pos.x >= br.x || pos.y < tl.y || pos.y >= br.y)
        return float4(0, 0, 0, 1);

    float2 uv = (pos.xy - tl) / disp;
    
    float strength = 24.0;
    float x = (uv.x + 4.0) * (uv.y + 4.0) * (finalPassData.time * 10.0);
    float4 grain = (fmod((fmod(x, 13.0) + 1.0) * (fmod(x, 123.0) + 1.0), 0.01) - 0.005) * strength;
    grain = 1.0 - grain;
    
    return float4(WhitePreservingLumaBasedReinhardToneMapping(frameInput.Sample(linearSampler, uv).rgb * grain.x), 1);

}