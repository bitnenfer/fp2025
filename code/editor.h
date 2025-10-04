#pragma once

#include "imgui/imgui.h"
#include "ni.h"

#include "../tmp/shaders/ImGui_VS.h"
#include "../tmp/shaders/ImGui_PS.h"

struct Editor
{
	static inline ImGuiKey mapKeyCode(ni::KeyCode keyCode)
	{
		switch (keyCode)
		{
		case ni::KeyCode::ALT:
			return ImGuiKey_LeftAlt;
		case ni::KeyCode::DOWN:
			return ImGuiKey_DownArrow;
		case ni::KeyCode::LEFT:
			return ImGuiKey_LeftArrow;
		case ni::KeyCode::RIGHT:
			return ImGuiKey_RightArrow;
		case ni::KeyCode::UP:
			return ImGuiKey_UpArrow;
		case ni::KeyCode::BACKSPACE:
			return ImGuiKey_Backspace;
		case ni::KeyCode::CAPS_LOCK:
			return ImGuiKey_CapsLock;
		case ni::KeyCode::CTRL:
			return ImGuiKey_LeftCtrl;
		case ni::KeyCode::DEL:
			return ImGuiKey_Delete;
		case ni::KeyCode::END:
			return ImGuiKey_End;
		case ni::KeyCode::ENTER:
			return ImGuiKey_Enter;
		case ni::KeyCode::ESC:
			return ImGuiKey_Escape;
		case ni::KeyCode::F1:
			return ImGuiKey_F1;
		case ni::KeyCode::F2:
			return ImGuiKey_F2;
		case ni::KeyCode::F3:
			return ImGuiKey_F3;
		case ni::KeyCode::F4:
			return ImGuiKey_F4;
		case ni::KeyCode::F5:
			return ImGuiKey_F5;
		case ni::KeyCode::F6:
			return ImGuiKey_F6;
		case ni::KeyCode::F7:
			return ImGuiKey_F7;
		case ni::KeyCode::F8:
			return ImGuiKey_F8;
		case ni::KeyCode::F9:
			return ImGuiKey_F9;
		case ni::KeyCode::F10:
			return ImGuiKey_F10;
		case ni::KeyCode::F11:
			return ImGuiKey_F11;
		case ni::KeyCode::F12:
			return ImGuiKey_F12;
		case ni::KeyCode::HOME:
			return ImGuiKey_Home;
		case ni::KeyCode::INSERT:
			return ImGuiKey_Insert;
		case ni::KeyCode::NUM_LOCK:
			return ImGuiKey_NumLock;
		case ni::KeyCode::NUMPAD_0:
			return ImGuiKey_Keypad0;
		case ni::KeyCode::NUMPAD_1:
			return ImGuiKey_Keypad1;
		case ni::KeyCode::NUMPAD_2:
			return ImGuiKey_Keypad2;
		case ni::KeyCode::NUMPAD_3:
			return ImGuiKey_Keypad3;
		case ni::KeyCode::NUMPAD_4:
			return ImGuiKey_Keypad4;
		case ni::KeyCode::NUMPAD_5:
			return ImGuiKey_Keypad5;
		case ni::KeyCode::NUMPAD_6:
			return ImGuiKey_Keypad6;
		case ni::KeyCode::NUMPAD_7:
			return ImGuiKey_Keypad7;
		case ni::KeyCode::NUMPAD_8:
			return ImGuiKey_Keypad8;
		case ni::KeyCode::NUMPAD_9:
			return ImGuiKey_Keypad9;
		case ni::KeyCode::NUMPAD_MINUS:
			return ImGuiKey_KeypadSubtract;
		case ni::KeyCode::NUMPAD_ASTERIKS:
			return ImGuiKey_KeypadMultiply;
		case ni::KeyCode::NUMPAD_DOT:
			return ImGuiKey_KeypadDecimal;
		case ni::KeyCode::NUMPAD_SLASH:
			return ImGuiKey_KeypadDivide;
		case ni::KeyCode::NUMPAD_SUM:
			return ImGuiKey_KeypadAdd;
		case ni::KeyCode::PAGE_DOWN:
			return ImGuiKey_PageDown;
		case ni::KeyCode::PAGE_UP:
			return ImGuiKey_PageUp;
		case ni::KeyCode::PAUSE:
			return ImGuiKey_Pause;
		case ni::KeyCode::PRINT_SCREEN:
			return ImGuiKey_PrintScreen;
		case ni::KeyCode::SCROLL_LOCK:
			return ImGuiKey_ScrollLock;
		case ni::KeyCode::SHIFT:
			return ImGuiKey_LeftShift;
		case ni::KeyCode::SPACE:
			return ImGuiKey_Space;
		case ni::KeyCode::TAB:
			return ImGuiKey_Tab;
		case ni::KeyCode::A:
			return ImGuiKey_A;
		case ni::KeyCode::B:
			return ImGuiKey_B;
		case ni::KeyCode::C:
			return ImGuiKey_C;
		case ni::KeyCode::D:
			return ImGuiKey_D;
		case ni::KeyCode::E:
			return ImGuiKey_E;
		case ni::KeyCode::F:
			return ImGuiKey_F;
		case ni::KeyCode::G:
			return ImGuiKey_G;
		case ni::KeyCode::H:
			return ImGuiKey_H;
		case ni::KeyCode::I:
			return ImGuiKey_I;
		case ni::KeyCode::J:
			return ImGuiKey_J;
		case ni::KeyCode::K:
			return ImGuiKey_K;
		case ni::KeyCode::L:
			return ImGuiKey_L;
		case ni::KeyCode::M:
			return ImGuiKey_M;
		case ni::KeyCode::N:
			return ImGuiKey_N;
		case ni::KeyCode::O:
			return ImGuiKey_O;
		case ni::KeyCode::P:
			return ImGuiKey_P;
		case ni::KeyCode::Q:
			return ImGuiKey_Q;
		case ni::KeyCode::R:
			return ImGuiKey_R;
		case ni::KeyCode::S:
			return ImGuiKey_S;
		case ni::KeyCode::T:
			return ImGuiKey_T;
		case ni::KeyCode::U:
			return ImGuiKey_U;
		case ni::KeyCode::V:
			return ImGuiKey_V;
		case ni::KeyCode::W:
			return ImGuiKey_W;
		case ni::KeyCode::X:
			return ImGuiKey_X;
		case ni::KeyCode::Y:
			return ImGuiKey_Y;
		case ni::KeyCode::Z:
			return ImGuiKey_Z;
		case ni::KeyCode::NUM_0:
			return ImGuiKey_0;
		case ni::KeyCode::NUM_1:
			return ImGuiKey_1;
		case ni::KeyCode::NUM_2:
			return ImGuiKey_2;
		case ni::KeyCode::NUM_3:
			return ImGuiKey_3;
		case ni::KeyCode::NUM_4:
			return ImGuiKey_4;
		case ni::KeyCode::NUM_5:
			return ImGuiKey_5;
		case ni::KeyCode::NUM_6:
			return ImGuiKey_6;
		case ni::KeyCode::NUM_7:
			return ImGuiKey_7;
		case ni::KeyCode::NUM_8:
			return ImGuiKey_8;
		case ni::KeyCode::NUM_9:
			return ImGuiKey_9;
		case ni::KeyCode::QUOTE:
			return ImGuiKey_Apostrophe;
		case ni::KeyCode::MINUS:
			return ImGuiKey_Minus;
		case ni::KeyCode::COMMA:
			return ImGuiKey_Comma;
		case ni::KeyCode::SLASH:
			return ImGuiKey_Slash;
		case ni::KeyCode::SEMICOLON:
			return ImGuiKey_Semicolon;
		case ni::KeyCode::LEFT_SQRBRACKET:
			return ImGuiKey_LeftBracket;
		case ni::KeyCode::RIGHT_SQRBRACKET:
			return ImGuiKey_RightBracket;
		case ni::KeyCode::BACKSLASH:
			return ImGuiKey_Backslash;
		case ni::KeyCode::EQUALS:
			return ImGuiKey_Equal;
		default:
			break;
		}
		return ImGuiKey_None;
	}


	Editor()
	{
		ni::GraphicsPipelineDesc psoDesc = {};
		psoDesc.pixel.shader = { ImGui_PS, sizeof(ImGui_PS) };
		psoDesc.pixel.renderTargets.add(DXGI_FORMAT_R8G8B8A8_UNORM);
		psoDesc.vertex.shader = { ImGui_VS, sizeof(ImGui_VS) };
		psoDesc.vertex.addVertexBuffer(sizeof(ImDrawVert))
			.addVertexAttribute(DXGI_FORMAT_R32G32_FLOAT, "POSITION", 0, offsetof(ImDrawVert, pos))
			.addVertexAttribute(DXGI_FORMAT_R8G8B8A8_UNORM, "COLOR", 0, offsetof(ImDrawVert, col))
			.addVertexAttribute(DXGI_FORMAT_R32G32_FLOAT, "TEXCOORD", 0, offsetof(ImDrawVert, uv));
		psoDesc.rasterizer.cullMode = D3D12_CULL_MODE_NONE;
		psoDesc.layout.add32BitConstant(0, 0, ni::calcNumUint32FromSize<ni::Float4x4>(), D3D12_SHADER_VISIBILITY_VERTEX);
		psoDesc.layout.addDescriptorTable(ni::DescriptorRange(
			ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0)
		), D3D12_SHADER_VISIBILITY_PIXEL);
		psoDesc.layout.addStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		pipelineState = ni::buildGraphicsPipelineState(psoDesc);

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		uint8_t* fontPixels = nullptr;
		int32_t fontWidth = 0, fontHeight = 0;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight);
		fontTexture = ni::createTexture(fontWidth, fontHeight, 1, fontPixels, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_NONE);
		ImGui::GetIO().Fonts->SetTexID((ImTextureID)fontTexture);

		setRenderEnable(false);
	}
	
	~Editor()
	{
		ni::destroyPipelineState(pipelineState);
		ni::destroyBuffer(vertexBuffer);
		ni::destroyBuffer(indexBuffer);
		ni::destroyBuffer(uploadIndexBuffer);
		ni::destroyBuffer(uploadVertexBuffer);
		ni::destroyTexture(fontTexture);
		cleanUpBuffers();
	}

	void cleanUpBuffers()
	{
		for (uint64_t index = 0; index < buffersToDestroy.getNum(); ++index)
		{
			ni::destroyBuffer(buffersToDestroy[index]);
		}
		buffersToDestroy.reset();
	}

	void drawUI()
	{
		cleanUpBuffers();

		if (!shouldRender) return;

		// Update ImGui Input
		ImGui::GetIO().AddMousePosEvent(ni::mouseX(), ni::mouseY());
		ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, ni::mouseDown(ni::MOUSE_BUTTON_LEFT));
		ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, ni::mouseDown(ni::MOUSE_BUTTON_RIGHT));
		ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Middle, ni::mouseDown(ni::MOUSE_BUTTON_MIDDLE));
		ImGui::GetIO().DisplaySize = { ni::getViewWidth(), ni::getViewHeight() };
		ImGui::GetIO().AddInputCharacterUTF16(ni::getLastChar());
		ImGui::GetIO().AddMouseWheelEvent(0, ni::mouseWheelY());
		ni::forEachKeyDown([](ni::KeyCode keyCode, bool isKeyDown) {ImGui::GetIO().AddKeyEvent(Editor::mapKeyCode(keyCode), isKeyDown); });

		// Draw ImGui
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();
	}

	void render(ID3D12GraphicsCommandList* commandList, ni::DescriptorAllocator* descriptorAllocator, ni::ResourceBarrierBatcher& resourceBarriers)
	{
		if (!shouldRender) return;
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData == nullptr) return;
		// ImGui: Create resources on demand
		if (drawData->TotalVtxCount > 0 && (vertexBuffer == nullptr || vertexBuffer->sizeInBytes < drawData->TotalVtxCount * sizeof(ImDrawVert)))
		{
			if (vertexBuffer != nullptr)
			{
				buffersToDestroy.add(vertexBuffer);
				buffersToDestroy.add(uploadVertexBuffer);
			}

			vertexBuffer = ni::createBuffer(drawData->TotalVtxCount * sizeof(ImDrawVert), ni::VERTEX_BUFFER);
			uploadVertexBuffer = ni::createBuffer(drawData->TotalVtxCount * sizeof(ImDrawVert), ni::UPLOAD_BUFFER);
		}

		if (drawData->TotalIdxCount > 0 && (indexBuffer == nullptr || indexBuffer->sizeInBytes < drawData->TotalIdxCount * sizeof(ImDrawIdx)))
		{
			if (indexBuffer != nullptr)
			{
				buffersToDestroy.add(indexBuffer);
				buffersToDestroy.add(uploadIndexBuffer);
			}
			indexBuffer = ni::createBuffer(drawData->TotalIdxCount * sizeof(ImDrawIdx), ni::INDEX_BUFFER);
			uploadIndexBuffer = ni::createBuffer(drawData->TotalIdxCount * sizeof(ImDrawIdx), ni::UPLOAD_BUFFER);
		}

		if (drawData->CmdListsCount > 0)
		{
			// ImGui: Copy render data to upload buffers
			ImDrawVert* vertices = nullptr;
			ImDrawIdx* indices = nullptr;

			uploadVertexBuffer->resource.apiResource->Map(0, nullptr, (void**)&vertices);
			uploadIndexBuffer->resource.apiResource->Map(0, nullptr, (void**)&indices);

			for (int32_t Index = 0; Index < drawData->CmdListsCount; ++Index)
			{
				const ImDrawList* cmdList = drawData->CmdLists[Index];
				memcpy(vertices, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(indices, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
				vertices += cmdList->VtxBuffer.Size;
				indices += cmdList->IdxBuffer.Size;
			}

			uploadIndexBuffer->resource.apiResource->Unmap(0, nullptr);
			uploadVertexBuffer->resource.apiResource->Unmap(0, nullptr);

			resourceBarriers.transition(vertexBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarriers.transition(indexBuffer->resource, D3D12_RESOURCE_STATE_COPY_DEST);
			resourceBarriers.flush(commandList);
			commandList->CopyResource(vertexBuffer->resource.apiResource, uploadVertexBuffer->resource.apiResource);
			commandList->CopyResource(indexBuffer->resource.apiResource, uploadIndexBuffer->resource.apiResource);
			resourceBarriers.transition(vertexBuffer->resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			resourceBarriers.transition(indexBuffer->resource, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			resourceBarriers.flush(commandList);

			ni::Float4x4 viewMatrix;
			viewMatrix.orthographic(0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, -1000.0f, 1000.0f);
			commandList->SetPipelineState(pipelineState->pso);
			commandList->SetGraphicsRootSignature(pipelineState->rootSignature);
			commandList->SetGraphicsRoot32BitConstants(0, ni::calcNumUint32FromSize<ni::Float4x4>(), &viewMatrix, 0);
			D3D12_VIEWPORT viewport;
			viewport.Width = (float)drawData->DisplaySize.x;
			viewport.Height = (float)drawData->DisplaySize.y;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.MinDepth = 0.1f;
			viewport.MaxDepth = 1000.0f;
			commandList->RSSetViewports(1, &viewport);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = vertexBuffer->resource.apiResource->GetGPUVirtualAddress();
			vbv.SizeInBytes = (UINT)vertexBuffer->sizeInBytes;
			vbv.StrideInBytes = sizeof(ImDrawVert);
			commandList->IASetVertexBuffers(0, 1, &vbv);

			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = indexBuffer->resource.apiResource->GetGPUVirtualAddress();
			ibv.SizeInBytes = (UINT)indexBuffer->sizeInBytes;
			ibv.Format = DXGI_FORMAT_R16_UINT;
			commandList->IASetIndexBuffer(&ibv);

			ImVec2 clipOff = drawData->DisplayPos;
			uint32_t globalVtxOffset = 0;
			uint32_t globalIdxOffset = 0;
			for (int32_t Index = 0; Index < drawData->CmdListsCount; ++Index)
			{
				const ImDrawList* cmdList = drawData->CmdLists[Index];
				for (int32_t cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; ++cmdIndex)
				{
					const ImDrawCmd& cmd = cmdList->CmdBuffer[cmdIndex];
					ImVec2 clipMin = { cmd.ClipRect.x - clipOff.x, cmd.ClipRect.y - clipOff.y };
					ImVec2 clipMax = { cmd.ClipRect.z - clipOff.x, cmd.ClipRect.w - clipOff.y };
					if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					{
						continue;
					}
					D3D12_RECT scissor{};
					scissor.left = (uint32_t)clipMin.x;
					scissor.right = (uint32_t)clipMax.x;
					scissor.top = (uint32_t)clipMin.y;
					scissor.bottom = (uint32_t)clipMax.y;
					commandList->RSSetScissorRects(1, &scissor);
					ni::Texture* texture = (ni::Texture*)cmd.TextureId;
					ni::DescriptorTable descTable = descriptorAllocator->allocateDescriptorTable(1);
					descTable.allocSRVTex2D(texture->resource, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1, 0, 0.0f);
					commandList->SetGraphicsRootDescriptorTable(1, descTable.gpuBaseHandle);
					commandList->DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset + globalIdxOffset, cmd.VtxOffset + globalVtxOffset, 0);
				}
				globalIdxOffset += cmdList->IdxBuffer.Size;
				globalVtxOffset += cmdList->VtxBuffer.Size;
			}
		}
	}

	void setRenderEnable(bool value)
	{
		shouldRender = value;
		ShowCursor(shouldRender);
	}

	void toggleRenderEnable()
	{
		shouldRender = !shouldRender;
		ShowCursor(shouldRender);
	}

	bool isDrawing() const { return shouldRender; }

private:
	ni::PipelineState* pipelineState = nullptr;
	ni::Buffer* vertexBuffer = nullptr;
	ni::Buffer* indexBuffer = nullptr;
	ni::Buffer* uploadIndexBuffer = nullptr;
	ni::Buffer* uploadVertexBuffer = nullptr;
	ni::Texture* fontTexture = nullptr;
	ni::Array<ni::Buffer*> buffersToDestroy;
	bool shouldRender = false;
};