#include "Shader.hpp"

#include "IncludeVulkan.hpp"

VkShaderModule Shader::load(VkDevice device, std::vector<char>& spvCode)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spvCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}
