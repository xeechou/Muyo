#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image.h"

Texture::Texture()
    : m_image(VK_NULL_HANDLE),
      m_imageView(VK_NULL_HANDLE),
      m_textureSampler(VK_NULL_HANDLE)
{
}

Texture::~Texture()
{
    if (GetRenderDevice()->GetDevice() != VK_NULL_HANDLE)
    {
        vkDestroySampler(GetRenderDevice()->GetDevice(), m_textureSampler, nullptr);
        vkDestroyImageView(GetRenderDevice()->GetDevice(), m_imageView, nullptr);
        GetMemoryAllocator()->FreeImage(m_image, m_allocation);
    }
}

void Texture::LoadPixels(void *pixels, int width, int height,
                         const VkCommandPool &pool, const VkQueue &queue)
{
    // Upload pixels to staging buffer
    const size_t BUFFER_SIZE = width * height * 4;
    VmaAllocation stagingAllocation;
    VkBuffer stagingBuffer;
    GetMemoryAllocator()->AllocateBuffer(
        BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation);

    void *pMappedMemory = nullptr;
    GetMemoryAllocator()->MapBuffer(stagingAllocation, &pMappedMemory);
    memcpy(pMappedMemory, pixels, BUFFER_SIZE);
    GetMemoryAllocator()->UnmapBuffer(stagingAllocation);
    stbi_image_free(pixels);

    createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

    // UNDEFINED -> DST_OPTIMAL
    sTransitionImageLayout(GetRenderDevice()->GetDevice(), pool, queue, m_image,
                           VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy source to image
    mCopyBufferToImage(GetRenderDevice()->GetDevice(), pool, queue, stagingBuffer,
                       m_image, static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height));

    // DST_OPTIMAL -> SHADER_READ_ONLY
    sTransitionImageLayout(GetRenderDevice()->GetDevice(), pool, queue, m_image,
                           VK_FORMAT_R8G8B8A8_UNORM,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    mInitImageView();
    mInitSampler();
}

void Texture::LoadImage(const std::string path, const VkDevice &device,
                        const VkPhysicalDevice &physicalDevice,
                        const VkCommandPool &pool, const VkQueue &queue)
{

    int width, height, channels;
    stbi_uc *pixels =
        stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(pixels);
    LoadPixels((void *)pixels, width, height, pool, queue);
}
