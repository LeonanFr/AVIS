#pragma once
#include <audioclient.h>
#include <memory>
#include <mmdeviceapi.h>

#include "DirectionAnalyzer.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct CoTaskDeleter
{
	void operator()(WAVEFORMATEX* p) const
	{
		if (p) CoTaskMemFree(p);
	}
};

class AudioCapturer
{
public:
	explicit AudioCapturer(std::atomic<int>& dirRef);
	~AudioCapturer();
	void run() const;
private:
	std::atomic<int>& directionRef;
	void initialize();

	DirectionAnalyzer analyzer;

	ComPtr<IMMDeviceEnumerator> pEnumerator;
	ComPtr<IMMDevice> pDevice;
	ComPtr<IAudioClient> pAudioClient;
	ComPtr<IAudioCaptureClient> pCaptureClient;


	std::unique_ptr<WAVEFORMATEX, CoTaskDeleter> pwfx;
};
