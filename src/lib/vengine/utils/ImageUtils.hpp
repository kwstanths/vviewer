#ifndef __ImageUtils_hpp__
#define __ImageUtils_hpp__

#include <cmath>

namespace vengine {

float linearToSRGB(float linear);

float sRGBToLinear(float srgb);

}

#endif