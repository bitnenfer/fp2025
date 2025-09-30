#define NUM_PARTICLES (32*6)

#ifdef IS_CPU
typedef ni::Float3 float3;
typedef ni::Float4x4 float4x4;
typedef uint32_t uint;
#endif

struct ParticleData
{
	float3 position;
	float3 prevPosition;
	float3 velocity;
	float3 acceleration;
	float3 albedo;
	float radius;
	float elasticity;
	float friction;
	float reflection;
	float emissive;
	uint id;
	uint dynamic;
	uint visible;
};

struct ConstantBufferData
{
	float3 cameraPos;
	float _padding0;
	float4x4 invViewProjMtx;
	float3 prevCameraPos;
	float _padding1;
	float4x4 prevInvViewProjMtx;
	float4x4 viewMtx;
	float4x4 viewProjMtx;
	float4x4 prevViewProjMtx;
	float3 resolution;
	float time;
	float frame;
};

#ifndef IS_CPU
float toRad(float d) { return 3.14159265359f * d / 180.0f; }
#endif