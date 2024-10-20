#include "VulkanContext.hpp"

#include <set>

#include "debug_tools/Console.hpp"

#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanDeviceFunctions.hpp"

namespace vengine
{

VulkanContext::VulkanContext(const std::string &applicationName)
{
    if (createVulkanInstance(applicationName) != VK_SUCCESS) {
        throw std::runtime_error("Failed to initialize a Vulkan instance");
    }
}

VulkanContext::~VulkanContext()
{
    vkDestroyInstance(m_instance, nullptr);
}

VkResult VulkanContext::init()
{
    if (m_initialized) {
        debug_tools::ConsoleWarning("VulkanContext::init(): Already initialized");
        return VK_ERROR_UNKNOWN;
    }

    VULKAN_CHECK_CRITICAL(createDebugCallback());
    VULKAN_CHECK_CRITICAL(pickPhysicalDevice());
    VULKAN_CHECK_CRITICAL(m_queueManager.init(*this));
    VULKAN_CHECK_CRITICAL(createLogicalDevice());
    VULKAN_CHECK_CRITICAL(createCommandPool());

    VulkanDeviceFunctions::getInstance().init(m_device);

    m_initialized = true;

    return VK_SUCCESS;
}

VkResult VulkanContext::init(VkSurfaceKHR surface)
{
    if (m_initialized) {
        debug_tools::ConsoleWarning("VulkanContext::init(): Already initialized");
        return VK_ERROR_UNKNOWN;
    }

    m_surface = surface;

    VULKAN_CHECK_CRITICAL(createDebugCallback());
    VULKAN_CHECK_CRITICAL(pickPhysicalDevice());
    VULKAN_CHECK_CRITICAL(m_queueManager.init(*this));
    VULKAN_CHECK_CRITICAL(createLogicalDevice());
    VULKAN_CHECK_CRITICAL(createCommandPool());

    VulkanDeviceFunctions::getInstance().init(m_device);

    m_initialized = true;

    return VK_SUCCESS;
}

void VulkanContext::destroy()
{
    if (!m_initialized)
        return;

    vkDestroyCommandPool(m_device, m_renderCommandPool, nullptr);
    vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);

    destroyDebugCallback();
}

VkResult VulkanContext::createVulkanInstance(const std::string &applicationName)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vengine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(VULKAN_INSTANCE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = VULKAN_INSTANCE_EXTENSIONS.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(VULKAN_VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VULKAN_VALIDATION_LAYERS.data();
    VULKAN_CHECK_FATAL(vkCreateInstance(&createInfo, nullptr, &m_instance));

    return VK_SUCCESS;
}

VkResult VulkanContext::createDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;  // Optional

    VkResult res;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        res = func(m_instance, &createInfo, nullptr, &m_debugCallback);
    } else {
        res = VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    if (res != VK_SUCCESS) {
        debug_tools::ConsoleWarning("VulkanContext::createDebugCallback(): Failed to setup the debug callback");
    }

    return res;
}

void VulkanContext::destroyDebugCallback()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debugCallback, nullptr);
    }
}

VkResult VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    VULKAN_CHECK_CRITICAL(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));

    if (deviceCount == 0) {
        debug_tools::ConsoleCritical("VulkanContext::pickPhysicalDevice(): Failed to find GPUs with Vulkan support");
        return VK_ERROR_UNKNOWN;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VULKAN_CHECK_CRITICAL(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));

    for (int i = 0; i < devices.size(); i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (isPhysicalDeviceSuitable(devices[i], deviceProperties)) {
            m_physicalDevice = devices[i];
            m_physicalDeviceProperties = deviceProperties;

            m_msaaSamples = getMaxUsableSampleCount(deviceProperties);
            debug_tools::ConsoleInfo("Using device: " + std::string(deviceProperties.deviceName));
            return VK_SUCCESS;
        }
    }

    debug_tools::ConsoleCritical("VulkanContext::pickPhysicalDevice(): Failed to find a suitable GPU");
    return VK_ERROR_UNKNOWN;
}

bool VulkanContext::isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties)
{
    bool suitable = true;
    suitable = suitable && (getMaxUsableSampleCount(physicalDeviceProperties) != VK_SAMPLE_COUNT_1_BIT);
    suitable = suitable && (checkDeviceExtensionSupport(physicalDevice) == VK_SUCCESS);

    std::vector<VulkanQueueFamilyInfo> queueFamilies = findQueueFamilies(physicalDevice, m_surface);
    bool hasPresent = false;
    bool hasGraphics = false;
    for (VulkanQueueFamilyInfo& queueFamily : queueFamilies)
    {
        if (queueFamily.hasGraphics()) {
            hasGraphics = true;
        }
        if (queueFamily.hasPresent()) {
            hasPresent = true;
        }
    }

    if (!offlineMode()) {
        suitable = suitable && hasGraphics && hasPresent;
    } else {
        suitable = suitable && hasGraphics;
    }

    if (suitable && !offlineMode()) {
        VulkanSwapChainDetails details = querySwapChainSupport(physicalDevice, m_surface);
        suitable = suitable && !details.formats.empty() && !details.presentModes.empty();
    }

    return suitable;
}

VkSampleCountFlagBits VulkanContext::getMaxUsableSampleCount(const VkPhysicalDeviceProperties &deviceProperties)
{
    VkSampleCountFlags counts =
        deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

VkResult VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    VULKAN_CHECK_CRITICAL(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    VULKAN_CHECK_CRITICAL(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));

    std::set<std::string> mandatoryExtensions;
    for (const auto &extension : VULKAN_DEVICE_EXTENSIONS) {
        if (extension.second)
            mandatoryExtensions.insert(extension.first);
    }

    for (const auto &extension : availableExtensions) {
        mandatoryExtensions.erase(extension.extensionName);
    }

    if (mandatoryExtensions.empty())
        return VK_SUCCESS;

    return VK_ERROR_UNKNOWN;
}

VkResult VulkanContext::createLogicalDevice()
{
    /* Physical device features needed for ray tracing */
    VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures = {};
    physicalDeviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    physicalDeviceBufferDeviceAddressFeatures.pNext = NULL;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures = {};
    physicalDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    physicalDeviceAccelerationStructureFeatures.pNext = &physicalDeviceBufferDeviceAddressFeatures;
    physicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureCaptureReplay = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureHostCommands = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = {};
    physicalDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    physicalDeviceRayTracingPipelineFeatures.pNext = &physicalDeviceAccelerationStructureFeatures;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_FALSE;
    VkPhysicalDevice16BitStorageFeatures deviceFeatures16Storage = {};
    deviceFeatures16Storage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    deviceFeatures16Storage.storageBuffer16BitAccess = VK_TRUE;
    deviceFeatures16Storage.pNext = &physicalDeviceRayTracingPipelineFeatures;
    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {};
    indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    indexing_features.runtimeDescriptorArray = VK_TRUE;
    indexing_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    indexing_features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    indexing_features.pNext = &deviceFeatures16Storage;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext = &indexing_features;
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.geometryShader = VK_TRUE;
    deviceFeatures2.features.shaderInt64 = VK_TRUE;
    deviceFeatures2.features.shaderInt16 = VK_TRUE;
    deviceFeatures2.features.independentBlend = VK_TRUE;
    deviceFeatures2.pNext = &rayQueryFeatures;

    std::vector<const char *> extensionNames;
    for (auto e : VULKAN_DEVICE_EXTENSIONS)
        extensionNames.push_back(e.first);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = m_queueManager.getQueueCreateInfo();

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = NULL;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    createInfo.ppEnabledExtensionNames = extensionNames.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(VULKAN_VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VULKAN_VALIDATION_LAYERS.data();
    createInfo.pNext = &deviceFeatures2;

    VULKAN_CHECK_CRITICAL(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));

    VULKAN_CHECK_CRITICAL(m_queueManager.createQueues(*this));

    return VK_SUCCESS;
}

VkResult VulkanContext::createCommandPool()
{
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueManager.graphicsQueueIndex().first;
        VULKAN_CHECK_CRITICAL(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool));
    }

    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueManager.renderQueueIndex().first;
        VULKAN_CHECK_CRITICAL(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_renderCommandPool));
    }

    return VK_SUCCESS;
}

}  // namespace vengine