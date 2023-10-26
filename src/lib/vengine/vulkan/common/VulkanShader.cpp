#include "VulkanShader.hpp"

#include <fstream>

#include "IncludeVulkan.hpp"

namespace vengine {

VkShaderModule VulkanShader::load(VkDevice device, std::string filename)
{
    std::vector<char> spvCode;
    bool ret = readSPIRV(filename, spvCode);
    if (!ret)
    {
        debug_tools::ConsoleCritical("VulkanShader::load(): Failed to parse shader file: " + filename);
        return VK_NULL_HANDLE;
    }

    return VulkanShader::load(device, spvCode);
}

VkShaderModule VulkanShader::load(VkDevice device, const std::vector<char>& spvCode)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spvCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());
    VkShaderModule shaderModule;
    auto ret = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (ret != VK_SUCCESS) {
        debug_tools::ConsoleCritical("VulkanShader::load(): Failed to create shader module: " + std::to_string(ret));
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

bool VulkanShader::readSPIRV(const std::string& filename, std::vector<char>& outSPVCode)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    outSPVCode.resize(fileSize);

    file.seekg(0);
    file.read(outSPVCode.data(), fileSize);

    file.close();
    return true;
}

}