#pragma once
#include "SFML/Graphics.hpp"
#include <math.h>


namespace Rose {
	inline float Length(sf::Vector2f vector) {
		return std::sqrtf(vector.x * vector.x + vector.y * vector.y);
	}
	inline sf::Vector2f Abs(sf::Vector2f vector) {
		return sf::Vector2f(abs(vector.x), abs(vector.y));
	}
}
