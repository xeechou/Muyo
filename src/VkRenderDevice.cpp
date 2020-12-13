#include "VkRenderDevice.h"

#include <cassert>

#include "Debug.h"
#include "RenderResourceManager.h"

#define DEBUG_DEVICE
#ifdef DEBUG_DEVICE
static VkDebugRenderDevice renderDevice;
#else
static VkRenderDevice renderDevice;
#endif

VkRenderDevice* GetRenderDevice()
{
    return &renderDevice;
}

void VkRenderDevice::Initialize(
    const std::vector<const char*>& vExtensionNames,
    const std::vector<const char*>& vLayerNames)
{
    HWInfo info;
    for (const auto& slayerName : vLayerNames)
    {
        assert(info.IsLayerSupported(slayerName));
    }
    for (const auto& sInstanceExtensionName : vExtensionNames)
    {
        assert(info.IsExtensionSupported(sInstanceExtensionName));
    }

    // Create instance
    // set physical device
    // Populate application info structure
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = vExtensionNames.size();
    createInfo.ppEnabledExtensionNames = vExtensionNames.data();

    createInfo.enabledLayerCount = vLayerNames.size();
    createInfo.ppEnabledLayerNames = vLayerNames.data();

    assert(vkCreateInstance(&createInfo, nullptr, &m_instance) == VK_SUCCESS);

    // Enumerate physical device supports these extensions and layers
    PickPhysicalDevice();
}

void VkRenderDevice::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(GetRenderDevice()->GetInstance(), &deviceCount, nullptr);
    assert(deviceCount != 0);
    //assert(deviceCount == 1 && "Has more than 1 physical device, need compatibility check");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(GetRenderDevice()->GetInstance(), &deviceCount, devices.data());
    m_physicalDevice = devices[0];
}

void VkRenderDevice::CreateDevice(
    const std::vector<const char*>& vDeviceExtensions,
    const std::vector<const char*>& layers,
    VkSurfaceKHR surface)   // For checking queue familiy compatibility with the swapchain surface
{
    // Query queue family support
    struct QueueFamilyIndice
    {
        int graphicsFamily = -1;
        int presentFamily = -1;
        bool isComplete() { return graphicsFamily >= 0 && presentFamily >= 0; }
    };
    // Find supported queue
    QueueFamilyIndice indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

    // TODO: Find the first queue family support both graphics and presentation
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // Check for graphics support
        if (queueFamily.queueCount > 0 &&
            queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        // Check for presentation support ( they can be in the same queeu
        // family)
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, surface,
                                             &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete()) break;
        i++;
    }

    // TODO: Handle queue family indices and add them to the device creation info

    int queueFamilyIndex = 0;  //TODO: Enumerate proper queue family for different usages
    float queuePriority = 1.0f;
    // Create a queue for each of the family
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // Queue are stored in the orders
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = vDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = vDeviceExtensions.data();

    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    assert(vkCreateDevice(GetRenderDevice()->GetPhysicalDevice(), &createInfo,
                          nullptr, &m_device) == VK_SUCCESS);

    {
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(GetRenderDevice()->GetDevice(), queueFamilyIndex,
                         0, &graphicsQueue);
        vkGetDeviceQueue(GetRenderDevice()->GetDevice(), queueFamilyIndex,
                         0, &presentQueue);

        SetGraphicsQueue(graphicsQueue, queueFamilyIndex);
        SetPresentQueue(presentQueue, queueFamilyIndex);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(graphicsQueue), VK_OBJECT_TYPE_QUEUE, "Graphics/Present Queue");
    }
}

void VkRenderDevice::DestroyDevice()
{
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
}

void VkRenderDevice::CreateCommandPools()
{
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.queueFamilyIndex = 0;
    // Static comamand pools
    {
        commandPoolInfo.flags = 0;
        assert(vkCreateCommandPool(GetRenderDevice()->GetDevice(),
                                   &commandPoolInfo, nullptr,
                                   &m_aCommandPools[MAIN_CMD_POOL]) == VK_SUCCESS);
    }
    // transient pool
    {
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        assert(vkCreateCommandPool(GetRenderDevice()->GetDevice(),
                                   &commandPoolInfo, nullptr,
                                   &m_aCommandPools[IMMEDIATE_CMD_POOL]) == VK_SUCCESS);
    }
    // Reusable pool
    {
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        assert(vkCreateCommandPool(GetRenderDevice()->GetDevice(),
                                   &commandPoolInfo, nullptr,
                                   &m_aCommandPools[PER_FRAME_CMD_POOL]) == VK_SUCCESS);
    }
}

void VkRenderDevice::DestroyCommandPools()
{
    for (auto& cmdPool : m_aCommandPools)
    {
        vkDestroyCommandPool(GetRenderDevice()->GetDevice(), cmdPool, nullptr);
    }
}

void VkRenderDevice::Unintialize()
{
    vkDestroyInstance(m_instance, nullptr);
}

VkCommandBuffer VkRenderDevice::AllocateStaticPrimaryCommandbuffer()
{
    return AllocatePrimaryCommandbuffer(MAIN_CMD_POOL);
}

void VkRenderDevice::FreeStaticPrimaryCommandbuffer(VkCommandBuffer& commandBuffer)
{
    FreePrimaryCommandbuffer(commandBuffer, MAIN_CMD_POOL);
}

VkCommandBuffer VkRenderDevice::AllocateReusablePrimaryCommandbuffer()
{
    return AllocatePrimaryCommandbuffer(PER_FRAME_CMD_POOL);
}

void VkRenderDevice::FreeReusablePrimaryCommandbuffer(VkCommandBuffer& commandBuffer)
{
    FreePrimaryCommandbuffer(commandBuffer, PER_FRAME_CMD_POOL);
}

VkCommandBuffer VkRenderDevice::AllocateImmediateCommandBuffer()
{
    return AllocatePrimaryCommandbuffer(IMMEDIATE_CMD_POOL);
}

void VkRenderDevice::FreeImmediateCommandBuffer(VkCommandBuffer& commandBuffer)
{
    FreePrimaryCommandbuffer(commandBuffer, IMMEDIATE_CMD_POOL);
}

VkCommandBuffer VkRenderDevice::AllocateSecondaryCommandBuffer()
{
    // TODO: Implement this
    return VkCommandBuffer();
}
void VkRenderDevice::FreeSecondaryCommandBuffer(VkCommandBuffer& commandBuffer)
{
    vkFreeCommandBuffers(m_device, m_aCommandPools[MAIN_CMD_POOL], 1,
                         &commandBuffer);
}

// Helper functions
VkSampler VkRenderDevice::CreateSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    // Wrapping
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Anistropic filter
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Choose of [0, width] or [0, 1]
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    // Used for percentage-closer filter for shadow
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // mipmaps
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    assert(vkCreateSampler(GetRenderDevice()->GetDevice(), &samplerInfo,
                           nullptr, &sampler) == VK_SUCCESS);
    return sampler;
}

VkCommandBuffer VkRenderDevice::AllocatePrimaryCommandbuffer(CommandPools pool)
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = m_aCommandPools[pool];
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    assert(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &commandBuffer) ==
           VK_SUCCESS);
    return commandBuffer;
}

void VkRenderDevice::FreePrimaryCommandbuffer(VkCommandBuffer& commandBuffer,
                                              CommandPools pool)
{
    vkFreeCommandBuffers(m_device, m_aCommandPools[pool], 1, &commandBuffer);
}

void VkDebugRenderDevice::Initialize(
    const std::vector<const char*>& vExtensionNames,
    const std::vector<const char*>& vLayerNames)
{
    // Append debug extension and layer names to the device
    std::vector<const char*> vDebugExtNames = vExtensionNames;
    vDebugExtNames.push_back(GetValidationExtensionName());

    std::vector<const char*> vDebugLayerNames = vLayerNames;
    vDebugLayerNames.push_back(GetValidationLayerName());

    VkRenderDevice::Initialize(vDebugExtNames, vDebugLayerNames);
    m_debugMessenger.Initialize(m_instance);
}

void VkDebugRenderDevice::Unintialize()
{
    m_debugMessenger.Uninitialize(m_instance);
    VkRenderDevice::Unintialize();
}

void VkDebugRenderDevice::CreateDevice(const std::vector<const char*>& vExtensionNames, const std::vector<const char*>& vLayerNames, VkSurfaceKHR surface)
{
    std::vector<const char*> vDebugLayerNames = vLayerNames;
    vDebugLayerNames.push_back(GetValidationLayerName());
    VkRenderDevice::CreateDevice(
        vExtensionNames, vLayerNames, surface);
}
