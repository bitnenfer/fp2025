#include "ParticleConfig.h"

Texture2D<float4> CurrentFrame : register(t0);
Texture2D<float4> VelocityBuffer : register(t1);
Texture2D<float4> HistoryBuffer : register(t2);
Texture2D<float4> PositionBuffer : register(t3);
Texture2D<float4> NormalBuffer : register(t4);
Texture2D<float4> PrevPositionBuffer : register(t5);
Texture2D<float4> PrevNormalBuffer : register(t6);
Texture2D<float> HistoryM1Prev : register(t7);
Texture2D<float> HistoryM2Prev : register(t8);

RWTexture2D<float4> ResultTexture : register(u0);
RWTexture2D<float> HistoryM1Out : register(u1);
RWTexture2D<float> HistoryM2Out : register(u2);

ConstantBuffer<ConstantBufferData> ConstantData : register(b0);

SamplerState linearClamp : register(s0);

float Luma(float3 c)
{
    return dot(c, float3(0.299, 0.587, 0.114));
}

float LinearDepthFromPos(float3 worldPos)
{
    // View-space Z from position (positive forward depends on your convention).
    // Here we compute |camera->point| as a simple linear depth proxy:
    return distance(worldPos, ConstantData.cameraPos);
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint2 px = DTid.xy;
    if (px.x >= uint(ConstantData.resolution.x) || px.y >= uint(ConstantData.resolution.y))
        return;
    
    float2 uv = (float2(px) + 0.5) / ConstantData.resolution.xy;
    float3 ccurr = CurrentFrame.Load(int3(px, 0)).rgb;
    float3 ncurr = normalize(NormalBuffer.Load(float3(px, 0)).rgb);
    float3 pcurr = PositionBuffer.Load(float3(px, 0)).rgb;
    float zcurr = LinearDepthFromPos(pcurr);
    
    float2 vel = VelocityBuffer.Load(int3(px, 0)).xy / ConstantData.resolution.xy;
    float2 uvPrev = uv + vel;
    
    bool valid = all(uvPrev > 0.0) && all(uvPrev <= 1.0);
    
    float3 cprev = 0;
    float m1prev = 0;
    float m2prev = 0;
    
    if (valid)
    {
        cprev = HistoryBuffer.SampleLevel(linearClamp, uvPrev, 0).rgb;
        m1prev = HistoryM1Prev.SampleLevel(linearClamp, uvPrev, 0).r;
        m2prev = HistoryM2Prev.SampleLevel(linearClamp, uvPrev, 0).r;
        float3 nprev = normalize(PrevNormalBuffer.SampleLevel(linearClamp, uvPrev, 0).rgb);
        float3 pprev = PrevPositionBuffer.SampleLevel(linearClamp, uvPrev, 0).rgb;
        float zprev = LinearDepthFromPos(pprev);
        float ndot = dot(ncurr, nprev);
        float dz = abs(zcurr - zprev);
        float dzThr = max(0.002 * zcurr, 0.002 * 0.5);
        
        valid = valid && (ndot > 0.9) && (dz < dzThr);

    }

    float Ymin = +1e9, Ymax = -1e9, sum = 0, sum2 = 0;
    
    [unroll]
    for (int oy = -1; oy <= 1; ++oy)
    {
        [unroll]
        for (int ox = -1; ox <= 1; ++ox)
        {
            int2 q = clamp(int2(px) + int2(ox, oy), int2(0, 0), int2(ConstantData.resolution.xy) - 1);
            float3 Cn = CurrentFrame.Load(int3(q, 0)).rgb;
            float y = Luma(Cn);
            Ymin = min(Ymin, y);
            Ymax = max(Ymax, y);
            sum += y;
            sum2 += y * y;
        }
    }
    
    float mean = sum / 9.0;
    float var = max(sum2 / 9.0 - mean * mean, 0);
    float pad = 1.0 * sqrt(var); // tune 0.5–2.0
    Ymin -= pad;
    Ymax += pad;
    
    // Clamp history luminance into current range
    float3 Chist = cprev;
    if (valid)
    {
        float Yh = Luma(Chist);
        float Yhc = clamp(Yh, Ymin, Ymax);
        float s = (Yh > 1e-4) ? (Yhc / Yh) : 1.0;
        Chist *= s;
    }
    
    float Ycurr = Luma(ccurr);
    float3 Cout;
    float M1, M2;
    
    if (!valid)
    {
        Cout = ccurr;
        M1 = Ycurr;
        M2 = Ycurr * Ycurr;
    }
    else
    {
        Cout = lerp(ccurr, Chist, 1 - 0.15);
        M1 = lerp(Ycurr, m1prev, 1 - 0.15);
        M2 = lerp(Ycurr * Ycurr, m2prev, 1 - 0.15);
    }
    ResultTexture[px] = float4(Cout, 1);
    HistoryM1Out[px] = M1;
    HistoryM2Out[px] = M2;
}