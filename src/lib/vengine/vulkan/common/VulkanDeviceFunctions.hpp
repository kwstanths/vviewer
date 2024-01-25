#ifndef __VulkanDeviceFunctions_hpp__
#define __VulkanDeviceFunctions_hpp__

#include "IncludeVulkan.hpp"

namespace vengine
{

struct VulkanDeviceFunctionsRayTracing {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
};

class VulkanDeviceFunctions
{
    friend class VulkanContext;

public:
    static VulkanDeviceFunctions &getInstance()
    {
        static VulkanDeviceFunctions instance;
        return instance;
    }
    VulkanDeviceFunctions(VulkanDeviceFunctions const &) = delete;
    void operator=(VulkanDeviceFunctions const &) = delete;

    VulkanDeviceFunctionsRayTracing *rayTracingPipeline() { return &m_deviceFunctionsRayTracing; }

private:
    VulkanDeviceFunctions() {}

    VulkanDeviceFunctionsRayTracing m_deviceFunctionsRayTracing;

    void init(VkDevice device)
    {
        {
            m_deviceFunctionsRayTracing.vkGetBufferDeviceAddressKHR =
                reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
            m_deviceFunctionsRayTracing.vkCmdBuildAccelerationStructuresKHR =
                reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
                    vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
            m_deviceFunctionsRayTracing.vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
                vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
            m_deviceFunctionsRayTracing.vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
                vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
            m_deviceFunctionsRayTracing.vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
                vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
            m_deviceFunctionsRayTracing.vkGetAccelerationStructureBuildSizesKHR =
                reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
                    vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
            m_deviceFunctionsRayTracing.vkGetAccelerationStructureDeviceAddressKHR =
                reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
                    vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
            m_deviceFunctionsRayTracing.vkCmdTraceRaysKHR =
                reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
            m_deviceFunctionsRayTracing.vkGetRayTracingShaderGroupHandlesKHR =
                reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
                    vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
            m_deviceFunctionsRayTracing.vkCreateRayTracingPipelinesKHR =
                reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
        }
    }
};

}  // namespace vengine

#endif /* __VulkanDeviceFunctions_hpp__ */
