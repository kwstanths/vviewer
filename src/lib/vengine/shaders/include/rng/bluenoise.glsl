float getBlueNoise(uint texture, uvec2 pixel)
{
    texture = texture % BLUE_NOISE_TEXTURES;
    pixel.x = pixel.x % BLUE_NOISE_RESOLUTION;
    pixel.y = pixel.y % BLUE_NOISE_RESOLUTION;

    return blueNoiseData.data[texture][pixel.x][pixel.y];
}