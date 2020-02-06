#include "PipelineStateBuilder.h"
#include <array>
#include <cassert>
PipelineStateBuilder& PipelineStateBuilder::setShaderModules(
    const std::vector<VkShaderModule>& shaderModules)
{
    assert(shaderModules.size() == 2);
    mShaderStagesInfo.reserve(shaderModules.size());

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderModules[0];
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModules[1];
    fragShaderStageInfo.pName = "main";

    mShaderStagesInfo = {vertShaderStageInfo, fragShaderStageInfo};
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::setVertextInfo(
    const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>& attribDescriptions)
{
    mVertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mVertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
    mVertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    mVertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribDescriptions.size());
    mVertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();

    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::setViewport(
    const VkViewport& viewport, const VkRect2D& scissor)
{
    mViewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    mViewportState.viewportCount = 1;
    mViewportState.pViewports = &viewport;
    mViewportState.scissorCount = 1;
    mViewportState.pScissors = &scissor;

    return *this;
}

VkPipeline PipelineStateBuilder::build(VkDevice device)
{
    VkPipeline res = VK_NULL_HANDLE;
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = mShaderStagesInfo.size();
    pipelineInfo.pStages = mShaderStagesInfo.data();

    pipelineInfo.pVertexInputState = &mVertexInputInfo;

    pipelineInfo.pInputAssemblyState = &mInputAssemblyInfo;

    pipelineInfo.pViewportState = &mViewportState;

    pipelineInfo.pRasterizationState = &mRasterizerInfo;

    pipelineInfo.pMultisampleState = &mMultisamplingInfo;

    pipelineInfo.pDepthStencilState = &mDepthStencilInfo;

    pipelineInfo.pColorBlendState = &mColorBlendStateInfo;

    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = mPipelineLayout;

    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                     nullptr,
                                     &res) == VK_SUCCESS);
    return res;
}