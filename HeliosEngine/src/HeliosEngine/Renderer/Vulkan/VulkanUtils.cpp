#pragma once

#include "VulkanUtils.h"
#include <span>

namespace Helios
{
  PipelineBuilder::PipelineBuilder()
  {
    Clear();
  }

  void PipelineBuilder::Clear()
  {
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    shaderInfos.clear();
  }

  void PipelineBuilder::BuildPipeline(const VkDevice device, VkPipelineLayout& layout, VkPipeline& pipeline,
                                      const VkRenderPass renderPass, const VkDescriptorSetLayout* descriptorSetLayouts,
                                      uint32_t descriptorSetLayoutCount)
  {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.flags = 0;
    info.setLayoutCount = descriptorSetLayoutCount;
    info.pSetLayouts = descriptorSetLayouts;
    info.pushConstantRangeCount = 1;
    info.pPushConstantRanges = &pushConstantRange;

    auto result = vkCreatePipelineLayout(device, &info, nullptr, &layout);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create pipeline layout!");

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = vertexInputBindings.size();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputStates.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputStates.data();

    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.pDynamicStates = state;
    dynamicInfo.dynamicStateCount = 2;

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderInfos.size());
    for (auto& shaderInfo : shaderInfos) {
      shaderStageInfo.stage = shaderInfo.stage;
      shaderStageInfo.module = shaderInfo.shaderModule;
      shaderStages.push_back(shaderStageInfo);
    } 

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = &renderInfo;
    graphicsPipelineCreateInfo.pStages = shaderStages.data();
    graphicsPipelineCreateInfo.stageCount = shaderStages.size();
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicInfo;
    graphicsPipelineCreateInfo.layout = layout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create pipeline!");
  }

  void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
  {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
  }

  void PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
  {
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;
  }

  void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
  {
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
  }

  void PipelineBuilder::SetMultisamplingNone()
  {
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
  }

  void PipelineBuilder::DisableBlending()
  {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;
  }

  void PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
  {
    colorAttachmentFormat = format;

    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;
  }

  void PipelineBuilder::SetDepthFormat(VkFormat format)
  {
    renderInfo.depthAttachmentFormat = format;
  }

  void PipelineBuilder::DisableDepthTest()
  {
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
  }

  void PipelineBuilder::EnableDepthTest()
  {
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
  }
}