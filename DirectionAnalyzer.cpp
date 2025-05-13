#include "DirectionAnalyzer.h"

#include <iostream>
#include <vector>

Direction DirectionAnalyzer::analyze(const float* samples, unsigned int frameCount, int numChannels) const
{
	std::vector energy(numChannels, 0.0f);

	for(unsigned i = 0; i<frameCount; ++i)
	{
		for(int ch = 0; ch<numChannels; ++ch)
		{
			energy[ch] += std::abs(samples[i * numChannels + ch]);
		}
	}

	float left = 0, right = 0, up = 0, down = 0;

	if(numChannels>=6)
	{
		left = energy[0] + energy[4];
		right = energy[1] + energy[5];
		up = energy[0] + energy[1] + energy[2];
		down = energy[3] + energy[4] + energy[5];
	} else if(numChannels==2)
	{
		left = energy[0];
		right = energy[1];
		up = 0.0f;
		down = 0.0f;
	}


	const bool isLeft = left > right * 1.2f;
	const bool isRight = right > left * 1.2f;
	const bool isUp = up > down * 1.2f;
	const bool isDown = down > up * 1.2f;

	if(numChannels==2)
	{
		if (isLeft) return Direction::Left;
		if (isRight) return Direction::Right;
		if (!isLeft && !isRight) return Direction::Center;
	}

	if (isUp && isLeft) return Direction::UpLeft;
	if (isUp && isRight) return Direction::UpRight;
	if (isUp) return Direction::UpCenter;
	if (isDown && isLeft) return Direction::DownLeft;
	if (isDown && isRight) return Direction::DownRight;
	if (isDown) return Direction::DownCenter;
	if (isLeft) return Direction::CenterLeft;
	if (isRight) return Direction::CenterRight;

	return Direction::Unknown;

}