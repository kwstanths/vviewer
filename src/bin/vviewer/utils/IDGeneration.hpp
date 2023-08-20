#ifndef __IDGeneration_hpp__
#define __IDGeneration_hpp__

#include <stdint.h>
#include <atomic>

#include <glm/glm.hpp>

namespace vengine {

typedef uint32_t ID;

enum class ReservedObjectID {
    UNDEFINED = 0,
    RIGHT_TRANSFORM_ARROW = 1,
    FORWARD_TRANSFORM_ARROW = 2,
    UP_TRANSFORM_ARROW = 3,
};

/**
    A singleton that generates unique ids
*/
class IDGeneration {
public:
    static IDGeneration& getInstance()
    {
        static IDGeneration instance;
        return instance;
    }
    IDGeneration(IDGeneration const&) = delete;
    void operator=(IDGeneration const&) = delete;

    ID getID();

    static glm::vec3 toRGB(ID id);
    static ID fromRGB(glm::vec3 rgb);

private:
    IDGeneration() {}

    std::atomic<ID> m_index = 4;
};

}

#endif