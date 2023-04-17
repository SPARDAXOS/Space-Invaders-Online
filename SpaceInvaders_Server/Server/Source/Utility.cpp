#include "Utility.hpp"

namespace Orchid {

	double Orchid::Log(double base, double x) {
		return double(log2(x) / log2(base));
	}

}