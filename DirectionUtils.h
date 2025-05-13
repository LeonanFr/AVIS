#pragma once
#include "Direction.h"
#include <string>

inline std::string directionToString(Direction dir) {
    switch (dir) {
		case Direction::Left: return "LEFT";
	    case Direction::Right: return "RIGHT";
		case Direction::Center: return "CENTER";
	    case Direction::UpLeft: return "UP LEFT";
	    case Direction::UpCenter: return "UP CENTER";
	    case Direction::UpRight: return "UP RIGHT";
	    case Direction::CenterLeft: return "CENTER LEFT";
	    case Direction::CenterRight: return "CENTER RIGHT";
	    case Direction::DownLeft: return "DOWN LEFT";
	    case Direction::DownCenter: return "DOWN CENTER";
	    case Direction::DownRight: return "DOWN RIGHT";
	    case Direction::Unknown: return "UNKNOWN";
		default: return "NADA";
    }
}