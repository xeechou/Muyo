#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>


class DebugUtilsMessenger
{
public:
    void initialize();
    void uninitialize();
private:
    VkDebugUtilsMessengerEXT m_debugUtilsMessenger = VK_NULL_HANDLE;
};

const std::vector<const char*>& getValidationLayerNames();

const char* getValidationExtensionName();

VkResult setDebugUtilsObjectName(uint64_t objectHandle, VkObjectType objectType,
                                 const char* sName);


// Scoped markers
void beginMarker(VkQueue queue, std::string&& name, uint64_t color);
void endMarker(VkQueue queue);

void beginMarker(VkCommandBuffer cmd, std::string&& name, uint64_t color);
void endMarker(VkCommandBuffer cmd);

// Using template to handle command buffer and queue markers
template <class T>
class ScopedMarker
{
public:
    ScopedMarker(T vkObj, std::string&& marker)
    {
        m_markedVkObject = vkObj;
        beginMarker(vkObj, std::forward<std::string>(marker), uint64_t(0));
    }
    ~ScopedMarker()
    {
        endMarker(m_markedVkObject);
    }
private:
    T m_markedVkObject = VK_NULL_HANDLE;
};

// Create a macro to generate local variables 
#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define SCOPED_MARKER(OBJ, MESSAGE)\
    auto TOKENPASTE2(scoped_marker_,__LINE__) = ScopedMarker(OBJ, MESSAGE)

