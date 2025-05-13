#include "AudioCapturer.h"

#include <iostream>
#include <ostream>
#include <stdexcept>

#include "Direction.h"
#include "DirectionUtils.h"

AudioCapturer::AudioCapturer(std::atomic<int>& dirRef)
    : directionRef(dirRef), pwfx(nullptr)
{
    HRESULT hr = CoInitialize(nullptr);

    if (FAILED(hr)) {
        throw std::runtime_error("Erro ao inicializar COM");
    }

    initialize();
}

AudioCapturer::~AudioCapturer()
{
    CoUninitialize();
}

void AudioCapturer::run() const
{
    std::cout<<"Captura em tempo real inicializada...\n";

    while(true)
    {
        Sleep(10);

        UINT32 packetLength = 0;
        HRESULT hr = pCaptureClient->GetNextPacketSize(&packetLength);

        if (FAILED(hr)) continue;

        while(packetLength!= 0)
        {
            BYTE* pData = nullptr;
            UINT32 numFramesAvailable = 0;
            DWORD flags = 0;

            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            if((flags & AUDCLNT_BUFFERFLAGS_SILENT)==0 && pData)
            {
	            auto isFloat = false;
	            if(pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
	            {
                    isFloat = true;
	            } else if(pwfx->wFormatTag==WAVE_FORMAT_EXTENSIBLE)
	            {
		            if(const auto* wfext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(pwfx.get()); wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                    {
                        isFloat = true;
                    }
	            }

                if(isFloat)
                {
	                const auto floatData = reinterpret_cast<const float*>(pData);

	                const Direction dir = analyzer.analyze(floatData, numFramesAvailable, pwfx->nChannels);
                    if (dir == Direction::Left) directionRef = 1;
                    else if (dir == Direction::Right) directionRef = 2;
                    else directionRef = 0;
                    std::cout << directionToString(dir) << '\n';
                }
            }

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            if (FAILED(hr)) break;

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }
    }

}

void AudioCapturer::initialize()
{
    HRESULT hr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&pEnumerator));

    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao criar MMDeviceEnumerator");
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao obter dispositivo de áudio padrão");
    }

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &pAudioClient);
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao ativar IAudioClient");
    }

    WAVEFORMATEX* pWfxRaw = nullptr;

    hr = pAudioClient->GetMixFormat(&pWfxRaw);
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao obter formato de mixagem");
    }

    pwfx.reset(pWfxRaw);

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, pwfx.get(), nullptr);
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao inicializar IAudioClient");
    }

    hr = pAudioClient->GetService(IID_PPV_ARGS(&pCaptureClient));
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha ao obter IAudioCaptureClient");
    }

    hr = pAudioClient->Start();
    if(FAILED(hr))
    {
        throw std::runtime_error("Falha o iniciar captura de áudio");
    }
}
