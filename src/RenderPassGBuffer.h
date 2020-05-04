#pragma once
#include "RenderPass.h"
#include <array>

class RenderPassGBuffer : public RenderPass
{
public:
    struct GBufferAttachments
    {
        enum GBufferAttachmentNames
        {
            GBUFFER_POSITION,
            GBUFFER_ALBEDO,
            GBUFFER_NORMAL,
            GBUFFER_UV,
            GBUFFER_DEPTH,
            COLOR_ATTACHMENTS_COUNT = GBUFFER_DEPTH,
            ATTACHMENTS_COUNT
        };
        const std::array<std::string, ATTACHMENTS_COUNT> aNames = {
            "GBUFFER_POSITION",
            "GBUFFER_ALBEDO",
            "GBUFFER_NORMAL",
            "GBUFFER_UV",
            "GBUFFER_DEPTH",
        };

        const std::array<VkFormat, ATTACHMENTS_COUNT> aFormats = {
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_D32_SFLOAT
        };
        std::array<VkImageView, ATTACHMENTS_COUNT> aViews;

        std::array<VkAttachmentDescription, ATTACHMENTS_COUNT> aAttachmentDesc;
        GBufferAttachments()
        {
            // Construct color attachment descriptions
            VkAttachmentDescription desc = {};
            desc.samples = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            aAttachmentDesc[GBUFFER_POSITION] = desc;
            aAttachmentDesc[GBUFFER_POSITION].format = VK_FORMAT_R16G16B16A16_SFLOAT;

            aAttachmentDesc[GBUFFER_ALBEDO] = desc;
            aAttachmentDesc[GBUFFER_ALBEDO].format = VK_FORMAT_R16G16B16A16_SFLOAT;

            aAttachmentDesc[GBUFFER_UV] = desc;
            aAttachmentDesc[GBUFFER_UV].format = VK_FORMAT_R16G16B16A16_SFLOAT;

            aAttachmentDesc[GBUFFER_NORMAL] = desc;
            aAttachmentDesc[GBUFFER_NORMAL].format = VK_FORMAT_R16G16B16A16_SFLOAT;

            aAttachmentDesc[GBUFFER_DEPTH] = desc;
            aAttachmentDesc[GBUFFER_DEPTH].format = VK_FORMAT_D32_SFLOAT;
            aAttachmentDesc[GBUFFER_DEPTH].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            aAttachmentDesc[GBUFFER_DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            aAttachmentDesc[GBUFFER_DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    };
    RenderPassGBuffer();
    ~RenderPassGBuffer();
    void recordCommandBuffer(VkBuffer vertexBuffer, VkBuffer indexBuffer,
                uint32_t numIndices,
                VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                VkDescriptorSet descriptorSet);
    void createFramebuffer();
    void destroyFramebuffer();
    void setGBufferImageViews(VkImageView positionView, VkImageView albedoView,
                              VkImageView normalView, VkImageView uvView,
                              VkImageView depthView, uint32_t nWidth,
                              uint32_t nHeight);
    void createGBufferViews(VkExtent2D size);
    void removeGBufferViews();
    VkCommandBuffer GetCommandBuffer()
    {
        return m_commandBuffer;
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    GBufferAttachments m_attachments;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkExtent2D mRenderArea = {0, 0};
};
