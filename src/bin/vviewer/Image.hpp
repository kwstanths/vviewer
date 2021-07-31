#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

#include <stb_image.h>

class Image {
public:
    Image(std::string filename);
    ~Image();

    stbi_uc* getData();
    int getWidth();
    int getHeight();
    int getChannels();
    bool hasTransparency();

    static stbi_uc* loadDiskImage(std::string filename, int& width, int& height, int& channels);

private:
    int m_width, m_height, m_channels;
    stbi_uc* m_data;
};

#endif