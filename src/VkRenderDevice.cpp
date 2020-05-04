#include "VkRenderDevice.h"
#include "RenderResourceManager.h"
#include "Debug.h"
#include <cassert>

static VkRenderDevice renderDevice;

VkRenderDevice* GetRenderDevice() { return &renderDevice; }

void VkRenderDevice::Initialize(const std::vector<const char*>& layers,
                                const std::vector<const char*>& extensions)
{
    mExtensions = extensions;
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
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    createInfo.enabledLayerCount = layers.size();
    createInfo.ppEnabledLayerNames = layers.data();

    assert(vkCreateInstance(&createInfo, nullptr, &mInstance) == VK_SUCCESS);
    createPhysicalDevice();
}

void VkRenderDevice::createPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(GetRenderDevice()->GetInstance(), &deviceCount, nullptr);
    assert(deviceCount != 0);
    assert(deviceCount == 1 && "Has more than 1 physical device, need compatibility check");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(GetRenderDevice()->GetInstance(), &deviceCount, devices.data());
    mPhysicalDevice = devices[0];
}

void VkRenderDevice::createLogicalDevice(
    const std::vector<const char*>& layers,
    const std::vector<const char*>& extensions,
    VkSurfaceKHR surface
    )
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
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

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
        vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, surface,
                                             &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete()) break;
        i++;
    }

    // TODO: Handle queue family indices and add them to the device creation info

    int queueFamilyIndex = 0;       //TODO: Enumerate proper queue family for different usages
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

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    assert(vkCreateDevice(GetRenderDevice()->GetPhysicalDevice(), &createInfo,
                          nullptr, &mDevice) == VK_SUCCESS);

    {
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(GetRenderDevice()->GetDevice(), queueFamilyIndex,
                         0, &graphicsQueue);
        vkGetDeviceQueue(GetRenderDevice()->GetDevice(), queueFamilyIndex,
                         0, &presentQueue);

        SetGraphicsQueue(graphicsQueue, queueFamilyIndex);
        SetPresentQueue(presentQueue, queueFamilyIndex);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(graphicsQueue), VK_OBJECT_TYPE_QUEUE, "Graphics Queue");
    }
}

void VkRenderDevice::destroyLogicalDevice()
{
    vkDestroyDevice(mDevice, nullptr);
    mDevice = VK_NULL_HANDLE;
}

void VkRenderDevice::createCommandPools()
{
    VkCommandPoolCreateInfo commandPoolInfo = {};
    for (uint32_t i = 0; i < NUM_CMD_POOLS; i++)
    {
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.queueFamilyIndex = 0;
        commandPoolInfo.flags = 0;
        assert(vkCreateCommandPool(GetRenderDevice()->GetDevice(),
                                   &commandPoolInfo, nullptr,
                                   &maCommandPools[i]) == VK_SUCCESS);
    }
}

void VkRenderDevice::destroyCommandPools()
{
    for (auto& cmdPool : maCommandPools)
    {
        vkDestroyCommandPool(GetRenderDevice()->GetDevice(), cmdPool, nullptr);
    }
}

void VkRenderDevice::Unintialize()
{
    vkDestroyInstance(mInstance, nullptr);
}

VkCommandBuffer VkRenderDevice::allocatePrimaryCommandbuffer()
{
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = maCommandPools[MAIN_CMD_POOL];
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;
        assert(vkAllocateCommandBuffers(mDevice, &cmdAllocInfo,
                                        &commandBuffer) == VK_SUCCESS);
        return commandBuffer;
}

void VkRenderDevice::freePrimaryCommandbuffer(VkCommandBuffer& commandBuffer)
{
    vkFreeCommandBuffers(mDevice, maCommandPools[MAIN_CMD_POOL], 1,
                         &commandBuffer);
}

VkCommandBuffer VkRenderDevice::allocateSecondaryCommandBuffer()
{
    return VkCommandBuffer();
}
void VkRenderDevice::freeSecondaryCommandBuffer(VkCommandBuffer& commandBuffer)
{
    vkFreeCommandBuffers(mDevice, maCommandPools[MAIN_CMD_POOL], 1,
                         &commandBuffer);
}

VkCommandBuffer VkRenderDevice::allocateImmediateCommandBuffer()
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = maCommandPools[IMMEDIATE_CMD_POOL];
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    assert(vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &commandBuffer) ==
           VK_SUCCESS);
    return commandBuffer;
}

void VkRenderDevice::freeImmediateCommandBuffer(VkCommandBuffer& commandBuffer)
{
    vkFreeCommandBuffers(mDevice, maCommandPools[IMMEDIATE_CMD_POOL], 1,
                         &commandBuffer);
}

