cbuffer Uniforms : register(b0)
{
	float4x4 ViewProjection;
};

struct VertexIn
{
	float2 Position : POSITION0;
	float4 Tint : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 Tint : TEXCOORD1;
};

VertexOut main(VertexIn In)
{
	VertexOut Out;
	Out.Position = mul(ViewProjection, float4(In.Position, 0.0, 1.0));
	Out.TexCoord = In.TexCoord;
	Out.Tint = In.Tint;
	return Out;
}
