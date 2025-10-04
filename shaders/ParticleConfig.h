#ifndef _PARTICLE_CONFIG_H_
#define _PARTICLE_CONFIG_H_

//#define NUM_PARTICLES (32*1)

#ifdef IS_CPU
typedef ni::Float3 float3;
typedef ni::Float4x4 float4x4;
typedef ni::Float2 float2;
typedef uint32_t uint;
#endif

struct Material
{
	float3 albedo;
	float emissive;
	float reflection;
	float transparency;
	float indexOfRefraction;
};

struct ParticleData
{
	float3 position;
	float3 prevPosition;
	float3 velocity;
	float3 acceleration;
	float radius;
	float elasticity;
	float friction;
	uint id; // don't use this. It's for the temporal reprojection rejection
	uint dynamic;
	uint visible;
	Material material;
};

struct ParticleSceneData
{
	uint numParticles;
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
	uint sampleCount;
};

struct DepthOfFieldData
{
	float2 resolution;
	float focusPoint;
	float focusScale;
	float radiusScale;
	float blurSize;
	float maxDist;
};

struct SimulationData
{
	float3 cameraPos;
	uint scene;
	uint frame;// 0 = Don't run simulation (animation) and init particles for specific scene. >1 Run simulation for specific scene.
	float time;
};

struct AudioData
{
	float duration;
	uint sampleRate;
};

struct FinalPassData
{
	float2 resolution;
	float2 nativeResolution;
	float time;
	float brightness;
};

#ifndef IS_CPU
float toRad(float d) { return 3.14159265359f * d / 180.0f; }
#endif

#endif