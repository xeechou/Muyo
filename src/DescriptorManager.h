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
    DESCRIPTOR_LAYOUT_LIGHTING,          // TODO: Remove this
    DESCRIPTOR_LAYOUT_SINGLE_SAMPLER,    // A single sampler descriptor set layout at binding 0
    DESCRIPTOR_LAYOUT_SIGNLE_STORAGE_IMAGE,
    DESCRIPTOR_LAYOUT_PER_VIEW_DATA,     // A layout contains mvp matrices at binding 0
    DESCRIPTOR_LAYOUT_PER_VIEW_DATA_RT,  // A layout contains mvp matrices at binding 0, for ray tracing
    DESCRIPTOR_LAYOUT_PER_OBJ_DATA,      // Per object data layout
    DESCRIPTOR_LAYOUT_MATERIALS,         // A sampler array contains material textures
    DESCRIPTOR_LAYOUT_GBUFFER,           // Layouts contains output of GBuffer
    DESCRIPTOR_LAYOUT_IBL,               // IBL descriptor sets
    DESCRIPTOR_LAYOUT_RAY_TRACING,       // Raytracing Descriptor sets
    DESCRIPTOR_LAYOUT_COUNT,
};

class DescriptorManager
{
public:
    void createDescriptorPool();
    void destroyDescriptorPool();
    void createDescriptorSetLayouts();
    void destroyDescriptorSetLayouts();

    VkDescriptorSet AllocateLightingDescriptorSet(
        const UniformBuffer<PerViewData> &perViewData, VkImageView position,
        VkImageView albedo, VkImageView normal, VkImageView uv); // Deprecating

    VkDescriptorSet AllocateGBufferDescriptorSet(
        const RenderPassGBuffer::GBufferViews &gbufferViews);
    VkDescriptorSet AllocateGBufferDescriptorSet();
    static void updateGBufferDescriptorSet(
        VkDescriptorSet descriptorSet,
        const RenderPassGBuffer::GBufferViews &gbufferViews);

    VkDescriptorSet AllocateSingleSamplerDescriptorSet(VkImageView textureView);
    VkDescriptorSet AllocateSingleStorageImageDescriptorSet(VkImageView imageView);

    VkDescriptorSet AllocatePerviewDataDescriptorSet(
        const UniformBuffer<PerViewData> &perViewData, bool bIsRT = false);

    // Material Descriptor Set
    VkDescriptorSet AllocateMaterialDescriptorSet();
    VkDescriptorSet AllocateMaterialDescriptorSet(const Material::MaterialParameters &materialParameters);
    static void UpdateMaterialDescriptorSet(VkDescriptorSet descriptorSet, const Material::MaterialParameters &materialParameters);

    // IBL descriptor set
    VkDescriptorSet AllocateIBLDescriptorSet();
    VkDescriptorSet AllocateIBLDescriptorSet(
            VkImageView irradianceMap,
            VkImageView prefilteredEnvMap,
            VkImageView specularBrdfLutMap
            );

    void UpdateIBLDescriptorSet(
            VkDescriptorSet& descriptorSet,
            VkImageView irradianceMap,
            VkImageView prefilteredEnvMap,
            VkImageView specularBrdfLutMap
            );

    // Ray Tracing descriptor set
    VkDescriptorSet AllocateRayTracingDescriptorSet(const VkAccelerationStructureKHR &acc, const VkImageView &outputImage);
    static void UpdateRayTracingDescriptorSet(VkDescriptorSet descriptorSet, const VkAccelerationStructureKHR &acc, const VkImageView &outputImage);


    VkDescriptorSetLayout getDescriptorLayout(DescriptorLayoutType type) const
    {
        return m_aDescriptorSetLayouts[type];
    }

    // Function template to allocate single type uniform buffer descriptor set
    template <class T>
    VkDescriptorSet AllocateUniformBufferDescriptorSet(const UniformBuffer<T> &uniformBuffer, uint32_t nBinding, const VkDescriptorSetLayout &descLayout)
    {
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        // Create descriptor sets
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descLayout;

        assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                        &descriptorSet) == VK_SUCCESS);

        // Bind uniform buffer to descriptor
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniformBuffer.buffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(T);
            std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSet;
            descriptorWrites[0].dstBinding = nBinding;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(),
                                   static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
        }

        return descriptorSet;
    }

private:
    static VkDescriptorSetLayoutBinding GetUniformBufferBinding(uint32_t binding, VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = binding;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = stages;
        return uboLayoutBinding;
    }
    static VkDescriptorSetLayoutBinding GetBinding(uint32_t nBinding, VkDescriptorType descType, uint32_t nDescCount = 1, VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_ALL)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = nBinding;
        binding.descriptorCount = nDescCount;
        binding.descriptorType = descType;
        binding.stageFlags = shaderStageFlags;
        return binding;
    }

    static VkDescriptorSetLayoutBinding GetSamplerArrayBinding(
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

    static VkDescriptorSetLayoutBinding GetInputAttachmentBinding(
        uint32_t binding, uint32_t numAttachments = 1)
    {
        VkDescriptorSetLayoutBinding descSetBinding;
        descSetBinding.binding = binding;
        descSetBinding.descriptorCount = numAttachments;
        descSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descSetBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        return descSetBinding;
    }

    static VkDescriptorSetLayoutBinding GetSamplerBinding(uint32_t binding)
    {
        return GetSamplerArrayBinding(binding, 1);
    }

    std::array<VkDescriptorSetLayout, DESCRIPTOR_LAYOUT_COUNT>
        m_aDescriptorSetLayouts = {VK_NULL_HANDLE};
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    const uint32_t DESCRIPTOR_COUNT_EACH_TYPE = 1000;
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
DescriptorManager *GetDescriptorManager();
