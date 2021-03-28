#pragma once
#include <vulkan/vulkan.h>

#include <cstring>
#include <glm/glm.hpp>

#include "VkMemoryAllocator.h"
#include "VkRenderDevice.h"
#include "RenderResource.h"

struct PerViewData
{
    glm::mat4 model = glm::mat4(1.0);
    glm::mat4 view = glm::mat4(1.0);
    glm::mat4 proj = glm::mat4(1.0);

    glm::mat4 objectToView = glm::mat4(1.0);
    glm::mat4 viewToObject = glm::mat4(1.0);
    glm::mat4 normalObjectToView = glm::mat4(1.0);
};

struct PerGeometryData
{
    glm::mat4 model = glm::mat4(1.0);
};

template <class T>
class UniformBuffer : public BufferResource
{
public:
    UniformBuffer()
        : BufferResource(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VMA_MEMORY_USAGE_CPU_TO_GPU)

    {
        const size_t size = sizeof(T);
        GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE, MEMORY_USAGE,
                                             m_buffer, m_allocation,
                                             "Uniform Buffer");
    }
    void setData(const T& buffer)
    {
        void* pMappedMemory = nullptr;
        GetMemoryAllocator()->MapBuffer(m_allocation, &pMappedMemory);
        memcpy(pMappedMemory, (void*)&buffer, sizeof(T));
        GetMemoryAllocator()->UnmapBuffer(m_allocation);
    }
};
