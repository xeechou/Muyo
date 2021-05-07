#include "DescriptorManager.h"

#include <cassert>
#include <vulkan/vulkan_core.h>

#include "Debug.h"
#include "SamplerManager.h"
#include "VkRenderDevice.h"

// Poor man's singletone
static DescriptorManager descriptorManager;

void DescriptorManager::createDescriptorPool()
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(POOL_SIZES.size());
    poolInfo.pPoolSizes = POOL_SIZES.data();
    poolInfo.maxSets =
        DESCRIPTOR_COUNT_EACH_TYPE * static_cast<uint32_t>(POOL_SIZES.size());
    assert(vkCreateDescriptorPool(GetRenderDevice()->GetDevice(), &poolInfo,
                                  nullptr, &m_descriptorPool) == VK_SUCCESS);
}

void DescriptorManager::destroyDescriptorPool()
{
    vkDestroyDescriptorPool(GetRenderDevice()->GetDevice(), m_descriptorPool,
                            nullptr);
}

void DescriptorManager::createDescriptorSetLayouts()
{
    // GBuffer lighting layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
            GetUniformBufferBinding(0),      // Pre view data
            GetSamplerArrayBinding(1, 4),  // gbuffers
        };
        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "Lighting");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_LIGHTING] = layout;
    }

    // Single sampler descriptor set layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
            GetSamplerBinding(0),
        };
        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;

        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "Single sampler");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_SINGLE_SAMPLER] = layout;
    }

    {
        // Single storage image
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
            GetBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT)};

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;

        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "Single storage image");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_SIGNLE_STORAGE_IMAGE] = layout;
   
    }

    // Per view layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
            GetUniformBufferBinding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "PerViewData");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_PER_VIEW_DATA] = layout;
    }

    // Per view ray tracing layout
    // This layout is exactly the same layout of PER_VIEW_DATA, but used for ray tracing pipeline
    // This is because ray tracing pipeline descriptor set layout can not be used with non ray tracing one in the same pipeline layout.
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
            GetUniformBufferBinding(0,  VK_SHADER_STAGE_RAYGEN_BIT_KHR)};

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "PerViewDataRt");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_PER_VIEW_DATA_RT] = layout;
    }

    // Per obj layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
            GetUniformBufferBinding(0)};    // VERTEX and FRAGMENT !?

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "PerObjData");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_PER_OBJ_DATA] = layout;
    }

    // Material layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
            GetSamplerArrayBinding(0, Material::TEX_COUNT),
            GetUniformBufferBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT)
        }; // PBR material

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "Material");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_MATERIALS] = layout;
    }

    // GBuffer layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, RenderPassGBuffer::LightingAttachments::GBUFFER_ATTACHMENTS_COUNT> bindings = {
            GetInputAttachmentBinding(0, 1),
            GetInputAttachmentBinding(1, 1),
            GetInputAttachmentBinding(2, 1),
            GetInputAttachmentBinding(3, 1)};  // GBuffer textures

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "GBuffer");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_GBUFFER] = layout;
    }

    // IBL layout
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
            GetSamplerBinding(0),    // Irradiance map
            GetSamplerBinding(1),    // prefiltered environment 
            GetSamplerBinding(2)};   // specular brdf lut

        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);

        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(layout),
                                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                "IBL");
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_IBL] = layout;
    }

    // Ray tracing
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};

        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
            GetBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR),
            GetBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR)};
        descriptorSetLayoutInfo.bindingCount = (uint32_t)bindings.size();
        descriptorSetLayoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        assert(vkCreateDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                           &descriptorSetLayoutInfo, nullptr,
                                           &layout) == VK_SUCCESS);
        m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_RAY_TRACING] = layout;
    }
}

void DescriptorManager::destroyDescriptorSetLayouts()
{
    for (auto& descriptorSetLayout : m_aDescriptorSetLayouts)
    {
        vkDestroyDescriptorSetLayout(GetRenderDevice()->GetDevice(),
                                     descriptorSetLayout, nullptr);
    }
}

VkDescriptorSet DescriptorManager::AllocateMaterialDescriptorSet()
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts =
        &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_MATERIALS];
    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "Material");

    return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocateMaterialDescriptorSet(const Material::MaterialParameters& materialParameters)
{
    VkDescriptorSet descSet = AllocateMaterialDescriptorSet();
    UpdateMaterialDescriptorSet(descSet, materialParameters);
    return descSet;
}

void DescriptorManager::UpdateMaterialDescriptorSet(VkDescriptorSet descriptorSet, const Material::MaterialParameters &materialParameters)
{
    assert(materialParameters.m_apTextures.size() == Material::TEX_COUNT);

    // prepare image array info for descriptor
    std::array<VkDescriptorImageInfo, Material::TEX_COUNT> imageInfos = {};
    for (size_t i = 0; i < Material::TEX_COUNT; i++)
    {
        imageInfos[i] = {
            GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
            materialParameters.m_apTextures[i]->getView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    // Two writes, one for PBR texture array, another for PBR factors
    std::array<VkWriteDescriptorSet, 2> aWriteDescriptorSets = {};
    {
        // Material sampler views
        VkWriteDescriptorSet &writeDescriptorSet = aWriteDescriptorSets[0];
        writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writeDescriptorSet.pImageInfo = imageInfos.data();
    }

    // Material parameters
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = materialParameters.m_pFactors->buffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Material::PBRFactors);
    {

        VkWriteDescriptorSet &writeDescriptorSet = aWriteDescriptorSets[1];
        writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = 1;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &bufferInfo;
    }

    vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), static_cast<uint32_t>(aWriteDescriptorSets.size()), aWriteDescriptorSets.data(),
                           0, nullptr);
}

VkDescriptorSet DescriptorManager::AllocateLightingDescriptorSet(
    const UniformBuffer<PerViewData>& perViewData, VkImageView position,
    VkImageView albedo, VkImageView normal, VkImageView uv)
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts =
        &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_LIGHTING];

    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    // Prepare buffer descriptor
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = perViewData.buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(PerViewData);

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // prepare image descriptor
        std::array<VkDescriptorImageInfo, 4> imageInfos = {
            // position
            GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
            position,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            // albedo
            GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
            albedo,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            // normal
            GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
            normal,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            // UV
            GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
            uv,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount =
            static_cast<uint32_t>(imageInfos.size());
        descriptorWrites[1].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(),
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
    return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocateGBufferDescriptorSet()
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_GBUFFER];

    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "GBuffer");

    return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocateGBufferDescriptorSet(
    const RenderPassGBuffer::GBufferViews& gbufferViews)
{
    VkDescriptorSet descriptorSet = AllocateGBufferDescriptorSet();
    updateGBufferDescriptorSet(descriptorSet, gbufferViews);
    return descriptorSet;
}

void DescriptorManager::updateGBufferDescriptorSet(
    VkDescriptorSet descriptorSet,
    const RenderPassGBuffer::GBufferViews& gbufferViews)

{
    // prepare image descriptor
    std::array<VkDescriptorImageInfo, RenderPassGBuffer::LightingAttachments::GBUFFER_ATTACHMENTS_COUNT> imageInfos;
    std::array<VkWriteDescriptorSet, RenderPassGBuffer::LightingAttachments::GBUFFER_ATTACHMENTS_COUNT> writeDescriptorSets;

    for (uint32_t i = 0; i < RenderPassGBuffer::LightingAttachments::GBUFFER_ATTACHMENTS_COUNT; i++)
    {
        VkDescriptorImageInfo& imageInfo = imageInfos[i];
        imageInfo = {GetSamplerManager()->getSampler(SAMPLER_1_MIPS),
                     gbufferViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkWriteDescriptorSet& writeDescriptorSet = writeDescriptorSets[i];
        writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = i;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pImageInfo = &imageInfo;
    }
    vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

VkDescriptorSet DescriptorManager::AllocateSingleSamplerDescriptorSet(
    VkImageView textureView)
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts =
        &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_SINGLE_SAMPLER];

    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "Single Sampler");

    // Prepare buffer descriptor
    {
        // prepare image descriptor
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = GetSamplerManager()->getSampler(SAMPLER_1_MIPS);

        VkWriteDescriptorSet descriptorWrite = {};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), 1,
                               &descriptorWrite, 0, nullptr);
    }
    return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocateSingleStorageImageDescriptorSet(VkImageView imageView)
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_SIGNLE_STORAGE_IMAGE];

    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "Single storage image");

    // Prepare buffer descriptor
    {
        // prepare image descriptor
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = imageView;

        VkWriteDescriptorSet descriptorWrite = {};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), 1,
                               &descriptorWrite, 0, nullptr);
    }
    return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocatePerviewDataDescriptorSet(
    const UniformBuffer<PerViewData>& perViewData, bool bIsRT)
{
    return AllocateUniformBufferDescriptorSet(perViewData, 0,
                                              m_aDescriptorSetLayouts[bIsRT ? DESCRIPTOR_LAYOUT_PER_VIEW_DATA_RT : DESCRIPTOR_LAYOUT_PER_VIEW_DATA]);
}

VkDescriptorSet DescriptorManager::AllocateIBLDescriptorSet()
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts =
        &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_IBL];
    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "IBL");

    return descriptorSet;
}
VkDescriptorSet DescriptorManager::AllocateIBLDescriptorSet(
    VkImageView irradianceMap,
    VkImageView prefilteredEnvMap,
    VkImageView specularBrdfLutMap)
{
    VkDescriptorSet descSet = AllocateIBLDescriptorSet();
    UpdateIBLDescriptorSet(descSet, irradianceMap, prefilteredEnvMap, specularBrdfLutMap);
    return descSet;
}

void DescriptorManager::UpdateIBLDescriptorSet(
    VkDescriptorSet& descriptorSet,
    VkImageView irradianceMap,
    VkImageView prefilteredEnvMap,
    VkImageView specularBrdfLutMap)
{
    // prepare image descriptor
    std::array<VkImageView, 3> aIBLViews = {irradianceMap, prefilteredEnvMap, specularBrdfLutMap};
    std::array<VkDescriptorImageInfo, 3> imageInfos;
    std::array<VkWriteDescriptorSet, 3> writeDescriptorSets;

    for (uint32_t i = 0; i < aIBLViews.size(); i++)
    {
        VkDescriptorImageInfo& imageInfo = imageInfos[i];
        imageInfo = {GetSamplerManager()->getSampler(SAMPLER_1_MIPS), aIBLViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkWriteDescriptorSet& writeDescriptorSet = writeDescriptorSets[i];
        writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = i;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pImageInfo = &imageInfo;
    }
    vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

VkDescriptorSet DescriptorManager::AllocateRayTracingDescriptorSet(const VkAccelerationStructureKHR &acc, const VkImageView &outputImage)
{
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_aDescriptorSetLayouts[DESCRIPTOR_LAYOUT_RAY_TRACING];
    assert(vkAllocateDescriptorSets(GetRenderDevice()->GetDevice(), &allocInfo,
                                    &descriptorSet) == VK_SUCCESS);

    setDebugUtilsObjectName(reinterpret_cast<uint64_t>(descriptorSet),
                            VK_OBJECT_TYPE_DESCRIPTOR_SET, "Ray Tracing input output");
    UpdateRayTracingDescriptorSet(descriptorSet, acc, outputImage);
    return descriptorSet;
}

void DescriptorManager::UpdateRayTracingDescriptorSet(VkDescriptorSet descriptorSet, const VkAccelerationStructureKHR &acc, const VkImageView &outputImage)
{
    // Two descriptor writes
    VkWriteDescriptorSetAccelerationStructureKHR accDesc = {};
    accDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accDesc.accelerationStructureCount = 1;
    accDesc.pAccelerationStructures = &acc;

    VkDescriptorImageInfo imageInfo = {VK_NULL_HANDLE, outputImage, VK_IMAGE_LAYOUT_GENERAL};

    std::array<VkWriteDescriptorSet, 2> writes;
    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writes[0].pNext = &accDesc;


    writes[1] = {};
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].pNext = nullptr;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(GetRenderDevice()->GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

DescriptorManager* GetDescriptorManager() { return &descriptorManager; }

