#pragma once
#include "Direction.h"

class DirectionAnalyzer
{
public:
	Direction analyze(const float* samples, unsigned int frameCount, int numChannels) const;
};
