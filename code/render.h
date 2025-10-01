#pragma once

#include "ni.h"
#include "../tmp/shaders/RenderToScreenVS.h"

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

	void draw(uint32_t viewWidth, uint32_t viewHeight, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle, ni::DescriptorAllocator* descriptorAllocator, const ni::Array<ResourceDesc>& params)
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
		scissor.right = viewWidth;
		scissor.top = 0;
		scissor.bottom = viewHeight;
		commandList->RSSetScissorRects(1, &scissor);
		D3D12_VIEWPORT viewport;
		viewport.Width = (float)viewWidth;
		viewport.Height = (float)viewHeight;
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

	ConstantBufferUploader(const T& initData)
	{
		buffer = ni::createBuffer(sizeof(T), ni::BufferType::CONSTANT_BUFFER);
		uploadBuffer = ni::createBuffer(sizeof(T), ni::BufferType::UPLOAD_BUFFER);
		data = initData;
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
