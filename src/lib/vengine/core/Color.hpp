#ifndef __Color_hpp
#define __Color_hpp

namespace vengine
{

enum class ColorDepth {
    BITS8 = 0,
    BITS16 = 1,
    BITS32 = 2,
};

enum class ColorSpace {
    sRGB = 0,
    LINEAR = 1,
};

enum class Color {
    BLACK = 0,
    WHITE = 1,
    RED = 2,
    GREEN = 3,
    BLUE = 4,
    NORMAL_MAP = 5,
};

}  // namespace vengine

#endif