#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

namespace vengine {

enum class TextureType {
    NO_TYPE = 0,
    COLOR = 1,
    LINEAR = 2,
    HDR = 3,
};

class Texture {
public:
    Texture(std::string name, TextureType type, size_t width, size_t height) : 
        m_name(name), m_type(type), m_width(width), m_height(height) {};

    TextureType m_type;
    std::string m_name = "";
    size_t m_width;
    size_t m_height;

private:

};

}

#endif