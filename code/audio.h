#pragma once

#include "../shaders/ParticleConfig.h"

#include <mmsystem.h>
#include <mmreg.h>
#pragma comment(lib, "winmm.lib")

struct AudioRenderer
{
	enum class RenderState
	{
		NOT_STARTED,
		DISPATCHED_TO_GPU,
		COPYING_TO_CPU,
		FINISHED
	};

	static const uint32_t WAVE_BLOCK_NUM = 3;

	static void CALLBACK waveOutCallback(HWAVEOUT hwo, UINT msg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		if (msg == WOM_DONE)
		{
			AudioRenderer* renderer = (AudioRenderer*)dwInstance;
			WAVEHDR* hdr = (WAVEHDR*)dwParam1;
			hdr->dwUser = 0;
			InterlockedExchange8((char*)&renderer->isWaitingForWaveOut, 0);
		}
	}

	static DWORD runAudioThread(LPVOID threadParam)
	{
		AudioRenderer* renderer = (AudioRenderer*)threadParam;
		while (renderer->shouldRunAudioThread)
		{
			renderer->processAudioThreadFrame();
			Sleep(1);
		}
		return 0;
	}

	static inline short floatToPCM16(float x) {
		// clamp and convert [-1,1] to int16
		if (x > 1.0f) x = 1.0f;
		if (x < -1.0f) x = -1.0f;
		int s = int(lround(x * 32767.0f));
		if (s > 32767) s = 32767;
		if (s < -32768) s = -32768;
		return short(s);
	}

	AudioRenderer(uint32_t secondOfAudio) :
		durationInSeconds(secondOfAudio)
	{
		ni::ComputePipelineDesc audioProcessDesc = {};
		audioProcessDesc.shader = { AudioProcessCS, sizeof(AudioProcessCS) };
		audioProcessDesc.layout.addDescriptorTable(ni::DescriptorRange(
			ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0),
			ni::DescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0)
		), D3D12_SHADER_VISIBILITY_ALL);
		audioProcess = ni::buildComputePipelineState(audioProcessDesc);

		blockAlign = (bitsPerSample / 8) * numChannels;
		numFrames = (sampleRate * durationInSeconds);
		cpuBufferSize = numFrames * blockAlign;
		size_t gpuBufferSize = numFrames * sizeof(ni::Float2);
		renderedBuffer = ni::createBuffer(gpuBufferSize, ni::UNORDERED_BUFFER, 0, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		readbackBuffer = ni::createBuffer(gpuBufferSize, ni::READBACK_BUFFER, 0);

		ni::getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence));
		copyFenceValue = 0;
		copyFenceEvent = CreateEvent(nullptr, false, false, nullptr);

		audioData = new ConstantBufferUploader<AudioData>();
		audioData->data.sampleRate = sampleRate;
		audioData->data.duration = (float)durationInSeconds;
		sampleRateSplitIncrement = sampleRate / sampleRateSplit;
		NI_ASSERT((sampleRate % sampleRateSplit) == 0, "Split must be divisible by sampleRate: %u", sampleRate);
	}
	~AudioRenderer()
	{
		shouldRunAudioThread = false;
		WaitForSingleObject(audioThreadHandle, INFINITE);

		while (!canDestroy()) {}

		ni::destroyPipelineState(audioProcess);
		ni::destroyBuffer(renderedBuffer);
		ni::destroyBuffer(readbackBuffer);
		copyFence->Release();
		CloseHandle(copyFenceEvent);
		delete audioData;
		free(cpuBuffer);
	}
	void renderAudioBufferGPU(ID3D12GraphicsCommandList* commandList, ni::DescriptorAllocator* descriptorAllocator, ni::ResourceBarrierBatcher& resourceBarriers)
	{
		if (renderState == RenderState::NOT_STARTED)
		{
			audioData->update(commandList);

			commandList->SetPipelineState(audioProcess->pso);
			commandList->SetComputeRootSignature(audioProcess->rootSignature);
			ni::DescriptorTable audioProcessDescriptorTable = descriptorAllocator->allocateDescriptorTable(2);
			audioProcessDescriptorTable.allocUAVBuffer(renderedBuffer->resource, nullptr, DXGI_FORMAT_UNKNOWN, 0, numFrames, sizeof(ni::Float2), 0);
			audioProcessDescriptorTable.allocCBVBuffer(audioData->buffer->resource, audioData->buffer->resource.apiResource->GetDesc().Width);
			commandList->SetComputeRootDescriptorTable(0, audioProcessDescriptorTable.gpuBaseHandle);
			commandList->Dispatch(numFrames / 256, 1, 1);

			resourceBarriers.transition(renderedBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			resourceBarriers.flush(commandList);
			commandList->CopyResource(readbackBuffer->resource.apiResource, renderedBuffer->resource.apiResource);
			resourceBarriers.flush(commandList);

			renderState = RenderState::DISPATCHED_TO_GPU;
		}
	}
	void processCPU()
	{
		if (renderState == RenderState::DISPATCHED_TO_GPU)
		{
			ni::getCommandQueue()->Signal(copyFence, ++copyFenceValue);
			renderState = RenderState::COPYING_TO_CPU;
		}
		else if (renderState == RenderState::COPYING_TO_CPU)
		{
			if (copyFence->GetCompletedValue() < copyFenceValue)
			{
				copyFence->SetEventOnCompletion(copyFenceValue, copyFenceEvent);
				WaitForSingleObject(copyFenceEvent, INFINITE);
			}

			if (cpuBuffer = malloc(cpuBufferSize))
			{
				void* readBufferData = nullptr;
				if (readbackBuffer->resource.apiResource->Map(0, nullptr, &readBufferData) == S_OK)
				{
					ni::Float2* dataAsFloat2 = (ni::Float2*)readBufferData;
					int16_t* pcmData = (int16_t*)cpuBuffer;

					for (uint32_t index = 0; index < numFrames; ++index)
					{
						ni::Float2 sample = dataAsFloat2[index];
						pcmData[index * numChannels + 0] = floatToPCM16(sample.x);
						pcmData[index * numChannels + 1] = floatToPCM16(sample.y);
					}
					readbackBuffer->resource.apiResource->Unmap(0, nullptr);
				}

				WAVEFORMATEXTENSIBLE  wfx{};
				wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
				wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
				wfx.Format.nChannels = numChannels;
				wfx.Format.nSamplesPerSec = sampleRate;
				wfx.Format.wBitsPerSample = bitsPerSample; // for float; use 16 for PCM16
				wfx.Format.nBlockAlign = (wfx.Format.wBitsPerSample / 8) * wfx.Format.nChannels;
				wfx.Format.nAvgBytesPerSec = wfx.Format.nBlockAlign * wfx.Format.nSamplesPerSec;
				wfx.Samples.wValidBitsPerSample = bitsPerSample;
				wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
				wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

				MMRESULT mmr = waveOutOpen(&waveOutHandle, WAVE_MAPPER, (WAVEFORMATEX*)&wfx, (DWORD_PTR)&AudioRenderer::waveOutCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);
				if (mmr == MMSYSERR_NOERROR)
				{
					for (uint32_t index = 0; index < WAVE_BLOCK_NUM; ++index)
					{
						waveHeader[index].lpData = nullptr;
						waveHeader[index].dwBufferLength = (DWORD)(sampleRateSplitIncrement * numChannels * sizeof(int16_t));
						waveHeader[index].dwFlags = 0;
						waveHeader[index].dwUser = 0;
						waveOutPrepareHeader(waveOutHandle, &waveHeader[index], sizeof(WAVEHDR));
					}
				}

				audioThreadHandle = CreateThread(nullptr, 4096, &AudioRenderer::runAudioThread, this, 0, nullptr);
				audioThreadHandle&& SetThreadPriority(audioThreadHandle, THREAD_PRIORITY_TIME_CRITICAL);
				renderState = RenderState::FINISHED;
			}
		}
	}
	void play()
	{
		if (hasFinishedRendering())
		{
			framesElapsed = 0;
			playing = true;
		}
	}
	void stop()
	{
		if (hasFinishedRendering())
		{
			framesElapsed = 0;
			playing = false;
		}
	}
	void pause()
	{
		if (hasFinishedRendering())
		{
			playing = false;
		}
	}
	void resume()
	{
		if (hasFinishedRendering())
		{
			playing = true;
		}
	}
	bool isState(RenderState state)
	{
		return renderState == state;
	}
	bool hasFinishedRendering() const
	{
		return renderState == RenderState::FINISHED;
	}
	bool isPlaying() const
	{
		return playing;
	}

private:
	bool canDestroy() const
	{
		bool hasStopped = true;
		for (uint32_t index = 0; index < WAVE_BLOCK_NUM; ++index)
		{
			hasStopped = hasStopped && waveHeader[index].dwUser == 0;
		}
		return /*!isWaitingForWaveOut &&*/ hasStopped;
	}

	void processAudioThreadFrame()
	{
		if (playing && framesElapsed < numFrames)
		{
			for (uint32_t index = 0; index < WAVE_BLOCK_NUM; ++index)
			{
				if (InterlockedCompareExchange(&waveHeader[index].dwUser, 1, 0) == 0)
				{
					waveHeader[index].lpData = (LPSTR)((int16_t*)cpuBuffer + framesElapsed * 2);
					MMRESULT r = waveOutWrite(waveOutHandle, &waveHeader[index], sizeof(WAVEHDR));
					if (r != MMSYSERR_NOERROR)
					{
						InterlockedExchange(reinterpret_cast<volatile LONG*>(&waveHeader[index].dwUser), 0);
						char errorMsg[256] = {};
						waveOutGetErrorTextA(r, errorMsg, 256);
						NI_LOG("Audio Error: %s", errorMsg);
						NI_DEBUG_BREAK();
					}
					else
					{
						InterlockedExchange8((char*)&isWaitingForWaveOut, 1);
					}

					if (framesElapsed < numFrames)
					{
						framesElapsed += sampleRateSplitIncrement;
					}
					else if (shouldLoop)
					{
						framesElapsed = 0;
					}
					else
					{
						playing = false;
					}
				}
			}
		}
	}

	RenderState renderState = RenderState::NOT_STARTED;
	bool playing = false;
	const uint32_t durationInSeconds = 5;
	const uint32_t sampleRate = 44100;
	const uint32_t numChannels = 2;
	const uint32_t bitsPerSample = 16;
	uint32_t numFrames = 0;
	uint32_t blockAlign = 0;
	uint32_t cpuBufferSize = 0;
	uint32_t framesElapsed = 0;
	uint32_t sampleRateSplit = 10;
	uint32_t sampleRateSplitIncrement = 0;
	ConstantBufferUploader<AudioData>* audioData;
	ni::PipelineState* audioProcess = nullptr;
	ni::Buffer* renderedBuffer = nullptr;
	ni::Buffer* readbackBuffer = nullptr;
	ID3D12Fence* copyFence = nullptr;
	uint64_t copyFenceValue = 0;
	HANDLE copyFenceEvent = INVALID_HANDLE_VALUE;
	void* cpuBuffer = nullptr;
	HWAVEOUT waveOutHandle = 0;
	WAVEHDR waveHeader[WAVE_BLOCK_NUM] = {};
	HANDLE audioThreadHandle = 0;
	uint32_t currentBufferIdx = 0;
	bool shouldRunAudioThread = true;
	bool isWaitingForWaveOut = true;
	bool shouldLoop = false;
};