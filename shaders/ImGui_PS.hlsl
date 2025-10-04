Texture2D<float4> BaseTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VertexOut
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 Tint : TEXCOORD1;
};

float4 main(VertexOut In) : SV_TARGET
{
    float4 TexColor = BaseTexture.Sample(LinearSampler, In.TexCoord);
	return TexColor * In.Tint;
}
