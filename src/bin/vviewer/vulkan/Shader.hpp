#ifndef __Shader_hpp__
#define __Shader_hpp__

#include <vector>

#include "IncludeVulkan.hpp"

class Shader {
public:
    static VkShaderModule load(VkDevice device, const std::vector<char>& spvCode);

private:

};

#endif