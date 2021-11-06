#ifndef __Image_hpp__
#define __Image_hpp__

#include <string>

#include <stb_image.h>


class Image {
public:
    enum class Color {
        BLACK = 0,
        WHITE = 1,
        RED = 2,
        GREEN = 3,
        BLUE = 4,
    };

    Image(std::string filename);
    Image(Color color);
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