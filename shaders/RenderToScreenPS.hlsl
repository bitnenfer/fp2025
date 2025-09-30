Texture2D<float4> inputTexture : register(t0);
SamplerState staticSampler : register(s0);

float4 main(float4 vert : SV_Position) : SV_TARGET
{
    float2 uv = vert.xy / float2(1920, 1080);
    float4 color = inputTexture.Sample(staticSampler, uv);
    return color;
}