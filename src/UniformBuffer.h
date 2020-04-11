#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "VkMemoryAllocator.h"
#include "VkRenderDevice.h"

struct UnifromBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    static VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        return uboLayoutBinding;
    }
};

class UniformBuffer
{
public:
    UniformBuffer(size_t size = 0)
    {
        if (size != 0)
        {
            GetMemoryAllocator()->AllocateBuffer(
                size, BUFFER_USAGE, MEMORY_USAGE, m_buffer, m_allocation, "Uniform Buffer");
        }
    }
    void setData(const UnifromBufferObject& buffer, VkCommandPool commandPool, VkQueue queue)
    {
        void* pMappedMemory = nullptr;
        GetMemoryAllocator()->MapBuffer(m_allocation, &pMappedMemory);
        memcpy(pMappedMemory, (void*)&buffer, sizeof(UnifromBufferObject));
        GetMemoryAllocator()->UnmapBuffer(m_allocation);
    }
    ~UniformBuffer()
    {
        GetMemoryAllocator()->FreeBuffer(m_buffer, m_allocation);
    }

    VkBuffer buffer() const { return m_buffer; }
private:
    VkBuffer m_buffer = VK_NULL_HANDLE;

    VmaAllocation m_allocation = VK_NULL_HANDLE;
    const VkBufferUsageFlags BUFFER_USAGE = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VmaMemoryUsage MEMORY_USAGE = VMA_MEMORY_USAGE_GPU_ONLY;
};
