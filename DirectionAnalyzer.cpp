#include "DirectionAnalyzer.h"

#include <iostream>
#include <vector>

Direction DirectionAnalyzer::analyze(const float* samples, unsigned int frameCount, int numChannels) const
{
	if (numChannels <= 0 || frameCount == 0 || !samples) {
		return Direction::Unknown;
	}

	std::vector energy(numChannels, 0.0f);

	for(unsigned i = 0; i<frameCount; ++i)
	{
		for(int ch = 0; ch<numChannels; ++ch)
		{
			energy[ch] += std::abs(samples[i * numChannels + ch]);
		}
	}

	float left = 0, right = 0, up = 0, down = 0;

	// Handle different channel configurations more accurately
	switch (numChannels) {
	case 1: // Mono
		return Direction::Center;
		
	case 2: // Stereo: [L, R]
		left = energy[0];
		right = energy[1];
		up = 0.0f;
		down = 0.0f;
		break;
		
	case 4: // Quadraphonic: [FL, FR, RL, RR]
		left = energy[0] + energy[2];  // Front Left + Rear Left
		right = energy[1] + energy[3]; // Front Right + Rear Right
		up = energy[0] + energy[1];    // Front channels
		down = energy[2] + energy[3];  // Rear channels
		break;
		
	case 5: // 5.0 surround: [FL, FR, FC, RL, RR]
		left = energy[0] + energy[3];  // Front Left + Rear Left
		right = energy[1] + energy[4]; // Front Right + Rear Right
		up = energy[0] + energy[1] + energy[2]; // Front channels + Center
		down = energy[3] + energy[4];  // Rear channels
		break;
		
	case 6: // 5.1 surround: [FL, FR, FC, LFE, RL, RR]
		left = energy[0] + energy[4];  // Front Left + Rear Left
		right = energy[1] + energy[5]; // Front Right + Rear Right
		up = energy[0] + energy[1] + energy[2]; // Front channels + Center
		down = energy[4] + energy[5] + energy[3]; // Rear channels + LFE
		break;
		
	case 8: // 7.1 surround: [FL, FR, FC, LFE, RL, RR, SL, SR]
		left = energy[0] + energy[4] + energy[6];  // FL + RL + SL
		right = energy[1] + energy[5] + energy[7]; // FR + RR + SR
		up = energy[0] + energy[1] + energy[2];    // Front channels + Center
		down = energy[4] + energy[5] + energy[6] + energy[7] + energy[3]; // Rear + Side + LFE
		break;
		
	default: // Fallback for other configurations
		// Assume even channels are left, odd are right
		for (int ch = 0; ch < numChannels; ++ch) {
			if (ch % 2 == 0) {
				left += energy[ch];
			} else {
				right += energy[ch];
			}
		}
		up = 0.0f;
		down = 0.0f;
		break;
	}

	const bool isLeft = left > right * 1.2f;
	const bool isRight = right > left * 1.2f;
	const bool isUp = up > down * 1.2f;
	const bool isDown = down > up * 1.2f;

	// For stereo, only return left/right/center
	if(numChannels == 2)
	{
		if (isLeft) return Direction::Left;
		if (isRight) return Direction::Right;
		return Direction::Center;
	}

	// For multi-channel, support full directional analysis
	if (isUp && isLeft) return Direction::UpLeft;
	if (isUp && isRight) return Direction::UpRight;
	if (isUp) return Direction::UpCenter;
	if (isDown && isLeft) return Direction::DownLeft;
	if (isDown && isRight) return Direction::DownRight;
	if (isDown) return Direction::DownCenter;
	if (isLeft) return Direction::CenterLeft;
	if (isRight) return Direction::CenterRight;

	return Direction::Center; // Changed from Unknown to Center for balanced audio

}