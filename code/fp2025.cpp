#include "ni.h"

// Shaders
#include "../shaders/InitParticlesCS.h"
#include "../shaders/SimulateCS.h"
#include "../shaders/ParticleBasePassCS.h"
#include "../shaders/TemporalAACS.h"
#include "../shaders/RenderToScreenVS.h"
#include "../shaders/RenderToScreenPS.h"
#include "../shaders/TemporalReprojectionCS.h"
#include "../shaders/ATrousFilterCS.h"

#define IS_CPU 1
#include "../shaders/ParticleConfig.h"


int main()
{
	ni::init(1920, 1080, "FP2025", !true, true);

	// Initialize particles
	ni::ComputePipelineDesc initParticlesDesc = {};
	initParticlesDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	initParticlesDesc.layout.add32BitConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
	initParticlesDesc.shader = { InitParticlesCS, sizeof(InitParticlesCS) };
	ni::PipelineState* initParticles = ni::buildComputePipelineState(initParticlesDesc);
	ni::Buffer* particleBuffer = ni::createBuffer(sizeof(ParticleData) * NUM_PARTICLES, ni::BufferType::UNORDERED_BUFFER, nullptr, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::DescriptorAllocator* descriptorAllocator = ni::createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	// Initialize particle simulation
	ni::ComputePipelineDesc simulateParticlesDesc = {};
	simulateParticlesDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	simulateParticlesDesc.shader = { SimulateCS, sizeof(SimulateCS) };
	ni::PipelineState* simulateParticles = ni::buildComputePipelineState(simulateParticlesDesc);

	// Particle Base Pass
	ni::ComputePipelineDesc basePassParticleDesc = {};
	basePassParticleDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	basePassParticleDesc.shader = { ParticleBasePassCS, sizeof(ParticleBasePassCS) };
	ni::PipelineState* basePassParticle = ni::buildComputePipelineState(basePassParticleDesc);
	ni::Texture* outputTexture = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* velocityBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* positionBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* normalBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ConstantBufferData sceneConstantBufferData = {};
	ni::Float4x4 projMtx;
	projMtx.perspective(ni::toRad(30.0f), ni::getViewAspectRatio(), 0.1f, 1000.0f);
	ni::FlyCamera camera(ni::Float3(0, -20, -200));
	camera.speedScale = 10.0f;
	sceneConstantBufferData.resolution = ni::Float3(ni::getViewWidth(), ni::getViewHeight(), 1.0f);
	sceneConstantBufferData.time = 0.0f;
	ni::Buffer* sceneConstantBuffer = ni::createBuffer(sizeof(ConstantBufferData), ni::CONSTANT_BUFFER);
	ni::Buffer* sceneConstantBufferUpload = ni::createBuffer(sizeof(ConstantBufferData), ni::UPLOAD_BUFFER);

	ni::ResourceBarrierBatcher resourceBarrier;

	sceneConstantBufferData.cameraPos = camera.position;
	sceneConstantBufferData.viewMtx = camera.makeViewMatrix();
	sceneConstantBufferData.viewProjMtx = projMtx * sceneConstantBufferData.viewMtx;
	sceneConstantBufferData.invViewProjMtx = sceneConstantBufferData.viewProjMtx;
	sceneConstantBufferData.invViewProjMtx.transpose().invert();
	sceneConstantBufferData.prevCameraPos = sceneConstantBufferData.cameraPos;
	sceneConstantBufferData.prevInvViewProjMtx = sceneConstantBufferData.invViewProjMtx;
	sceneConstantBufferData.prevViewProjMtx = sceneConstantBufferData.viewProjMtx;
	sceneConstantBufferData.frame = 0.0f;

	ni::DescriptorAllocator* rtvDescriptorAllocator = ni::createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// Temporal Reprojection
	ni::Texture* prevPositionBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevNormalBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* historyM1Buffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* historyM2Buffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevHistoryM1Buffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevHistoryM2Buffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ni::ComputePipelineDesc temporalReprojectionDesc = {};
	temporalReprojectionDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 0, 0),
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
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.layout.add32BitConstant(0, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.layout.add32BitConstant(1, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.shader = { ATrousFilterCS, sizeof(ATrousFilterCS) };
	ni::PipelineState* AtrousFilter = ni::buildComputePipelineState(AtrousFilterDesc);

	// Temporal AA Setup
	ni::ComputePipelineDesc temporalAADesc = {};
	temporalAADesc.shader = { TemporalAACS, sizeof(TemporalAACS) };
	temporalAADesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	temporalAADesc.layout.add32BitConstant(0, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
	ni::PipelineState* temporalAA = ni::buildComputePipelineState(temporalAADesc);
	ni::Texture* historyBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ni::Texture* resultTAA = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	
	// Render to screen
	ni::Float3 triangle[] = { {-1.0f, -7.0f, 0.0f }, {-1.0f, 7.0f, 0.0f}, {7.0f, 7.0f, 0.0f} };
	ni::Buffer* triangleVB = ni::createBuffer(sizeof(triangle), ni::VERTEX_BUFFER, triangle);
	ni::GraphicsPipelineDesc renderToScreenDesc = {};
	renderToScreenDesc.rasterizer.cullMode = D3D12_CULL_MODE_NONE;
	renderToScreenDesc.vertex.shader = { RenderToScreenVS, sizeof(RenderToScreenVS) };
	renderToScreenDesc.vertex.addVertexBuffer(sizeof(ni::Float3)).addVertexAttribute(DXGI_FORMAT_R32G32B32_FLOAT, "POSITION", 0, 0);
	renderToScreenDesc.pixel.shader = { RenderToScreenPS, sizeof(RenderToScreenPS) };
	renderToScreenDesc.pixel.renderTargets.add(DXGI_FORMAT_R8G8B8A8_UNORM);
	renderToScreenDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_PIXEL);
	renderToScreenDesc.layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	ni::PipelineState* renderToScreen = ni::buildGraphicsPipelineState(renderToScreenDesc);

	while (!ni::shouldQuit())
	{
		ni::pollEvents();

		// Update CPU scene data
		camera.update();
		sceneConstantBufferData.cameraPos = camera.position;
		sceneConstantBufferData.viewMtx = camera.makeViewMatrix();
		sceneConstantBufferData.viewProjMtx = projMtx* sceneConstantBufferData.viewMtx;
		sceneConstantBufferData.invViewProjMtx = sceneConstantBufferData.viewProjMtx;
		sceneConstantBufferData.invViewProjMtx.transpose().invert();

		if (ni::FrameData* frame = ni::beginFrame())
		{
			ID3D12GraphicsCommandList* commandList = frame->commandList;
			ni::DescriptorTable rtvDescriptorTable = rtvDescriptorAllocator->allocateDescriptorTable(1);
			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
			resourceBarrier.flush(commandList);
			rtvDescriptorTable.allocRTVTex2D(ni::getCurrentBackbuffer()->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			float clearColor[] = { 0, 0, 0, 1 };
			commandList->ClearRenderTargetView(rtvDescriptorTable.cpuBaseHandle, clearColor, 0, nullptr);
			rtvDescriptorAllocator->reset();
			
			commandList->SetDescriptorHeaps(1, &descriptorAllocator->descriptorHeap);

			if (ni::mouseClick(ni::MOUSE_BUTTON_RIGHT) || sceneConstantBufferData.time == 0.0f)
			{
				commandList->SetPipelineState(initParticles->pso);
				commandList->SetComputeRootSignature(initParticles->rootSignature);
				ni::DescriptorTable initParticleDescriptorTable = descriptorAllocator->allocateDescriptorTable(1);
				initParticleDescriptorTable.allocUAVBuffer(particleBuffer->resource, nullptr, DXGI_FORMAT_UNKNOWN, 0, NUM_PARTICLES, sizeof(ParticleData), 0);
				commandList->SetComputeRootDescriptorTable(0, initParticleDescriptorTable.gpuBaseHandle);
				commandList->SetComputeRoot32BitConstants(1, 1, &sceneConstantBufferData.time, 0);
				commandList->Dispatch(NUM_PARTICLES / 32, 1, 1);
			}

			// Upload scene data
			void* localSceneData = nullptr;
			if (sceneConstantBufferUpload->resource.apiResource->Map(0, nullptr, &localSceneData) == S_OK)
			{
				memcpy(localSceneData, &sceneConstantBufferData, sizeof(sceneConstantBufferData));
				sceneConstantBufferUpload->resource.apiResource->Unmap(0, nullptr);
				resourceBarrier.transition(sceneConstantBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				resourceBarrier.flush(commandList);
				commandList->CopyResource(sceneConstantBuffer->resource.apiResource, sceneConstantBufferUpload->resource.apiResource);
				resourceBarrier.transition(sceneConstantBuffer->resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				resourceBarrier.flush(commandList);
			}

			// Run Simulation
			commandList->SetPipelineState(simulateParticles->pso);
			commandList->SetComputeRootSignature(simulateParticles->rootSignature);
			ni::DescriptorTable simulateParticlesDescriptorTable = descriptorAllocator->allocateDescriptorTable(1);
			simulateParticlesDescriptorTable.allocUAVBuffer(particleBuffer->resource, nullptr, DXGI_FORMAT_UNKNOWN, 0, NUM_PARTICLES, sizeof(ParticleData), 0);
			commandList->SetComputeRootDescriptorTable(0, simulateParticlesDescriptorTable.gpuBaseHandle);
			commandList->Dispatch(NUM_PARTICLES / 32, 1, 1);

			resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.flush(commandList);

			// Render Scene
			commandList->SetPipelineState(basePassParticle->pso);
			commandList->SetComputeRootSignature(basePassParticle->rootSignature);
			ni::DescriptorTable basePassDescriptorTable = descriptorAllocator->allocateDescriptorTable(6);
			basePassDescriptorTable.allocSRVBuffer(particleBuffer->resource, DXGI_FORMAT_UNKNOWN, 0, NUM_PARTICLES, sizeof(ParticleData));
			basePassDescriptorTable.allocUAVTex2D(outputTexture->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(velocityBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(positionBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(normalBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocCBV(sceneConstantBuffer->resource, sceneConstantBuffer->resource.apiResource->GetDesc().Width);
			commandList->SetComputeRootDescriptorTable(0, basePassDescriptorTable.gpuBaseHandle);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f)+1, 1);

			// Temporal Reprojection
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(historyBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(prevHistoryM1Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(prevHistoryM2Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			resourceBarrier.flush(commandList);

			commandList->SetPipelineState(temporalReprojection->pso);
			commandList->SetComputeRootSignature(temporalReprojection->rootSignature);
			ni::DescriptorTable temporalReprojectionDescriptorTable = descriptorAllocator->allocateDescriptorTable(13);
			temporalReprojectionDescriptorTable.allocSRVTex2D(outputTexture->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(historyBuffer->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(positionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(normalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevPositionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevNormalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM1Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM2Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocUAVTex2D(resultTAA->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			temporalReprojectionDescriptorTable.allocUAVTex2D(historyM1Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
			temporalReprojectionDescriptorTable.allocUAVTex2D(historyM2Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
			temporalReprojectionDescriptorTable.allocCBV(sceneConstantBuffer->resource, sceneConstantBuffer->resource.apiResource->GetDesc().Width);
			commandList->SetComputeRootDescriptorTable(0, temporalReprojectionDescriptorTable.gpuBaseHandle);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f) + 1, 1);

			resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.flush(commandList);

			commandList->SetPipelineState(temporalAA->pso);
			commandList->SetComputeRootSignature(temporalAA->rootSignature);
			ni::DescriptorTable temporalAADescriptorTable = descriptorAllocator->allocateDescriptorTable(4);
			temporalAADescriptorTable.allocSRVTex2D(resultTAA->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(historyBuffer->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocUAVTex2D(outputTexture->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			commandList->SetComputeRootDescriptorTable(0, temporalAADescriptorTable.gpuBaseHandle);
			commandList->SetComputeRoot32BitConstants(1, 3, &sceneConstantBufferData.resolution, 0);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f)+1, 1);


			resourceBarrier.transition(historyBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.flush(commandList);
			commandList->CopyResource(historyBuffer->resource.apiResource, outputTexture->resource.apiResource);
#if 0
			// Render to screen
			commandList->SetPipelineState(renderToScreen->pso);
			commandList->SetGraphicsRootSignature(renderToScreen->rootSignature);
			D3D12_VERTEX_BUFFER_VIEW triangleVBV;
			triangleVBV.BufferLocation = triangleVB->resource.apiResource->GetGPUVirtualAddress();
			triangleVBV.SizeInBytes = (UINT)triangleVB->sizeInBytes;
			triangleVBV.StrideInBytes = sizeof(ni::Float3);
			commandList->IASetVertexBuffers(0, 1, &triangleVBV);
			commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->OMSetRenderTargets(1, &rtvDescriptorTable.cpuBaseHandle, true, nullptr);
			D3D12_RECT scissor;
			scissor.left = 0;
			scissor.right = ni::getViewWidthUint();
			scissor.top = 0;
			scissor.bottom = ni::getViewHeightUint();
			commandList->RSSetScissorRects(1, &scissor);
			D3D12_VIEWPORT viewport;
			viewport.Width = ni::getViewWidth();
			viewport.Height = ni::getViewHeight();
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.MinDepth = 0.1f;
			viewport.MaxDepth = 1000.0f;
			commandList->RSSetViewports(1, &viewport);
			ni::DescriptorTable renderToScreenDescriptorTable = descriptorAllocator->allocateDescriptorTable(1);
			renderToScreenDescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			resourceBarrier.flush(commandList);
			commandList->SetGraphicsRootDescriptorTable(0, renderToScreenDescriptorTable.gpuBaseHandle);
			commandList->DrawInstanced(3, 1, 0, 0);

			resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#else
			resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(prevHistoryM1Buffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(prevHistoryM2Buffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.flush(commandList);

			commandList->CopyResource(ni::getCurrentBackbuffer()->resource.apiResource, outputTexture->resource.apiResource);
			commandList->CopyResource(prevPositionBuffer->resource.apiResource, positionBuffer->resource.apiResource);
			commandList->CopyResource(prevNormalBuffer->resource.apiResource, normalBuffer->resource.apiResource);
			commandList->CopyResource(prevHistoryM1Buffer->resource.apiResource, historyM1Buffer->resource.apiResource);
			commandList->CopyResource(prevHistoryM2Buffer->resource.apiResource, historyM2Buffer->resource.apiResource);
#endif
			resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_PRESENT);
			resourceBarrier.flush(commandList);

			ni::endFrame();

			sceneConstantBufferData.time += 0.016f;
			sceneConstantBufferData.frame += 1.0f;
			sceneConstantBufferData.prevCameraPos = sceneConstantBufferData.cameraPos;
			sceneConstantBufferData.prevInvViewProjMtx = sceneConstantBufferData.invViewProjMtx;
			sceneConstantBufferData.prevViewProjMtx = sceneConstantBufferData.viewProjMtx;

		}

		descriptorAllocator->reset();
		ni::present(!false);
	}

	ni::waitForAllFrames();

	ni::destroyPipelineState(AtrousFilter);
	ni::destroyPipelineState(temporalReprojection);
	ni::destroyTexture(historyM1Buffer);
	ni::destroyTexture(historyM2Buffer);
	ni::destroyTexture(prevHistoryM1Buffer);
	ni::destroyTexture(prevHistoryM2Buffer);
	ni::destroyTexture(prevPositionBuffer);
	ni::destroyTexture(prevNormalBuffer);
	ni::destroyTexture(positionBuffer);
	ni::destroyTexture(normalBuffer);
	ni::destroyTexture(resultTAA);
	ni::destroyTexture(historyBuffer);
	ni::destroyPipelineState(temporalAA);
	ni::destroyBuffer(triangleVB);
	ni::destroyPipelineState(renderToScreen);
	ni::destroyDescriptorAllocator(rtvDescriptorAllocator);
	ni::destroyBuffer(sceneConstantBufferUpload);
	ni::destroyBuffer(sceneConstantBuffer);
	ni::destroyTexture(outputTexture);
	ni::destroyTexture(velocityBuffer);
	ni::destroyPipelineState(basePassParticle);
	ni::destroyPipelineState(simulateParticles);
	ni::destroyDescriptorAllocator(descriptorAllocator);
	ni::destroyBuffer(particleBuffer);
	ni::destroyPipelineState(initParticles);
	ni::destroy();

	return 0;
}