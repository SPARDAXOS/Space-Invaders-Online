#include "interpolator.hpp"


Interpolator::Interpolator() {
	InterpolationDelay = 0.0f;
	LastReceivedPayloadTimestamp = 0.0f;
}
void Interpolator::CalculateInterpolationDelay(float nowAsSeconds) {
	InterpolationDelay = nowAsSeconds - LastReceivedPayloadTimestamp;
	LastReceivedPayloadTimestamp = nowAsSeconds;
}
sf::Vector2f Interpolator::Interpolate(sf::Vector2f current, sf::Vector2f target) {
	sf::Vector2f NewPosition;
	NewPosition = current + (target - current) * InterpolationDelay;
	return NewPosition;
}
