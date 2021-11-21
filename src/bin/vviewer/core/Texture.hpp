#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

class Texture {
public:
    Texture(std::string name) : m_name(name) {};

    std::string m_name = "";

private:
};

#endif