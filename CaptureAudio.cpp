#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <iostream>
#include <comdef.h>
#include <fstream>
#include <vector>
#include <algorithm> // Para std::min e std::max

#pragma comment(lib, "Ole32.lib")

#define REFTIMES_PER_SEC  10000000  // Unidade de tempo para WASAPI (100 nanossegundos)

// Estrutura do cabeçalho WAV
struct WAVHeader {
    char chunkID[4] = { 'R', 'I', 'F', 'F' }; // "RIFF"
    uint32_t chunkSize;                       // Tamanho do arquivo - 8 bytes
    char format[4] = { 'W', 'A', 'V', 'E' }; // "WAVE"
    char subchunk1ID[4] = { 'f', 'm', 't', ' ' }; // "fmt "
    uint32_t subchunk1Size = 16;              // Tamanho do subchunk (16 para PCM)
    uint16_t audioFormat = 1;                 // Formato de áudio (1 para PCM)
    uint16_t numChannels;                     // Número de canais
    uint32_t sampleRate;                      // Taxa de amostragem
    uint32_t byteRate;                        // Bytes por segundo
    uint16_t blockAlign;                      // Alinhamento do bloco
    uint16_t bitsPerSample;                   // Bits por amostra
    char subchunk2ID[4] = { 'd', 'a', 't', 'a' }; // "data"
    uint32_t subchunk2Size;                   // Tamanho dos dados de áudio
};

// Função para escrever o cabeçalho WAV
void WriteWAVHeader(std::ofstream& file, const WAVHeader& header) {
    file.write(header.chunkID, 4);
    file.write(reinterpret_cast<const char*>(&header.chunkSize), 4);
    file.write(header.format, 4);
    file.write(header.subchunk1ID, 4);
    file.write(reinterpret_cast<const char*>(&header.subchunk1Size), 4);
    file.write(reinterpret_cast<const char*>(&header.audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&header.numChannels), 2);
    file.write(reinterpret_cast<const char*>(&header.sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&header.byteRate), 4);
    file.write(reinterpret_cast<const char*>(&header.blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&header.bitsPerSample), 2);
    file.write(header.subchunk2ID, 4);
    file.write(reinterpret_cast<const char*>(&header.subchunk2Size), 4);
}

// Função para normalizar áudio em formato float para PCM de 16 bits
void NormalizeAudio(const float* input, int16_t* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; i++) {
        float sample = input[i];
        sample = max(-1.0f, min(1.0f, sample)); // Limita o valor entre -1.0 e 1.0
        output[i] = static_cast<int16_t>(sample * 32767.0f); // Converte para PCM de 16 bits
    }
}

void CaptureAudio(const std::string& outputFile) {
    HRESULT hr;
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC; // 1 segundo de buffer
    UINT32 bufferFrameCount;
    BYTE* pData;
    DWORD flags;

    // Inicializa o COM
    hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao inicializar COM: " << _com_error(hr).ErrorMessage() << std::endl;
        return;
    }

    // Cria o enumerador de dispositivos de áudio
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao criar enumerador de dispositivos: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return;
    }

    // Obtém o dispositivo de áudio padrão (saída)
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao obter dispositivo de áudio padrão: " << _com_error(hr).ErrorMessage() << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Ativa o dispositivo de áudio
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao ativar dispositivo de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Obtém o formato do áudio
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao obter formato de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Verifica se o formato de áudio é suportado
    bool isFloat = false;
    if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        isFloat = true;
    }
    else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        // Verifica o subformat para WAVE_FORMAT_EXTENSIBLE
        WAVEFORMATEXTENSIBLE* pwfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
        if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            isFloat = true;
        }
        else if (pwfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            isFloat = false;
        }
        else {
            std::wcerr << "Formato de áudio não suportado (SubFormat desconhecido)." << std::endl;
            CoTaskMemFree(pwfx);
            pAudioClient->Release();
            pDevice->Release();
            pEnumerator->Release();
            CoUninitialize();
            return;
        }
    }
    else if (pwfx->wFormatTag != WAVE_FORMAT_PCM) {
        std::wcerr << "Formato de áudio não suportado." << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Inicializa o cliente de áudio no modo loopback
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, nullptr);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao inicializar cliente de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Obtém o cliente de captura
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr)) {
        std::wcerr << "Erro ao obter cliente de captura: " << _com_error(hr).ErrorMessage() << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Inicia a captura
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::wcerr << "Erro ao iniciar captura de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
        pCaptureClient->Release();
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Abre o arquivo WAV para escrita
    std::ofstream wavFile(outputFile, std::ios::binary);
    if (!wavFile.is_open()) {
        std::wcerr << "Erro ao abrir o arquivo WAV para escrita." << std::endl;
        pCaptureClient->Release();
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    // Prepara o cabeçalho WAV
    WAVHeader wavHeader;
    wavHeader.numChannels = pwfx->nChannels;
    wavHeader.sampleRate = pwfx->nSamplesPerSec; // Usa a taxa de amostragem do áudio capturado
    wavHeader.bitsPerSample = 16; // Forçamos PCM de 16 bits
    wavHeader.byteRate = wavHeader.sampleRate * wavHeader.numChannels * (wavHeader.bitsPerSample / 8);
    wavHeader.blockAlign = wavHeader.numChannels * (wavHeader.bitsPerSample / 8);

    // Escreve o cabeçalho WAV (o tamanho dos dados será atualizado posteriormente)
    WriteWAVHeader(wavFile, wavHeader);

    std::cout << "Capturando áudio de saída e salvando em " << outputFile << "..." << std::endl;

    // Loop de captura
    uint32_t totalBytesWritten = 0;
    std::vector<int16_t> pcmBuffer; // Buffer para armazenar dados PCM de 16 bits
    while (true) {
        // Obtém os dados capturados
        hr = pCaptureClient->GetBuffer(&pData, &bufferFrameCount, &flags, nullptr, nullptr);
        if (FAILED(hr)) {
            std::wcerr << "Erro ao obter buffer de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
            break;
        }

        if (bufferFrameCount > 0) {
            // Converte os dados para PCM de 16 bits (se necessário)
            if (isFloat) {
                // Normaliza os dados de float para PCM de 16 bits
                pcmBuffer.resize(bufferFrameCount * pwfx->nChannels);
                NormalizeAudio(reinterpret_cast<const float*>(pData), pcmBuffer.data(), bufferFrameCount * pwfx->nChannels);
                wavFile.write(reinterpret_cast<const char*>(pcmBuffer.data()), pcmBuffer.size() * sizeof(int16_t));
                totalBytesWritten += pcmBuffer.size() * sizeof(int16_t);
            }
            else {
                // Escreve os dados PCM diretamente
                wavFile.write(reinterpret_cast<const char*>(pData), bufferFrameCount * pwfx->nBlockAlign);
                totalBytesWritten += bufferFrameCount * pwfx->nBlockAlign;
            }
        }

        // Libera o buffer
        hr = pCaptureClient->ReleaseBuffer(bufferFrameCount);
        if (FAILED(hr)) {
            std::wcerr << "Erro ao liberar buffer de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
            break;
        }
    }

    // Atualiza o cabeçalho WAV com o tamanho dos dados
    wavHeader.chunkSize = totalBytesWritten + sizeof(WAVHeader) - 8;
    wavHeader.subchunk2Size = totalBytesWritten;
    wavFile.seekp(0);
    WriteWAVHeader(wavFile, wavHeader);

    // Fecha o arquivo WAV
    wavFile.close();

    // Para a captura
    hr = pAudioClient->Stop();
    if (FAILED(hr)) {
        std::wcerr << "Erro ao parar captura de áudio: " << _com_error(hr).ErrorMessage() << std::endl;
    }

    // Libera recursos
    pCaptureClient->Release();
    CoTaskMemFree(pwfx);
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
}