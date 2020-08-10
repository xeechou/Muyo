#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <string>
#include <vector>

#include "Material.h"
#include "RenderPassGBuffer.h"
#include "UniformBuffer.h"

enum DescriptorLayoutType
{
    DESCRIPTOR_LAYOUT_LIGHTING,        // TODO: Remove this
    DESCRIPTOR_LAYOUT_SINGLE_SAMPLER,  // A single sampler descriptor set layout
                                       // at binding 0
    DESCRIPTOR_LAYOUT_PER_VIEW_DATA,   // A layout contains mvp matrices at
                                       // binding 0
    DESCRIPTOR_LAYOUT_MATERIALS,  // A sampler array contains material textures
    DESCRIPTOR_LAYOUT_GBUFFER,    // Layouts contains output of GBuffer
    DESCRIPTOR_LAYOUT_COUNT,
};
class DescriptorManager
{
public:
    void createDescriptorPool();
    void destroyDescriptorPool();
    void createDescriptorSetLayouts();
    void destroyDescriptorSetLayouts();

    VkDescriptorSet allocateLightingDescriptorSet(
        const UniformBuffer<PerViewData>& perViewData, VkImageView position,
        VkImageView albedo, VkImageView normal, VkImageView uv);  // Deprecating

    VkDescriptorSet allocateGBufferDescriptorSet(
        const RenderPassGBuffer::GBufferViews& gbufferViews);
    VkDescriptorSet allocateGBufferDescriptorSet();
    static void updateGBufferDescriptorSet(
        VkDescriptorSet descriptorSet,
        const RenderPassGBuffer::GBufferViews& gbufferViews);

    VkDescriptorSet allocateImGuiDescriptorSet(VkImageView textureView);

    VkDescriptorSet allocatePerviewDataDescriptorSet(
        const UniformBuffer<PerViewData>& perViewData);

    VkDescriptorSet allocateMaterialDescriptorSet();
    VkDescriptorSet allocateMaterialDescriptorSet(
        const Material::PBRViews& pbrViews);
    static void updateMaterialDescriptorSet(VkDescriptorSet descriptorSet,
                                            const Material::PBRViews& pbrViews);

    VkDescriptorSetLayout getDescriptorLayout(DescriptorLayoutType type) const
    {
        return m_aDescriptorSetLayouts[type];
    }

private:
    static VkDescriptorSetLayoutBinding getPerViewDataBinding(uint32_t binding)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = binding;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        return uboLayoutBinding;
    }

    static VkDescriptorSetLayoutBinding getSamplerArrayBinding(
        uint32_t binding, uint32_t numSamplers)
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = binding;
        samplerLayoutBinding.descriptorCount = numSamplers;
        samplerLayoutBinding.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        return samplerLayoutBinding;
    }

    static VkDescriptorSetLayoutBinding getSamplerBinding(uint32_t binding)
    {
        return getSamplerArrayBinding(binding, 1);
    }

    std::array<VkDescriptorSetLayout, DESCRIPTOR_LAYOUT_COUNT>
        m_aDescriptorSetLayouts = {VK_NULL_HANDLE};
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    const uint32_t DESCRIPTOR_COUNT_EACH_TYPE = 100;
    const std::vector<VkDescriptorPoolSize> POOL_SIZES = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DESCRIPTOR_COUNT_EACH_TYPE},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DESCRIPTOR_COUNT_EACH_TYPE}};
};
DescriptorManager* GetDescriptorManager();
