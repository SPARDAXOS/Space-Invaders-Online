#pragma once
#include "SFML/Graphics.hpp"

class Interpolator {
public:
	Interpolator();

public:
	void CalculateInterpolationDelay(float nowAsSeconds);
	sf::Vector2f Interpolate(sf::Vector2f current, sf::Vector2f target);

private:
	float InterpolationDelay;
	float LastReceivedPayloadTimestamp;
};