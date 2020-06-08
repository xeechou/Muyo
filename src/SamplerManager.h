#pragma once
#include "VkRenderDevice.h"
#include "Debug.h"
#include <vulkan/vulkan.h>
#include <array>
#include <cassert>
#include <cmath>
enum SamplerTypes
{
    SAMPLER_1_MIPS,
    SAMPLER_2_MIPS,
    SAMPLER_4_MIPS,
    SAMPLER_8_MIPS,
    SAMPLER_16_MIPS,
    SAMPLER_32_MIPS,
    SAMPLER_TYPE_COUNT
};
class SamplerManager
{
public:
    SamplerManager()
    {
        for (auto& sampler : m_aSamplers)
        {
            sampler = VK_NULL_HANDLE;
        }
    }
    void createSamplers()
    {
        // Frame sampler
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
        samplerInfo.maxLod = 1.0f;

        // full screen texture sampler
        for (int i = 0; i<SAMPLER_TYPE_COUNT; i++)
        {
            samplerInfo.maxLod = std::pow(2, i);
            assert(vkCreateSampler(GetRenderDevice()->GetDevice(), &samplerInfo,
                                   nullptr, &m_aSamplers[i]) == VK_SUCCESS);
            setDebugUtilsObjectName(reinterpret_cast<uint64_t>(m_aSamplers[i]),
                                    VK_OBJECT_TYPE_SAMPLER, "Frame Sampler");
        }
    }
    void destroySamplers()
    {
        for (auto& sampler : m_aSamplers)
        {
            vkDestroySampler(GetRenderDevice()->GetDevice(), sampler, nullptr);
        }
    }
    VkSampler getSampler(SamplerTypes type)
    {
        assert(m_aSamplers[type] != VK_NULL_HANDLE);
        return m_aSamplers[type];
    }
private:
    std::array<VkSampler, SAMPLER_TYPE_COUNT> m_aSamplers;
};

SamplerManager* GetSamplerManager();
