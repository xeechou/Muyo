#pragma once
#include <memory>

#include "Geometry.h"
#include "RenderPass.h"
#include "Texture.h"
#include "UniformBuffer.h"

class RenderLayerIBL : public RenderPass
{
    struct IBLPassResource
    {
    };
    // Generate convoluted irradiance map
public:
    RenderLayerIBL();
    ~RenderLayerIBL();
    void destroyFramebuffer();

    VkCommandBuffer GetCommandBuffer(size_t idx) const override
    {
        return m_commandBuffer;
    }

private:    // Methods
    void setupRenderPass();
    void setupFramebuffer();
    void setupPipeline();
    void setupDescriptorSets();
    void recordCommandBuffer();

private:
    const uint32_t ENV_CUBE_DIM = 128;
    const uint32_t IRR_CUBE_DIM = 64;
    const uint32_t NUM_FACES = 6;
    const uint32_t NUM_MIP = (uint32_t)std::floor(std::log2(ENV_CUBE_DIM))+1;
    const VkFormat TEX_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkExtent2D m_renderArea = {0, 0};
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    std::unique_ptr<Geometry> m_pSkybox = nullptr;

    Texture m_texEnvCupMap;

    VkPipeline m_envCubeMapPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_envCubeMapPipelineLayout = VK_NULL_HANDLE;

    VkPipeline m_irrCubeMapPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_irrCubeMapPipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSet m_perViewDataDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet m_envMapDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet m_irrMapDescriptorSet = VK_NULL_HANDLE;

    std::array<VkFramebuffer, 2> m_frameBuffers = {VK_NULL_HANDLE, VK_NULL_HANDLE} ;

    UniformBuffer<PerViewData> m_uniformBuffer;

    enum RenderPasses
    {
        RENDERPASS_LOAD_ENV_MAP = 0,
        RENDERPASS_COMPUTE_IRR_MAP,
        RENDERPASS_COUNT
    };
    std::array<VkRenderPass, RENDERPASS_COUNT> m_aRenderPasses = {
        VK_NULL_HANDLE, VK_NULL_HANDLE};

    std::array<VkFramebuffer, RENDERPASS_COUNT> m_aFramebuffers = {
        VK_NULL_HANDLE, VK_NULL_HANDLE};

    const std::array<uint32_t, RENDERPASS_COUNT> m_aCubemapSizes = {ENV_CUBE_DIM,
                                                              IRR_CUBE_DIM};

    const std::array<std::string, RENDERPASS_COUNT> m_aRenderPassNames = {
        "Load cubemap","Compute irradiance map"
    };
};

