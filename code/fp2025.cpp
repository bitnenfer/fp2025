#include "ni.h"

// Shaders
#include "../tmp/shaders/InitParticlesCS.h"
#include "../tmp/shaders/SimulateCS.h"
#include "../tmp/shaders/ParticleBasePassCS.h"
#include "../tmp/shaders/TemporalAACS.h"
#include "../tmp/shaders/RenderToScreenVS.h"
#include "../tmp/shaders/TemporalReprojectionCS.h"
#include "../tmp/shaders/ATrousFilterCS.h"
#include "../tmp/shaders/DepthOfFieldPS.h"

#define IS_CPU 1
#include "../shaders/ParticleConfig.h"

inline UINT PIX_COLOR_INDEX(BYTE i) { return 0x00000000 | i; }

enum class ResourceType
{
	UAV_BUFFER,
	UAV_TEX1D,
	UAV_TEX1D_ARRAY,
	UAV_TEX2D,
	UAV_TEX2D_ARRAY,
	UAV_TEX3D,
	SRV_BUFFER,
	SRV_TEX1D,
	SRV_TEX1D_ARRAY,
	SRV_TEX2D,
	SRV_TEX2D_ARRAY,
	SRV_TEX3D,
	SRV_TEXCUBE,
	RTV_TEX2D,
	DSV_TEX2D,
	CBV_BUFFER
};

enum class ResourceAccessType
{
	INVALID,
	UAV,
	SRV,
	RTV,
	DSV,
	CBV
};

union ResourceVariant
{
	struct
	{
		ni::Resource* counter;
		DXGI_FORMAT format;
		uint64_t firstElement;
		uint32_t numElements; 
		uint32_t structureByteStride;
		uint64_t counterOffsetInBytes;
		bool rawBuffer;
	} uavBuffer;

	struct
	{
		ni::Resource* counter; 
		DXGI_FORMAT format; 
		uint32_t mipSlice;
	} uavTex1D;

	struct
	{
		ni::Resource* counter; 
		DXGI_FORMAT format;
		uint32_t mipSlice;
		uint32_t firstArraySlice;
		uint32_t arraySize;
	} uavTex1DArray;

	struct
	{
		ni::Resource* counter;
		DXGI_FORMAT format;
		uint32_t mipSlice;
		uint32_t planeSlice;
	} uavTex2D;

	struct
	{
		ni::Resource* counter;
		DXGI_FORMAT format;
		uint32_t mipSlice;
		uint32_t firstArraySlice;
		uint32_t arraySize;
		uint32_t planeSlice;
	} uavTex2DArray;

	struct
	{
		ni::Resource* counter;
		DXGI_FORMAT format;
		uint32_t mipSlice;
		uint32_t firstWSlice;
		uint32_t wSize;
	} uavTex3D;

	struct
	{
		DXGI_FORMAT format;
		uint64_t firstElement;
		uint32_t numElements;
		uint32_t structureByteStride;
		bool rawBuffer;
	} srvBuffer;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		float resourceMinLODClamp;
	} srvTex1D;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		uint32_t firstArraySlice;
		uint32_t arraySize;
		float resourceMinLODClamp;
	} srvTex1DArray;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		uint32_t planeSlice;
		float resourceMinLODClamp;
	} srvTex2D;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		uint32_t firstArraySlice;
		uint32_t arraySize;
		uint32_t planeSlice;
		float resourceMinLODClamp;
	} srvTex2DArray;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		float resourceMinLODClamp;
	} srvTex3D;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mostDetailedMip;
		uint32_t mipLevels;
		float resourceMinLODClamp;
	} srvTexCube;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mipSlice; 
		uint32_t planeSlice;
	} rtvTex2D;

	struct
	{
		DXGI_FORMAT format;
		uint32_t mipSlice;
		D3D12_DSV_FLAGS flags;
	} dsvTex2D;

	struct
	{
		size_t sizeInBytes;
	} cbvBuffer;
};

struct ResourceDesc
{
	static ResourceDesc uavBuffer(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, uint64_t counterOffsetInBytes, bool rawBuffer = false)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_BUFFER;
		desc.variant.uavBuffer = { counter, format, firstElement, numElements, structureByteStride, counterOffsetInBytes, rawBuffer };
		return desc;
	}
	static ResourceDesc uavTex1D(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint32_t mipSlice)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_TEX1D;
		desc.variant.uavTex1D = { counter, format, mipSlice };
		return desc;
	}
	static ResourceDesc uavTex1DArray(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_TEX1D_ARRAY;
		desc.variant.uavTex1DArray = { counter, format, mipSlice, firstArraySlice, arraySize };
		return desc;
	}
	static ResourceDesc uavTex2D(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_TEX2D;
		desc.variant.uavTex2D = { counter, format, mipSlice, planeSlice };
		return desc;
	}
	static ResourceDesc uavTex2DArray(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_TEX2D_ARRAY;
		desc.variant.uavTex2DArray = { counter, format, mipSlice, firstArraySlice, arraySize, planeSlice };
		return desc;
	}
	static ResourceDesc uavTex3D(ni::Resource& resource, ni::Resource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstWSlice, uint32_t wSize)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::UAV_TEX3D;
		desc.variant.uavTex3D = { counter, format, mipSlice, firstWSlice, wSize };
		return desc;
	}
	static ResourceDesc srvBuffer(ni::Resource& resource, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, bool rawBuffer = false)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_BUFFER;
		desc.variant.srvBuffer = { format, firstElement, numElements, structureByteStride, rawBuffer };
		return desc;
	}
	static ResourceDesc srvTex1D(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEX1D;
		desc.variant.srvTex1D = { format, mostDetailedMip, mipLevels, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc srvTex1DArray(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEX1D_ARRAY;
		desc.variant.srvTex1DArray = { format, mostDetailedMip, mipLevels, firstArraySlice, arraySize, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc srvTex2D(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t planeSlice, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEX2D;
		desc.variant.srvTex2D = { format, mostDetailedMip, mipLevels, planeSlice, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc srvTex2DArray(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEX2D_ARRAY;
		desc.variant.srvTex2DArray = { format, mostDetailedMip, mipLevels, firstArraySlice, arraySize, planeSlice, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc srvTex3D(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEX3D;
		desc.variant.srvTex3D = { format, mostDetailedMip, mipLevels, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc srvTexCube(ni::Resource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::SRV_TEXCUBE;
		desc.variant.srvTexCube = { format, mostDetailedMip, mipLevels, resourceMinLODClamp };
		return desc;
	}
	static ResourceDesc rtvTex2D(ni::Resource& resource, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::RTV_TEX2D;
		desc.variant.rtvTex2D = { format, mipSlice, planeSlice };
		return desc;
	}
	static ResourceDesc dsvTex2D(ni::Resource& resource, DXGI_FORMAT format, uint32_t mipSlice, D3D12_DSV_FLAGS flags)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::DSV_TEX2D;
		desc.variant.dsvTex2D = { format, mipSlice, flags };
		return desc;
	}
	static ResourceDesc cbvBuffer(ni::Resource& resource, size_t sizeInBytes)
	{
		ResourceDesc desc = {};
		desc.resource = &resource;
		desc.type = ResourceType::CBV_BUFFER;
		desc.variant.cbvBuffer.sizeInBytes = sizeInBytes;
		return desc;
	}

	ResourceAccessType getAccessType() const
	{
		switch (type)
		{
		case ResourceType::UAV_BUFFER:
		case ResourceType::UAV_TEX1D:
		case ResourceType::UAV_TEX1D_ARRAY:
		case ResourceType::UAV_TEX2D:
		case ResourceType::UAV_TEX2D_ARRAY:
		case ResourceType::UAV_TEX3D:
			return ResourceAccessType::UAV;
		case ResourceType::SRV_BUFFER:
		case ResourceType::SRV_TEX1D:
		case ResourceType::SRV_TEX1D_ARRAY:
		case ResourceType::SRV_TEX2D:
		case ResourceType::SRV_TEX2D_ARRAY:
		case ResourceType::SRV_TEX3D:
		case ResourceType::SRV_TEXCUBE:
			return ResourceAccessType::SRV;
		case ResourceType::RTV_TEX2D:
			return ResourceAccessType::RTV;
		case ResourceType::DSV_TEX2D:
			return ResourceAccessType::DSV;
		case ResourceType::CBV_BUFFER:
			return ResourceAccessType::CBV;
		}

		return ResourceAccessType::INVALID;
	}

	ResourceType type;
	ni::Resource* resource;
	ResourceVariant variant;
};

struct FullscreenRasterPass
{
	FullscreenRasterPass() = default;
	~FullscreenRasterPass()
	{
		ni::destroyBuffer(triangleVB);
		ni::destroyPipelineState(renderToScreen);
	}
	void build()
	{
		ni::Float3 triangle[] = { {-1.0f, -7.0f, 0.0f }, {-1.0f, 7.0f, 0.0f}, {7.0f, 7.0f, 0.0f} };
		triangleVB = ni::createBuffer(sizeof(triangle), ni::VERTEX_BUFFER, triangle);
		ni::GraphicsPipelineDesc renderToScreenDesc = {};
		renderToScreenDesc.rasterizer.cullMode = D3D12_CULL_MODE_NONE;
		renderToScreenDesc.vertex.shader = { RenderToScreenVS, sizeof(RenderToScreenVS) };
		renderToScreenDesc.vertex.addVertexBuffer(sizeof(ni::Float3)).addVertexAttribute(DXGI_FORMAT_R32G32B32_FLOAT, "POSITION", 0, 0);
		renderToScreenDesc.pixel = pixel;
		renderToScreenDesc.layout = layout;
		renderToScreen = ni::buildGraphicsPipelineState(renderToScreenDesc);
	}

	void draw(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle, ni::DescriptorAllocator* descriptorAllocator, const ni::Array<ResourceDesc>& params)
	{
		NI_ASSERT(renderToScreen != nullptr, "Pass hasn't been built");

		ni::DescriptorTable descriptorTable = descriptorAllocator->allocateDescriptorTable((uint32_t)params.getNum());

		commandList->SetPipelineState(renderToScreen->pso);
		commandList->SetGraphicsRootSignature(renderToScreen->rootSignature);

		D3D12_VERTEX_BUFFER_VIEW triangleVBV;
		triangleVBV.BufferLocation = triangleVB->resource.apiResource->GetGPUVirtualAddress();
		triangleVBV.SizeInBytes = (UINT)triangleVB->sizeInBytes;
		triangleVBV.StrideInBytes = sizeof(ni::Float3);
		commandList->IASetVertexBuffers(0, 1, &triangleVBV);
		commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, true, nullptr);

		float clearColor[] = { 0,0,0,1 };
		commandList->ClearRenderTargetView(rtvDescriptorHandle, clearColor, 0, nullptr);

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

		for (uint64_t index = 0; index < params.getNum(); ++index)
		{
			ResourceDesc resDesc = params[index];
			ResourceAccessType accessType = resDesc.getAccessType();
			switch (accessType)
			{
			case ResourceAccessType::UAV:
				resourceBarriers.transition(*resDesc.resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				break;
			case ResourceAccessType::SRV:
				resourceBarriers.transition(*resDesc.resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				break;
			case ResourceAccessType::RTV:
				resourceBarriers.transition(*resDesc.resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
				break;
			case ResourceAccessType::DSV:
				resourceBarriers.transition(*resDesc.resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				break;
			case ResourceAccessType::CBV:
				resourceBarriers.transition(*resDesc.resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				break;
			default:
				NI_ASSERT(false, "invalid access type");
				break;
			}

			ResourceType type = resDesc.type;
			switch (type)
			{
			case ResourceType::UAV_BUFFER:
				NI_PANIC("not implemented");
				break;
			case ResourceType::UAV_TEX1D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::UAV_TEX1D_ARRAY:
				NI_PANIC("not implemented");
				break;
			case ResourceType::UAV_TEX2D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::UAV_TEX2D_ARRAY:
				NI_PANIC("not implemented");
				break;
			case ResourceType::UAV_TEX3D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_BUFFER:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_TEX1D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_TEX1D_ARRAY:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_TEX2D:
				descriptorTable.allocSRVTex2D(*resDesc.resource, resDesc.variant.srvTex2D.format, resDesc.variant.srvTex2D.mostDetailedMip, resDesc.variant.srvTex2D.mipLevels, resDesc.variant.srvTex2D.planeSlice, resDesc.variant.srvTex2D.resourceMinLODClamp);
				break;
			case ResourceType::SRV_TEX2D_ARRAY:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_TEX3D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::SRV_TEXCUBE:
				NI_PANIC("not implemented");
				break;
			case ResourceType::RTV_TEX2D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::DSV_TEX2D:
				NI_PANIC("not implemented");
				break;
			case ResourceType::CBV_BUFFER:
				descriptorTable.allocCBVBuffer(*resDesc.resource, resDesc.variant.cbvBuffer.sizeInBytes);
				break;
			default:
				NI_PANIC("invalid resource type");
				break;
			}
		}

		resourceBarriers.flush(commandList);
		// TODO: this only handles 1 descriptor table at index 0
		commandList->SetGraphicsRootDescriptorTable(0, descriptorTable.gpuBaseHandle);
		commandList->DrawInstanced(3, 1, 0, 0);
	}



	ni::PixelStage pixel;
	ni::BindingLayout layout;
	ni::Buffer* triangleVB = nullptr;
	ni::PipelineState* renderToScreen = nullptr;
	ni::ResourceBarrierBatcher resourceBarriers;
};

template<typename T>
struct ConstantBufferUploader
{
	ConstantBufferUploader()
	{
		buffer = ni::createBuffer(sizeof(T), ni::BufferType::CONSTANT_BUFFER);
		uploadBuffer = ni::createBuffer(sizeof(T), ni::BufferType::UPLOAD_BUFFER);
	}
	~ConstantBufferUploader()
	{
		ni::destroyBuffer(buffer);
		ni::destroyBuffer(uploadBuffer);
	}
	void update(ID3D12GraphicsCommandList* commandList)
	{
		void* ptr = nullptr;
		if (uploadBuffer->resource.apiResource->Map(0, nullptr, &ptr) == S_OK)
		{
			memcpy(ptr, &data, sizeof(T));
			uploadBuffer->resource.apiResource->Unmap(0, nullptr);
			ni::FixedResourceBarrierBatcher<1> resourceBarrier;
			resourceBarrier.transition(buffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.flush(commandList);
			commandList->CopyResource(buffer->resource.apiResource, uploadBuffer->resource.apiResource);
			resourceBarrier.transition(buffer->resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			resourceBarrier.flush(commandList);
		}
	}
	T data{};
	ni::Buffer* buffer = nullptr;
	ni::Buffer* uploadBuffer = nullptr;
};

int main()
{
	ni::init(1920, 1080, "FP2025", !true, NI_DEBUG);

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
#endif

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
	simulateParticlesDesc.layout.add32BitConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
	simulateParticlesDesc.shader = { SimulateCS, sizeof(SimulateCS) };
	ni::PipelineState* simulateParticles = ni::buildComputePipelineState(simulateParticlesDesc);

	// Particle Base Pass
	ni::ComputePipelineDesc basePassParticleDesc = {};
	basePassParticleDesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 5, 0, 0),
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
	projMtx.perspective(ni::toRad(40.0f), ni::getViewAspectRatio(), 0.1f, 1000.0f);
	ni::FlyCamera camera(ni::Float3(0, -0, -80));
	camera.speedScale = 0.15f;
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
	ni::Texture* depthBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ni::Texture* prevDepthBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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
	AtrousFilterDesc.layout.add32BitConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.layout.add32BitConstant(1, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
	AtrousFilterDesc.shader = { ATrousFilterCS, sizeof(ATrousFilterCS) };
	ni::PipelineState* atrousFilter = ni::buildComputePipelineState(AtrousFilterDesc);

	// Temporal AA Setup
	ni::ComputePipelineDesc temporalAADesc = {};
	temporalAADesc.shader = { TemporalAACS, sizeof(TemporalAACS) };
	temporalAADesc.layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_ALL);
	temporalAADesc.layout.add32BitConstant(0, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
	ni::PipelineState* temporalAA = ni::buildComputePipelineState(temporalAADesc);
	ni::Texture* historyBuffer = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	ni::Texture* resultTAA = ni::createTexture(ni::getViewWidthUint(), ni::getViewHeightUint(), 1, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	
	// Postprocesses

	// Depth of Field
	FullscreenRasterPass* depthOfFieldPass = new FullscreenRasterPass();
	depthOfFieldPass->pixel.shader = { DepthOfFieldPS, sizeof(DepthOfFieldPS) };
	depthOfFieldPass->pixel.renderTargets.add(DXGI_FORMAT_R8G8B8A8_UNORM);
	depthOfFieldPass->layout.addDescriptorTable(ni::DescriptorRange(
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0),
		ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
	), D3D12_SHADER_VISIBILITY_PIXEL);
	depthOfFieldPass->layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	depthOfFieldPass->build();

	ConstantBufferUploader<DepthOfFieldData>* depthOfFieldCB = new ConstantBufferUploader<DepthOfFieldData>();
	depthOfFieldCB->data.focusScale = 68.0f;
	depthOfFieldCB->data.focusPoint = 42.0f;
	depthOfFieldCB->data.radiusScale = 1.5f;
	depthOfFieldCB->data.blurSize = 10.0f;
	depthOfFieldCB->data.maxDist = 1000.0f;
	depthOfFieldCB->data.resolution = float2(sceneConstantBufferData.resolution.x, sceneConstantBufferData.resolution.y);

	while (!ni::shouldQuit())
	{
		ni::pollEvents();

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
		sceneConstantBufferData.cameraPos = camera.position;
		sceneConstantBufferData.viewMtx = camera.makeViewMatrix();
		sceneConstantBufferData.viewProjMtx = projMtx* sceneConstantBufferData.viewMtx;
		sceneConstantBufferData.invViewProjMtx = sceneConstantBufferData.viewProjMtx;
		sceneConstantBufferData.invViewProjMtx.transpose().invert();



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

		if (ni::FrameData* frame = ni::beginFrame())
		{
			ID3D12GraphicsCommandList* commandList = frame->commandList;

			depthOfFieldCB->update(commandList);

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
			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(0), "Particle Simulation");
			commandList->SetPipelineState(simulateParticles->pso);
			commandList->SetComputeRootSignature(simulateParticles->rootSignature);
			ni::DescriptorTable simulateParticlesDescriptorTable = descriptorAllocator->allocateDescriptorTable(1);
			simulateParticlesDescriptorTable.allocUAVBuffer(particleBuffer->resource, nullptr, DXGI_FORMAT_UNKNOWN, 0, NUM_PARTICLES, sizeof(ParticleData), 0);
			commandList->SetComputeRootDescriptorTable(0, simulateParticlesDescriptorTable.gpuBaseHandle);
			commandList->SetComputeRoot32BitConstants(1, 1, &sceneConstantBufferData.time, 0);
			commandList->Dispatch(NUM_PARTICLES / 32, 1, 1);
			pixEndEventOnCommandList(commandList);

			resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(velocityBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.flush(commandList);

			// Render Scene
			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(1), "Render Base Scene");
			commandList->SetPipelineState(basePassParticle->pso);
			commandList->SetComputeRootSignature(basePassParticle->rootSignature);
			ni::DescriptorTable basePassDescriptorTable = descriptorAllocator->allocateDescriptorTable(7);
			basePassDescriptorTable.allocSRVBuffer(particleBuffer->resource, DXGI_FORMAT_UNKNOWN, 0, NUM_PARTICLES, sizeof(ParticleData));
			basePassDescriptorTable.allocUAVTex2D(outputTexture->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(velocityBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(positionBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(normalBuffer->resource, nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0);
			basePassDescriptorTable.allocUAVTex2D(depthBuffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
			basePassDescriptorTable.allocCBVBuffer(sceneConstantBuffer->resource, sceneConstantBuffer->resource.apiResource->GetDesc().Width);
			commandList->SetComputeRootDescriptorTable(0, basePassDescriptorTable.gpuBaseHandle);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f)+1, 1);
			pixEndEventOnCommandList(commandList);

			// Temporal Reprojection
			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(2), "Temporal Reprojection");
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
			resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.flush(commandList);

			commandList->SetPipelineState(temporalReprojection->pso);
			commandList->SetComputeRootSignature(temporalReprojection->rootSignature);
			ni::DescriptorTable temporalReprojectionDescriptorTable = descriptorAllocator->allocateDescriptorTable(15);
			temporalReprojectionDescriptorTable.allocSRVTex2D(outputTexture->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(historyBuffer->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(positionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(normalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevPositionBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevNormalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM1Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevHistoryM2Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocSRVTex2D(prevDepthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalReprojectionDescriptorTable.allocUAVTex2D(resultTAA->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			temporalReprojectionDescriptorTable.allocUAVTex2D(historyM1Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
			temporalReprojectionDescriptorTable.allocUAVTex2D(historyM2Buffer->resource, nullptr, DXGI_FORMAT_R32_FLOAT, 0, 0);
			temporalReprojectionDescriptorTable.allocCBVBuffer(sceneConstantBuffer->resource, sceneConstantBuffer->resource.apiResource->GetDesc().Width);
			commandList->SetComputeRootDescriptorTable(0, temporalReprojectionDescriptorTable.gpuBaseHandle);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f) + 1, 1);
			pixEndEventOnCommandList(commandList);

			resourceBarrier.transition(historyM1Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(historyM2Buffer->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.flush(commandList);

			// A-trous filter passes
			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(3), "A-Trous Filter Passes");
			ni::Texture* atrousInputOutput[2] = { resultTAA, outputTexture };

			commandList->SetPipelineState(atrousFilter->pso);
			commandList->SetComputeRootSignature(atrousFilter->rootSignature);
			
			for (uint32_t index = 0; index < 5; ++index)
			{
				ni::Texture* input = atrousInputOutput[index % 2];
				ni::Texture* output = atrousInputOutput[(index + 1) % 2];

				resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				resourceBarrier.flush(commandList);

				ni::DescriptorTable atrousFilterDescriptorTable = descriptorAllocator->allocateDescriptorTable(7);
				atrousFilterDescriptorTable.allocSRVTex2D(resultTAA->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
				atrousFilterDescriptorTable.allocSRVTex2D(normalBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
				atrousFilterDescriptorTable.allocSRVTex2D(historyM1Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				atrousFilterDescriptorTable.allocSRVTex2D(historyM2Buffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				atrousFilterDescriptorTable.allocSRVTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
				atrousFilterDescriptorTable.allocUAVTex2D(outputTexture->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
				commandList->SetComputeRootDescriptorTable(0, atrousFilterDescriptorTable.gpuBaseHandle);
				commandList->SetComputeRoot32BitConstant(1, index+1, 0);
				commandList->SetComputeRoot32BitConstants(2, 3, &sceneConstantBufferData.resolution, 0);
				commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f) + 1, 1);
			}

			pixEndEventOnCommandList(commandList);

			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.flush(commandList);

			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(4), "TemporalAA");
			commandList->SetPipelineState(temporalAA->pso);
			commandList->SetComputeRootSignature(temporalAA->rootSignature);
			ni::DescriptorTable temporalAADescriptorTable = descriptorAllocator->allocateDescriptorTable(6);
			temporalAADescriptorTable.allocSRVTex2D(outputTexture->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(velocityBuffer->resource, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(historyBuffer->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocSRVTex2D(prevDepthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f);
			temporalAADescriptorTable.allocUAVTex2D(resultTAA->resource, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0);
			commandList->SetComputeRootDescriptorTable(0, temporalAADescriptorTable.gpuBaseHandle);
			commandList->SetComputeRoot32BitConstants(1, 3, &sceneConstantBufferData.resolution, 0);
			commandList->Dispatch((uint32_t)(sceneConstantBufferData.resolution.x / 32.0f), (uint32_t)(sceneConstantBufferData.resolution.y / 32.0f)+1, 1);
			pixEndEventOnCommandList(commandList);

			pixBeginEventOnCommandList(commandList, PIX_COLOR_INDEX(5), "Copy Next Frame Resources");
			resourceBarrier.transition(historyBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarrier.transition(resultTAA->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarrier.flush(commandList);
			commandList->CopyResource(historyBuffer->resource.apiResource, resultTAA->resource.apiResource);

			resourceBarrier.transition(particleBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(outputTexture->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			//resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_COPY_DEST);
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

			//commandList->CopyResource(ni::getCurrentBackbuffer()->resource.apiResource, outputTexture->resource.apiResource);
			commandList->CopyResource(prevPositionBuffer->resource.apiResource, positionBuffer->resource.apiResource);
			commandList->CopyResource(prevNormalBuffer->resource.apiResource, normalBuffer->resource.apiResource);
			commandList->CopyResource(prevHistoryM1Buffer->resource.apiResource, historyM1Buffer->resource.apiResource);
			commandList->CopyResource(prevHistoryM2Buffer->resource.apiResource, historyM2Buffer->resource.apiResource);
			commandList->CopyResource(prevDepthBuffer->resource.apiResource, depthBuffer->resource.apiResource);

			ni::Array<ResourceDesc> depthOfFieldPassParams;
			depthOfFieldPassParams.add(ResourceDesc::srvTex2D(depthBuffer->resource, DXGI_FORMAT_R32_FLOAT, 0, 1, 0, 0.0f));
			depthOfFieldPassParams.add(ResourceDesc::srvTex2D(outputTexture->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f));
			depthOfFieldPassParams.add(ResourceDesc::cbvBuffer(depthOfFieldCB->buffer->resource, depthOfFieldCB->buffer->resource.apiResource->GetDesc().Width));
			depthOfFieldPass->draw(commandList, rtvDescriptorTable.cpuBaseHandle, descriptorAllocator, depthOfFieldPassParams);

			resourceBarrier.transition(positionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(normalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(prevPositionBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(prevNormalBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(depthBuffer->resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resourceBarrier.transition(ni::getCurrentBackbuffer()->resource, D3D12_RESOURCE_STATE_PRESENT);
			resourceBarrier.flush(commandList);
			pixEndEventOnCommandList(commandList);

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
	delete depthOfFieldPass;
	delete depthOfFieldCB;

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
	ni::destroyTexture(resultTAA);
	ni::destroyTexture(historyBuffer);
	ni::destroyPipelineState(temporalAA);
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