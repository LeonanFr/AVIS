#pragma once
#include <cstdint>

enum class Direction : std::uint8_t
{
	Left,
	Right,
	Center,
	UpLeft,
	UpCenter,
	UpRight,
	CenterLeft,
	CenterRight,
	DownLeft,
	DownCenter,
	DownRight,
	Unknown
};