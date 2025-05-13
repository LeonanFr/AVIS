#include <iostream>
#include <sndfile.h>
#include <vector>
#include <cmath>

void AnalyzeAudioDirection(const std::string& filePath) {

    SF_INFO sfinfo;
    SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &sfinfo);
    if (!file) {
        std::cerr << "Erro ao abrir o arquivo de áudio: " << sf_strerror(nullptr) << std::endl;
        return;
    }

    if (sfinfo.channels != 2) {
        std::cerr << "O arquivo de áudio não é estéreo." << std::endl;
        sf_close(file);
        return;
    }

    const size_t BLOCK_SIZE = sfinfo.samplerate;
    std::vector<float> audioData(BLOCK_SIZE * sfinfo.channels);

    size_t secondsProcessed = 0;

    while (true) {
        sf_count_t framesRead = sf_readf_float(file, audioData.data(), BLOCK_SIZE);
        if (framesRead == 0) break;

        float leftSum = 0.0f;
        float rightSum = 0.0f;

        for (size_t i = 0; i < framesRead * 2; i += 2) {
            leftSum += std::fabs(audioData[i]);
            rightSum += std::fabs(audioData[i + 1]); 
        }

        leftSum /= framesRead;
        rightSum /= framesRead;

        float balance = (leftSum - rightSum) / (leftSum + rightSum + 1e-6f);

        std::cout << "Tempo: " << ++secondsProcessed << "s -> ";
        if (balance > 0.1f) {
            std::cout << "Som mais à ESQUERDA (balance = " << balance << ")" << std::endl;
        }
        else if (balance < -0.1f) {
            std::cout << "Som mais à DIREITA (balance = " << balance << ")" << std::endl;
        }
        else {
            std::cout << "Som CENTRALIZADO" << std::endl;
        }
    }

    sf_close(file);
}