#include "ni.h"

// Shaders
#include "../tmp/shaders/SimulateCS.h"
#include "../tmp/shaders/ParticleBasePassCS.h"
#include "../tmp/shaders/TemporalAACS.h"
#include "../tmp/shaders/TemporalReprojectionCS.h"
#include "../tmp/shaders/ATrousFilterCS.h"
#include "../tmp/shaders/DepthOfFieldPS.h"
#include "../tmp/shaders/TransferToBackbufferPS.h"
#include "../tmp/shaders/AudioProcessCS.h"

#define IS_CPU 1
#include "../shaders/ParticleConfig.h"

#include "render.h"
#include "audio.h"

inline UINT PIX_COLOR_INDEX(BYTE i) { return 0x00000000 | i; }

#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080

#define MAX_PARTICLE_NUM (32*10)

int main()
{
	ni::init(0, 0, 1920, 1080, "FP2025", !NI_DEBUG, NI_DEBUG);

#if NI_DEBUG
	typedef void(WINAPI* BeginEventOnCommandList)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
	typedef void(WINAPI* EndEventOnCommandList)(ID3D12GraphicsCommandList* commandList);
	typedef void(WINAPI* SetMarkerOnCommandList)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
	HMODULE module = LoadLibrary(L"thirdparty/WinPixEventRuntime.dll");
	BeginEventOnCommandList pixBeginEventOnCommandList = (BeginEventOnCommandList)GetProcAddress(module, "PIXBeginEventOnCommandList");
	EndEventOnCommandList   pixEndEventOnCommandList = (EndEventOnCommandList)GetProcAddress(module, "PIXEndEventOnCommandList");
	SetMarkerOnCommandList  pixSetMarkerOnCommandList = (SetMarkerOnCommandList)GetProcAddress(module, "PIXSetMarkerOnCommandList");
#else
#define pixBeginEventOnCommandList(...)
#define pixEndEventOnCommandList(...)
#define pixSetMarkerOnCommandList(...)
	ShowCursor(0);
#endif
	// Audio Renderer
	//AudioRenderer* audioRenderer = new AudioRenderer(3*60);

	// Initialize particle simulation
	ni::FlyCamera camera(ni::Float3(0, -0, -80));
	camera.speedScale = 0.15f;

	ConstantBufferUploader<ParticleSceneData>* particleSceneCB = new ConstantBufferUploader<ParticleSceneData>();
	particleSceneCB->data.numParticles = 64;

	ni::Buffer* particleBuffer = ni::createBuffer(sizeof(ParticleData) * MAX_PARTICLE_NUM, ni::BufferType::UNORDERED_BUFFER, nullptr, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::DescriptorAllocator* descriptorAllocator = ni::createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	ni::ComputePipelineDesc simulateParticlesDesc = {};
	simulateParticlesDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	simulateParticlesDesc.shader = { SimulateCS, sizeof(SimulateCS) };
	ni::PipelineState* simulateParticles = ni::buildComputePipelineState(simulateParticlesDesc);
	ConstantBufferUploader<SimulationData>* simulationCB = new ConstantBufferUploader<SimulationData>();
	particleBuffer->resource.apiResource->SetName(L"ParticleBuffer");

	// Particle Base Pass
	ni::ComputePipelineDesc basePassParticleDesc = {};
	basePassParticleDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 5, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	basePassParticleDesc.shader = { ParticleBasePassCS, sizeof(ParticleBasePassCS) };
	ni::PipelineState* basePassParticle = ni::buildComputePipelineState(basePassParticleDesc);
	ni::Texture* outputTexture = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* velocityBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* positionBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* normalBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ConstantBufferUploader<ConstantBufferData>* sceneRenderCB = new ConstantBufferUploader<ConstantBufferData>();

	ni::ResourceBarrierBatcher resourceBarrier;

	ni::Float4x4 projMtx;
	projMtx.perspective(ni::toRad(60.0f), (float)RENDER_WIDTH / (float)RENDER_HEIGHT, 0.1f, 1000.0f);
	sceneRenderCB->data.resolution = ni::Float3(RENDER_WIDTH, RENDER_HEIGHT, 1.0f);
	sceneRenderCB->data.time = 0.0f;
	sceneRenderCB->data.cameraPos = camera.position;
	sceneRenderCB->data.viewMtx = camera.makeViewMatrix();
	sceneRenderCB->data.viewProjMtx = projMtx * sceneRenderCB->data.viewMtx;
	sceneRenderCB->data.invViewProjMtx = sceneRenderCB->data.viewProjMtx;
	sceneRenderCB->data.invViewProjMtx.transpose().invert();
	sceneRenderCB->data.prevCameraPos = sceneRenderCB->data.cameraPos;
	sceneRenderCB->data.prevInvViewProjMtx = sceneRenderCB->data.invViewProjMtx;
	sceneRenderCB->data.prevViewProjMtx = sceneRenderCB->data.viewProjMtx;
	sceneRenderCB->data.frame = 0.0f;
	sceneRenderCB->data.sampleCount = 4;

	ni::DescriptorAllocator* rtvDescriptorAllocator = ni::createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// Temporal Reprojection
	ni::Texture* prevPositionBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevNormalBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* historyM1Buffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* historyM2Buffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevHistoryM1Buffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevHistoryM2Buffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* depthBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevDepthBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* historyBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, DXGI_FORMAT_R16G16B16A16_FLOAT);
	ni::Texture* resultTemporalReprojection = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ni::ComputePipelineDesc temporalReprojectionDesc = {};
	temporalReprojectionDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	temporalReprojectionDesc.layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	temporalReprojectionDesc.layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	temporalReprojectionDesc.shader = { TemporalReprojectionCS, sizeof(TemporalReprojectionCS) };
	ni::PipelineState* temporalReprojection = ni::buildComputePipelineState(temporalReprojectionDesc);

	// A-Trous Filter
	ni::ComputePipelineDesc AtrousFilterDesc = {};
	AtrousFilterDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.layout.add32BitConstant(0, 0, 4, D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.shader = { ATrousFilterCS, sizeof(ATrousFilterCS) };
	ni::PipelineState* atrousFilter = ni::buildComputePipelineState(AtrousFilterDesc);

	// Depth of Field
	FullscreenRasterPass* depthOfFieldPass = new FullscreenRasterPass();
	depthOfFieldPass->pixel.shader = { DepthOfFieldPS, sizeof(DepthOfFieldPS) };
	depthOfFieldPass->pixel.renderTargets.add(DXGI_FORMAT_R16G16B16A16_FLOAT);
	depthOfFieldPass->layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_PIXEL);
	depthOfFieldPass->layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	depthOfFieldPass->build();

	ConstantBufferUploader<DepthOfFieldData>* depthOfFieldCB = new ConstantBufferUploader<DepthOfFieldData>();
	depthOfFieldCB->data.focusScale = 18.0f;
	depthOfFieldCB->data.focusPoint = 22.0f;
	depthOfFieldCB->data.radiusScale = 1.5f;
	depthOfFieldCB->data.blurSize = 20.0f;
	depthOfFieldCB->data.maxDist = 1000.0f;
	depthOfFieldCB->data.resolution = float2(sceneRenderCB->data.resolution.x, sceneRenderCB->data.resolution.y);

	ni::Texture* finalBuffer = ni::createTexture(RENDER_WIDTH, RENDER_HEIGHT, 1, nullptr, D3D12_RESOURCE_STATE_RENDER_TARGET, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	FullscreenRasterPass* transferToBackBufferPass = new FullscreenRasterPass();
	transferToBackBufferPass->pixel.shader = { TransferToBackbufferPS, sizeof(TransferToBackbufferPS) };
	transferToBackBufferPass->pixel.renderTargets.add(DXGI_FORMAT_R8G8B8A8_UNORM);
	transferToBackBufferPass->layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_PIXEL);
	transferToBackBufferPass->layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	transferToBackBufferPass->build();
	ConstantBufferUploader<FinalPassData>* transferToBackBufferCB = new ConstantBufferUploader<FinalPassData>();
	transferToBackBufferCB->data.nativeResolution = ni::Float2(ni::getViewWidth(), ni::getViewHeight());
	transferToBackBufferCB->data.resolution = ni::Float2(RENDER_WIDTH, RENDER_HEIGHT);
	transferToBackBufferCB->data.time = 0.0f;



	while (!ni::shouldQuit())
	{
		ni::pollEvents();


		//NI_LOG("wheel %f", ni::mouseWheelY());

		if (ni::FrameData* frame = ni::beginFrame())
		{
			uint32_t pixColorIndex = 0;
			ID3D12GraphicsCommandList* commandList = frame->commandList;
			ni::DescriptorTable rtvDescriptorTable = rtvDescriptorAllocator->allocateDescriptorTable(2);
			resourceBarrier.transition(finalBuffer->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
			resourceBarrier.flush(commandList);
			rtvDescriptorTable.allocRTVTex2D(finalBuffer->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0);
			rtvDescriptorTable.allocRTVTex2D(ni::getCurrentBackbuffer()->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			float clearColor[] = { 0, 0, 0, 1 };
			commandList->ClearRenderTargetView(rtvDescriptorTable.cpuHandle(0), clearColor, 0, nullptr);
			commandList->ClearRenderTargetView(rtvDescriptorTable.cpuHandle(1), clearColor, 0, nullptr);
			commandList->SetDescriptorHeaps(1, &descriptorAllocator->descriptorHeap);

			/*if (!audioRenderer->hasFinishedRendering())
			{
				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Process Audio");
				audioRenderer->renderAudioBufferGPU(commandList, descriptorAllocator, resourceBarrier);
				pixEndEventOnCommandList(commandList);
			}
			else*/
			{
				/*if (!audioRenderer->isPlaying())
				{
					audioRenderer->play();
				}*/

				if (ni::keyDown(ni::SHIFT))
				{
					camera.speedScale = 0.35f;
				}
				else
				{
					camera.speedScale = 0.15f;
				}

				// Update CPU scene data
				camera.update();
				sceneRenderCB->data.cameraPos = camera.position;
				sceneRenderCB->data.viewMtx = camera.makeViewMatrix();
				sceneRenderCB->data.viewProjMtx = projMtx * sceneRenderCB->data.viewMtx;
				sceneRenderCB->data.invViewProjMtx = sceneRenderCB->data.viewProjMtx;
				sceneRenderCB->data.invViewProjMtx.transpose().invert();
				transferToBackBufferCB->data.time = sceneRenderCB->data.time;
				if (ni::keyDown(ni::UP))
				{
					depthOfFieldCB->data.focusPoint += 0.1f;
					NI_LOG("focusPoint: %.4f", depthOfFieldCB->data.focusPoint);
				}
				else if (ni::keyDown(ni::DOWN))
				{
					depthOfFieldCB->data.focusPoint -= 0.1f;
					NI_LOG("focusPoint: %.4f", depthOfFieldCB->data.focusPoint);
				}

				if (ni::keyDown(ni::RIGHT))
				{
					depthOfFieldCB->data.focusScale += 0.1f;
					NI_LOG("focusScale: %.4f", depthOfFieldCB->data.focusScale);
				}
				else if (ni::keyDown(ni::LEFT))
				{
					depthOfFieldCB->data.focusScale -= 0.1f;
					NI_LOG("focusScale: %.4f", depthOfFieldCB->data.focusScale);
				}

				depthOfFieldCB->data.focusPoint += ni::mouseWheelY() * 1.0f;

				if (ni::mouseClick(ni::MOUSE_BUTTON_RIGHT) || sceneRenderCB->data.time == 0.0f)
				{
					simulationCB->data.time = 0;
					simulationCB->data.frame = 0;
				}

				simulationCB->data.cameraPos = camera.position;

				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Update CBs");
				depthOfFieldCB->update(commandList);
				simulationCB->update(commandList);
				sceneRenderCB->update(commandList);
				transferToBackBufferCB->update(commandList);
				particleSceneCB->update(commandList);
				pixEndEventOnCommandList(commandList);

				// Run Simulation
				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Particle Simulation");
				commandList->SetPipelineState(simulateParticles->pso);
				commandList->SetComputeRootSignature(simulateParticles->rootSignature);
				ni::DescriptorTable simulateParticlesDescriptorTable = descriptorAllocator->allocateDescriptorTable(3);
				simulateParticlesDescriptorTable.allocUAVBuffer(particleBuffer->resource, nullptr, DXGI_FORMAT_UNKNOWN, 0, MAX_PARTICLE_NUM, sizeof(ParticleData), 0);
				simulateParticlesDescriptorTable.allocCBVBuffer(simulationCB->buffer->resource, simulationCB->buffer->resource.apiResource->GetDesc().Width);
				simulateParticlesDescriptorTable.allocCBVBuffer(particleSceneCB->buffer->resource, particleSceneCB->buffer->resource.apiResource->GetDesc().Width);
				commandList->SetComputeRootDescriptorTable(0, simulateParticlesDescriptorTable.gpuBaseHandle);
				commandList->Dispatch(particleSceneCB->data.numParticles / 32, 1, 1);

				resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.flush(commandList);
				pixEndEventOnCommandList(commandList);

				// Render Scene
				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Render Base Scene");
				commandList->SetPipelineState(basePassParticle->pso);
				commandList->SetComputeRootSignature(basePassParticle->rootSignature);
				ni::DescriptorTable basePassDescriptorTable = descriptorAllocator->allocateDescriptorTable(9);
				basePassDescriptorTable.allocSRVBuffer(particleBuffer->resource, DXGI_FORMAT_UNKNOWN, 0, MAX_PARTICLE_NUM, sizeof(ParticleData));
				basePassDescriptorTable.allocUAVTex2D(outputTexture->resource, nullptr, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0);
				basePassDescriptorTable.allocUAVTex2D(velocityBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
				basePassDescriptorTable.allocUAVTex2D(positionBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
				basePassDescriptorTable.allocUAVTex2D(normalBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
				basePassDescriptorTable.allocUAVTex2D(depthBuffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
				basePassDescriptorTable.allocCBVBuffer(sceneRenderCB->buffer->resource, sceneRenderCB->buffer->resource.apiResource->GetDesc().Width);
				basePassDescriptorTable.allocCBVBuffer(particleSceneCB->buffer->resource, particleSceneCB->buffer->resource.apiResource->GetDesc().Width);
				basePassDescriptorTable.allocCBVBuffer(simulationCB->buffer->resource, simulationCB->buffer->resource.apiResource->GetDesc().Width);
				commandList->SetComputeRootDescriptorTable(0, basePassDescriptorTable.gpuBaseHandle);
				commandList->Dispatch((uint32_t)(sceneRenderCB->data.resolution.x / 32.0f), (uint32_t)(sceneRenderCB->data.resolution.y / 32.0f) + 1, 1);
				pixEndEventOnCommandList(commandList);

				// Temporal Reprojection
				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Temporal Reprojection");
				resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(historyBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(prevHistoryM1Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(prevHistoryM2Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(depthBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(prevDepthBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(resultTemporalReprojection->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.flush(commandList);

				commandList->SetPipelineState(temporalReprojection->pso);
				commandList->SetComputeRootSignature(temporalReprojection->rootSignature);
				ni::DescriptorTable temporalReprojectionDescriptorTable = descriptorAllocator->allocateDescriptorTable(15);
				temporalReprojectionDescriptorTable.allocSRVTex2D(outputTexture->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(historyBuffer->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(positionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(normalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(prevPositionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(prevNormalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM1Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM2Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocSRVTex2D(prevDepthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				temporalReprojectionDescriptorTable.allocUAVTex2D(resultTemporalReprojection->resource, nullptr, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0);
				temporalReprojectionDescriptorTable.allocUAVTex2D(historyM1Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
				temporalReprojectionDescriptorTable.allocUAVTex2D(historyM2Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
				temporalReprojectionDescriptorTable.allocCBVBuffer(sceneRenderCB->buffer->resource, sceneRenderCB->buffer->resource.apiResource->GetDesc().Width);
				commandList->SetComputeRootDescriptorTable(0, temporalReprojectionDescriptorTable.gpuBaseHandle);
				commandList->Dispatch((uint32_t)(sceneRenderCB->data.resolution.x / 32.0f), (uint32_t)(sceneRenderCB->data.resolution.y / 32.0f) + 1, 1);

				resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.flush(commandList);
				pixEndEventOnCommandList(commandList);

				// A-trous filter passes
				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "A-Trous Filter Passes");
				ni::Texture* atrousInputOutput[2] = { resultTemporalReprojection, outputTexture };

				commandList->SetPipelineState(atrousFilter->pso);
				commandList->SetComputeRootSignature(atrousFilter->rootSignature);

				ni::Texture* input = atrousInputOutput[0];
				ni::Texture* output = atrousInputOutput[1];

				for (uint32_t index = 0; index < 2; ++index)
				{
					input = atrousInputOutput[index % 2];
					output = atrousInputOutput[(index + 1) % 2];

					resourceBarrier.transition(input->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					resourceBarrier.transition(output->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					resourceBarrier.flush(commandList);

					ni::DescriptorTable atrousFilterDescriptorTable = descriptorAllocator->allocateDescriptorTable(7);
					atrousFilterDescriptorTable.allocSRVTex2D(input->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 1, 0, 0.0f);
					atrousFilterDescriptorTable.allocSRVTex2D(normalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
					atrousFilterDescriptorTable.allocSRVTex2D(historyM1Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
					atrousFilterDescriptorTable.allocSRVTex2D(historyM2Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
					atrousFilterDescriptorTable.allocSRVTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
					atrousFilterDescriptorTable.allocUAVTex2D(output->resource, nullptr, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0);

					struct Constant
					{
						uint32_t step;
						ni::Float3 resolution;
					} constantData = { index + 1, sceneRenderCB->data.resolution };

					commandList->SetComputeRootDescriptorTable(0, atrousFilterDescriptorTable.gpuBaseHandle);
					commandList->SetComputeRoot32BitConstants(1, ni::calcNumUint32FromSize(sizeof(Constant)), &constantData, 0);
					commandList->Dispatch((uint32_t)(sceneRenderCB->data.resolution.x / 32.0f), (uint32_t)(sceneRenderCB->data.resolution.y / 32.0f) + 1, 1);
				}

				pixEndEventOnCommandList(commandList);

				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Copy Next Frame Resources");
				resourceBarrier.transition(historyBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.transition(resultTemporalReprojection->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.flush(commandList);
				commandList->CopyResource(historyBuffer->resource.apiResource, resultTemporalReprojection->resource.apiResource);

				resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(output->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.transition(prevHistoryM1Buffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.transition(prevHistoryM2Buffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.transition(depthBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
				resourceBarrier.transition(prevDepthBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.flush(commandList);

				commandList->CopyResource(prevPositionBuffer->resource.apiResource, positionBuffer->resource.apiResource);
				commandList->CopyResource(prevNormalBuffer->resource.apiResource, normalBuffer->resource.apiResource);
				commandList->CopyResource(prevHistoryM1Buffer->resource.apiResource, historyM1Buffer->resource.apiResource);
				commandList->CopyResource(prevHistoryM2Buffer->resource.apiResource, historyM2Buffer->resource.apiResource);
				commandList->CopyResource(prevDepthBuffer->resource.apiResource, depthBuffer->resource.apiResource);
				pixEndEventOnCommandList(commandList);

				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Depth of Field");
				ni::Array<ResourceDesc> depthOfFieldPassParams;
				depthOfFieldPassParams.add(ResourceDesc::srvTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f));
				depthOfFieldPassParams.add(ResourceDesc::srvTex2D(output->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 1, 0, 0.0f));
				depthOfFieldPassParams.add(ResourceDesc::cbvBuffer(depthOfFieldCB->buffer->resource, depthOfFieldCB->buffer->resource.apiResource->GetDesc().Width));
				depthOfFieldPass->draw(RENDER_WIDTH, RENDER_HEIGHT, commandList, rtvDescriptorTable.cpuBaseHandle, descriptorAllocator, depthOfFieldPassParams);
				resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(depthBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.transition(finalBuffer->resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
				resourceBarrier.flush(commandList);
				pixEndEventOnCommandList(commandList);

				pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(pixColorIndex++), "Render to Backbuffer");
				ni::Array<ResourceDesc> transferToBackbufferParams;
				transferToBackbufferParams.add(ResourceDesc::srvTex2D(finalBuffer->resource, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 1, 0, 0.0f));
				transferToBackbufferParams.add(ResourceDesc::cbvBuffer(transferToBackBufferCB->buffer->resource, transferToBackBufferCB->buffer->resource.apiResource->GetDesc().Width));
				transferToBackBufferPass->draw(ni::getViewWidthUint(), ni::getViewHeightUint(), commandList, rtvDescriptorTable.cpuHandle(1), descriptorAllocator, transferToBackbufferParams);
				resourceBarrier.transition(finalBuffer->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
				pixEndEventOnCommandList(commandList);

				sceneRenderCB->data.time += 0.016f;
				sceneRenderCB->data.frame += 1.0f;
				sceneRenderCB->data.prevCameraPos = sceneRenderCB->data.cameraPos;
				sceneRenderCB->data.prevInvViewProjMtx = sceneRenderCB->data.invViewProjMtx;
				sceneRenderCB->data.prevViewProjMtx = sceneRenderCB->data.viewProjMtx;
				simulationCB->data.time += 0.016f;
				simulationCB->data.frame++;
			}

			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_PRESENT);
			resourceBarrier.flush(commandList);

			ni::endFrame();
			//audioRenderer->processCPU();

			rtvDescriptorAllocator->reset();
			descriptorAllocator->reset();


		}

		ni::present(!false);
	}

	ni::waitForAllFrames();
	delete depthOfFieldPass;
	delete depthOfFieldCB;
	delete simulationCB;
	delete sceneRenderCB;
	delete transferToBackBufferPass;
	delete transferToBackBufferCB;
	delete particleSceneCB;
	//delete audioRenderer;

	ni::destroyPipelineState(atrousFilter);
	ni::destroyPipelineState(temporalReprojection);
	ni::destroyTexture(depthBuffer);
	ni::destroyTexture(prevDepthBuffer);
	ni::destroyTexture(historyM1Buffer);
	ni::destroyTexture(historyM2Buffer);
	ni::destroyTexture(prevHistoryM1Buffer);
	ni::destroyTexture(prevHistoryM2Buffer);
	ni::destroyTexture(prevPositionBuffer);
	ni::destroyTexture(prevNormalBuffer);
	ni::destroyTexture(positionBuffer);
	ni::destroyTexture(normalBuffer);
	ni::destroyTexture(resultTemporalReprojection);
	ni::destroyTexture(historyBuffer);
	ni::destroyDescriptorAllocator(rtvDescriptorAllocator);
	ni::destroyTexture(outputTexture);
	ni::destroyTexture(velocityBuffer);
	ni::destroyPipelineState(basePassParticle);
	ni::destroyPipelineState(simulateParticles);
	ni::destroyDescriptorAllocator(descriptorAllocator);
	ni::destroyBuffer(particleBuffer);
	ni::destroyTexture(finalBuffer);
	ni::destroy();

	return 0;
}