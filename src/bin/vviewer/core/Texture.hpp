#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

enum class TextureType {
    NO_TYPE = 0,
    COLOR = 1,
    LINEAR = 2,
    HDR = 3,
};

class Texture {
public:
    Texture(std::string name, TextureType type) : m_name(name), m_type(type) {};

    TextureType m_type;
    std::string m_name = "";

private:

};

#endif