#include "VulkanContext.hpp"

#include <set>

#include "debug_tools/Console.hpp"

#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VulkanContext::VulkanContext(const std::string &applicationName)
{
    if (createVulkanInstance(applicationName) != VK_SUCCESS) {
        throw std::runtime_error("Failed to initialize Vulkan");
    }
}

VulkanContext::~VulkanContext()
{
    vkDestroyInstance(m_instance, nullptr);
}

bool VulkanContext::init()
{
    if (m_initialized)
        return false;

    createDebugCallback();

    bool res = pickPhysicalDevice();
    if (!res) {
        debug_tools::ConsoleFatal("VulkanContext::init(): Failed to find suitable Vulkan device");
        return false;
    }

    createLogicalDevice();
    createCommandPool();

    m_initialized = true;
    return true;
}

bool VulkanContext::init(VkSurfaceKHR surface)
{
    if (m_initialized)
        return false;

    m_surface = surface;

    createDebugCallback();

    bool res = pickPhysicalDevice();
    if (!res) {
        debug_tools::ConsoleFatal("VulkanContext::init(): Failed to find suitable Vulkan device");
        return false;
    }

    debug_tools::ConsoleInfo("MSAA samples: " + std::to_string(msaaSamples()));

    createLogicalDevice();
    createCommandPool();

    m_initialized = true;
    return true;
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

void VulkanContext::createDebugCallback()
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
}

void VulkanContext::destroyDebugCallback()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debugCallback, nullptr);
    }
}

bool VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("VulkanContext::pickPhysicalDevice(): Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (int i = 0; i < devices.size(); i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        if (isPhysicalDeviceSuitable(devices[i], deviceProperties)) {
            m_physicalDevice = devices[i];
            m_physicalDeviceProperties = deviceProperties;

            m_msaaSamples = getMaxUsableSampleCount(deviceProperties);
            debug_tools::ConsoleInfo("Using device: " + std::string(deviceProperties.deviceName));
            return true;
        }
    }

    return false;
}

bool VulkanContext::isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties)
{
    bool suitable = true;
    suitable = suitable && getMaxUsableSampleCount(physicalDeviceProperties) != VK_SAMPLE_COUNT_1_BIT;
    suitable = suitable && checkDeviceExtensionSupport(physicalDevice);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, m_surface);
    if (!offlineMode()) {
        suitable = suitable && indices.isComplete();
    } else {
        suitable = suitable && indices.hasGraphics();
    }

    if (suitable && !offlineMode()) {
        SwapChainDetails details = querySwapChainSupport(physicalDevice, m_surface);
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

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> mandatoryExtensions;
    for (const auto &extension : VULKAN_DEVICE_EXTENSIONS) {
        if (extension.second)
            mandatoryExtensions.insert(extension.first);
    }

    for (const auto &extension : availableExtensions) {
        mandatoryExtensions.erase(extension.extensionName);
    }

    return mandatoryExtensions.empty();
}

bool VulkanContext::createLogicalDevice()
{
    m_queueFamilyIndices = findQueueFamilies(m_physicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<float> priorities = {1.0F, 0.F};  // First queue will be the render queue and it will have the highest priority
    if (!offlineMode()) {
        std::set<std::pair<uint32_t, uint32_t>> uniqueQueueFamilies = {{m_queueFamilyIndices.graphicsFamily.value(), 2},
                                                                       {m_queueFamilyIndices.presentFamily.value(), 1}};
        for (auto queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily.first;
            queueCreateInfo.queueCount = queueFamily.second;
            queueCreateInfo.pQueuePriorities = priorities.data();
            queueCreateInfos.push_back(queueCreateInfo);
        }
    } else {
        std::set<std::pair<uint32_t, uint32_t>> uniqueQueueFamilies = {{m_queueFamilyIndices.graphicsFamily.value(), 2}};
        for (auto queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily.first;
            queueCreateInfo.queueCount = queueFamily.second;
            queueCreateInfo.pQueuePriorities = priorities.data();
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }

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
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.geometryShader = VK_TRUE;
    deviceFeatures2.features.shaderInt64 = VK_TRUE;
    deviceFeatures2.features.shaderInt16 = VK_TRUE;
    deviceFeatures2.pNext = &indexing_features;

    std::vector<const char *> extensionNames;
    for (auto e : VULKAN_DEVICE_EXTENSIONS)
        extensionNames.push_back(e.first);

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
    VkResult ret = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

    if (ret != VK_SUCCESS) {
        throw std::runtime_error("VulkanContext::createLogicalDevice(): Failed to create logical device");
    }

    vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_renderQueue);
    vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 1, &m_graphicsQueue);

    if (!offlineMode()) {
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
    }

    return true;
}

void VulkanContext::createCommandPool()
{
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();
        VkResult ret = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool);
        if (ret != VK_SUCCESS) {
            throw std::runtime_error("VulkanContext::createCommandPool(): Failed to create a command pool");
        }
    }

    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

        VkResult ret = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_renderCommandPool);
        if (ret != VK_SUCCESS) {
            throw std::runtime_error("VulkanContext::createCommandPool(): Failed to create a command pool");
        }
    }
}

}  // namespace vengine