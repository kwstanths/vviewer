#ifndef __Shader_hpp__
#define __Shader_hpp__

#include <qvulkaninstance.h>

class Shader {
public:
    static VkShaderModule load(QVulkanInstance * instance, VkDevice device, std::vector<char>& spvCode);

private:

};

#endif