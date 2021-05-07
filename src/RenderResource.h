#pragma once
#include <vk_mem_alloc.h>
#include <cassert>
#include <vulkan/vulkan_core.h>

#include "Debug.h"
#include "VkMemoryAllocator.h"
#include "VkRenderDevice.h"

class IRenderResource
{
public:
    virtual ~IRenderResource(){};
    virtual VkObjectType GetVkObjectType() const = 0;
    virtual void SetDebugName(const std::string& sName) const = 0;
};

class ImageResource : public IRenderResource
{
public:
    VkImageView getView() const { return m_view; }
    VkImage getImage() const { return m_image; }
    virtual ~ImageResource()
    {
        vkDestroyImageView(GetRenderDevice()->GetDevice(), m_view, nullptr);
        GetMemoryAllocator()->FreeImage(m_image, m_allocation);
    }
    virtual VkObjectType GetVkObjectType() const override
    {
        return VK_OBJECT_TYPE_IMAGE;
    }
    virtual void SetDebugName(const std::string& sName) const override
    {
        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(m_image),
                                GetVkObjectType(), sName.c_str());
    }
protected:

    void CreateImageInternal(const VmaMemoryUsage &memoryUsage)
    {
        GetMemoryAllocator()->AllocateImage(
            &m_imageInfo, memoryUsage, m_image, m_allocation);
    }

    void CreateImageViewInternal()
    {
        assert(m_image != VK_NULL_HANDLE);
        m_imageViewInfo.image = m_image;
        m_imageViewInfo.format = m_imageInfo.format;

        assert(vkCreateImageView(GetRenderDevice()->GetDevice(), &m_imageViewInfo,
                                 nullptr, &m_view) == VK_SUCCESS);
    }

    static void TransitionImageLayout(VkImage image,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout,
                                      uint32_t nMipCount = 1,
                                      uint32_t nLayerCount = 1);

    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkImageCreateInfo m_imageInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr,
            0,                          // flags
            VK_IMAGE_TYPE_2D,           // imageType
            VK_FORMAT_UNDEFINED,        // Format
            {0, 0, 0},                  // Extent
            1,                          // mipLevels;
            1,                          // arrayLayers;
            VK_SAMPLE_COUNT_1_BIT,      // samples;
            VK_IMAGE_TILING_OPTIMAL,    // tiling;
            0,                          // usage;
            VK_SHARING_MODE_EXCLUSIVE,  // sharingMode;
            0,                          // queueFamilyIndexCount;
            nullptr,                    // pQueueFamilyIndices;
            VK_IMAGE_LAYOUT_UNDEFINED   // initialLayout;
        };

    VkImageViewCreateInfo m_imageViewInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType;
        nullptr,                                   // pNext;
        0,                                         // flags;
        VK_NULL_HANDLE,                            // image;
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType;
        VK_FORMAT_UNDEFINED,                       // format;
        {},                                        // components;
        {}                                         // subresourceRange;
    };
};

class BufferResource : public IRenderResource
{
public:
    BufferResource(VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage)
        : BUFFER_USAGE(bufferUsage), MEMORY_USAGE(memoryUsage)
    {
    }
    virtual VkBuffer buffer() const { return m_buffer; }
    virtual ~BufferResource()
    {
        GetMemoryAllocator()->FreeBuffer(m_buffer, m_allocation);
    }
    virtual VkObjectType GetVkObjectType() const override
    {
        return VK_OBJECT_TYPE_BUFFER;
    }

    virtual void SetDebugName(const std::string& sName) const override
    {
        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(m_buffer),
                                GetVkObjectType(), sName.c_str());
    }
    void SetData(const void* pData, size_t size)
    {
        if (m_buffer != VK_NULL_HANDLE && size > m_nSize)
        {
            GetMemoryAllocator()->FreeBuffer(m_buffer, m_allocation);
            m_buffer = VK_NULL_HANDLE;
        }
        if (m_buffer == VK_NULL_HANDLE)
        {
            GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE,
                                                 MEMORY_USAGE, m_buffer,
                                                 m_allocation);
        }
        m_nSize = size;

        if (BUFFER_USAGE & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        {
            assert(pData != nullptr && "Need to have data to upload");
            // Create staging buffer
            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VmaAllocation stagingAllocation = VK_NULL_HANDLE;
            GetMemoryAllocator()->AllocateBuffer(
                size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                stagingBuffer, stagingAllocation);

            void* pMappedMemory = nullptr;
            GetMemoryAllocator()->MapBuffer(stagingAllocation, &pMappedMemory);
            memcpy(pMappedMemory, pData, size);
            GetMemoryAllocator()->UnmapBuffer(stagingAllocation);

            // Submit the copy immedietly
            GetRenderDevice()->ExecuteImmediateCommand(
                [&](VkCommandBuffer commandBuffer) {
                    VkBufferCopy copyRegion = {};
                    copyRegion.size = size;
                    vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_buffer, 1,
                                    &copyRegion);
                    // Make sure the copy of the instance buffer are copied before triggering the
                    // acceleration structure build
                    VkMemoryBarrier barrier{
                        VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                        nullptr, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR};
                    vkCmdPipelineBarrier(commandBuffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                         0, 1, &barrier, 0, nullptr, 0, nullptr);
                });

            GetMemoryAllocator()->FreeBuffer(stagingBuffer, stagingAllocation);
        }
        else
        {
            if (pData != nullptr)
            {
                void* pMappedMemory = nullptr;
                GetMemoryAllocator()->MapBuffer(m_allocation, &pMappedMemory);
                memcpy(pMappedMemory, pData, size);
                GetMemoryAllocator()->UnmapBuffer(m_allocation);
            }
        }
    }

protected:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    uint32_t m_nSize;
    const VkBufferUsageFlags BUFFER_USAGE = 0x0;
    const VmaMemoryUsage MEMORY_USAGE = VMA_MEMORY_USAGE_UNKNOWN;
};


// Ray tracing related buffers
class AccelerationStructureBuffer : public BufferResource
{
public:
    AccelerationStructureBuffer(uint32_t size)
        : BufferResource(
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT |
                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
              VMA_MEMORY_USAGE_GPU_ONLY)
    {
        GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE, MEMORY_USAGE,
                                             m_buffer, m_allocation,
                                             "AccelerationStrucutre");
    }

    AccelerationStructureBuffer(const void* pData, uint32_t size)
        : BufferResource(
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT |
                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY)
    {
        GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE, MEMORY_USAGE,
                                             m_buffer, m_allocation,
                                             "AccelerationStrucutre");
        SetData(pData, size);
    }
};

class ShaderBindingTableBuffer : public BufferResource
{
public:
    //ShaderBindingTableBuffer(uint32_t size)
    //    : BufferResource(
    //          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT |
    //              VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
    //          VMA_MEMORY_USAGE_GPU_ONLY)
    //{
    //    GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE, MEMORY_USAGE,
    //                                         m_buffer, m_allocation,
    //                                         "SBT");
    //}

    ShaderBindingTableBuffer(const void* pData, uint32_t size)
        : BufferResource(
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT |
                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY)
    {
        GetMemoryAllocator()->AllocateBuffer(size, BUFFER_USAGE, MEMORY_USAGE,
                                             m_buffer, m_allocation,
                                             "SBT");
        SetData(pData, size);
    }
};
