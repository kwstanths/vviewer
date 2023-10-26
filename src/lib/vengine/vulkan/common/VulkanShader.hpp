#ifndef __Shader_hpp__
#define __Shader_hpp__

#include <vector>

#include "IncludeVulkan.hpp"

namespace vengine {

class VulkanShader {
public:
    /**
     * @brief Create a shader module from an .spv file
     * 
     * @param device 
     * @param filename 
     * @return VkShaderModule 
     */
    static VkShaderModule load(VkDevice device, std::string filename);

    /** Create a shader module from spv code
     * @brief 
     * 
     * @param device 
     * @param spvCode 
     * @return VkShaderModule 
     */
    static VkShaderModule load(VkDevice device, const std::vector<char>& spvCode);

    /**
     * @brief Read SPIRV code from a .spv file
     * 
     * @param filename 
     * @param outSPVCode 
     * @return true 
     * @return false 
     */
    static bool readSPIRV(const std::string& filename, std::vector<char>& outSPVCode);
};

}

#endif