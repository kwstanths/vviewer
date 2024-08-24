#ifndef __IDGeneration_hpp__
#define __IDGeneration_hpp__

#include <stdint.h>
#include <atomic>

#include <glm/glm.hpp>

namespace vengine
{

typedef uint32_t ID;

enum class ReservedObjectID {
    UNDEFINED = 0,
    TRANSFORM_ARROW_X = 1,
    TRANSFORM_ARROW_Y = 2,
    TRANSFORM_ARROW_Z = 3,

    TOTAL_RESERVED_IDS = 4,
};

/**
    A singleton that generates unique ids
*/
class IDGeneration
{
public:
    static IDGeneration &getInstance()
    {
        static IDGeneration instance;
        return instance;
    }
    IDGeneration(IDGeneration const &) = delete;
    void operator=(IDGeneration const &) = delete;

    ID generate();

    static glm::vec3 toRGB(ID id);
    static ID fromRGB(glm::vec3 rgb);

private:
    IDGeneration() {}

    std::atomic<uint32_t> m_index = static_cast<uint32_t>(ReservedObjectID::TOTAL_RESERVED_IDS);
};

}  // namespace vengine

#endif