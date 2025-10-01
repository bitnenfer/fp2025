
#include "ParticleConfig.h"

Texture2D<float4> Frame;
const float4 Resolution : register(b0);
SamplerState Sampler;

float4 main(float4 pos : SV_Position) : SV_Target
{
    float2 Uv = pos.xy / Resolution.xy;
    return Frame.Sample(Sampler, Uv);
}