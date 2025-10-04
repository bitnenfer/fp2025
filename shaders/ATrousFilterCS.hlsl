Texture2D<float4> currentFrame : register(t0);
Texture2D<float4> normalBuffer : register(t1);
Texture2D<float> M1 : register(t2);
Texture2D<float> M2 : register(t3);
Texture2D<float> depthBuffer : register(t4);

RWTexture2D<float4> resultTexture : register(u0);

const int iterStep;
const float3 resolution;

float3 AtrousIter(int2 px, int step)
{
    static const int2 taps[25] =
    {
        int2(-2, -2), int2(-1, -2), int2(0, -2), int2(1, -2), int2(2, -2),
        int2(-2, -1), int2(-1, -1), int2(0, -1), int2(1, -1), int2(2, -1),
        int2(-2, 0), int2(-1, 0), int2(0, 0), int2(1, 0), int2(2, 0),
        int2(-2, 1), int2(-1, 1), int2(0, 1), int2(1, 1), int2(2, 1),
        int2(-2, 2), int2(-1, 2), int2(0, 2), int2(1, 2), int2(2, 2)
    };
    float3 Cc = currentFrame[px].rgb;
    float3 Nc = normalBuffer[px].xyz;
    float Zc = depthBuffer[px].r; // linear
    float Lc = dot(Cc, float3(0.299, 0.587, 0.114));
    float sigma = sqrt(max(M2[px] - M1[px] * M1[px], 0));

    float3 sum = 0;
    float wsum = 0;
    [unroll]
    for (int i = 0; i < 25; i++)
    {
        int2 q = clamp(px + taps[i] * step, int2(0, 0), int2(resolution.xy) - 1);
        float3 Cq = currentFrame[q].rgb;
        float3 Nq = normalBuffer[q].xyz;
        float Zq = depthBuffer[q].r;
        float Lq = dot(Cq, float3(0.299, 0.587, 0.114));

        float wn = pow(saturate(dot(Nc, Nq)), 48.0);
        float wz = exp(-abs(Zc - Zq) / 0.002); // or a fixed 0.002–0.01
        float wl = exp(-abs(Lc - Lq) / max(1e-3, 1.25 * sigma));

        float w = wn * wz * wl;
        sum += w * Cq;
        wsum += w;
    }
    return sum / max(wsum, 1e-6);
}


[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= resolution.x || DTid.y >= resolution.y)
        return;
    
    resultTexture[DTid.xy] = float4(AtrousIter(DTid.xy, iterStep), 1);
    
}