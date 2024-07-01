#include "VulkanStructs.h"

#include "VulkanContext.h"

namespace VoxelEngine
{
  void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const vk::DescriptorType type,
                                           const vk::ShaderStageFlags shaderStage)
  {
    vk::DescriptorSetLayoutBinding newBind;
    newBind.binding = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType = type;
    newBind.stageFlags = shaderStage;

    bindings.emplace_back(newBind);
  }

  vk::DescriptorSetLayout DescriptorLayoutBuilder::Build(const vk::Device device,
                                                         const vk::DescriptorSetLayoutCreateFlags flags,
                                                         const void* pNext)
  {
    vk::DescriptorSetLayoutCreateInfo info;
    info.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    info.pNext = pNext;
    info.pBindings = bindings.data();
    info.bindingCount = bindings.size();
    info.flags = flags;

     vk::DescriptorSetLayout set = device.createDescriptorSetLayout(info);

    return set;
  }

  void DescriptorAllocator::InitPool(const vk::Device device, const uint32_t maxSets,
                                     const std::span<PoolSizeRatio>& poolRatios)
  {
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (auto& poolRatio : poolRatios) {
      poolSizes.push_back(vk::DescriptorPoolSize{
            poolRatio.type,
            uint32_t(poolRatio.ratio * maxSets)
        });
    }

    vk::DescriptorPoolCreateInfo info;
    info.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    info.maxSets = maxSets;
    info.poolSizeCount = (uint32_t)poolSizes.size();
    info.pPoolSizes = poolSizes.data();

    pool = device.createDescriptorPool(info);
  }

  void DescriptorAllocator::ClearDescriptors(const vk::Device device) const
  {
    device.resetDescriptorPool(pool);
  }

  void DescriptorAllocator::DestroyPool(const vk::Device device) const
  {
    device.destroyDescriptorPool(pool);
  }

  vk::DescriptorSet DescriptorAllocator::Allocate(const vk::Device device,
                                                  const vk::DescriptorSetLayout layout) const
  {
    vk::DescriptorSetAllocateInfo info;
    info.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    vk::DescriptorSet set = device.allocateDescriptorSets(info).front();

    return set;
  }

  void PipelineBuilder::Clear()
  {
    inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;

    rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;

    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;

    depthStencil.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;

    renderInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;

    shaderStages.clear();
  }

  void PipelineBuilder::BuildPipeline(const vk::Device device, vk::PipelineLayout& layout,
                                      vk::Pipeline& pipeline,
                                      const vk::DescriptorSetLayout descriptorSetLayout)
  {
    vk::PipelineLayoutCreateInfo info;
    info.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    info.flags = vk::PipelineLayoutCreateFlags();
    info.setLayoutCount = 1;
    info.pSetLayouts = &descriptorSetLayout;
    info.pushConstantRangeCount = pushConstantRanges.size();
    info.pPushConstantRanges = pushConstantRanges.data();
    
    layout = device.createPipelineLayout(info);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputInfo.vertexBindingDescriptionCount = vertexInputBindings.size();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputStates.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputStates.data();

    vk::DynamicState state[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicInfo;
    dynamicInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicInfo.pDynamicStates = state;
    dynamicInfo.dynamicStateCount = 2;

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
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

    auto result = device.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);

    for (auto& stage : shaderStages) {
      device.destroyShaderModule(stage.module);
    }
    
    VE_ASSERT(result == vk::Result::eSuccess, "Failed to create pipelines!");
  }

  void PipelineBuilder::SetInputTopology(const vk::PrimitiveTopology topology)
  {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
  }

  void PipelineBuilder::SetPolygonMode(const vk::PolygonMode mode)
  {
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;
  }

  void PipelineBuilder::SetCullMode(const vk::CullModeFlags cullMode, const vk::FrontFace frontFace)
  {
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
  }

  void PipelineBuilder::SetMultisamplingNone()
  {
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
  }

  void PipelineBuilder::DisableBlending()
  {
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;
  }

  void PipelineBuilder::SetColorAttachmentFormat(const vk::Format format)
  {
    colorAttachmentFormat = format;

    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;
  }

  void PipelineBuilder::SetDepthFormat(const vk::Format format)
  {
    renderInfo.depthAttachmentFormat = format;
  }

  void PipelineBuilder::DisableDepthTest()
  {
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = vk::CompareOp::eNever;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
  }

  void PipelineBuilder::EnableDepthTest()
  {
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = vk::CompareOp::eNever;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
  }

  void ComputeEffect::Init()
  {
    VulkanContext& context = VulkanContext::Get();

    pipelineBuilder.Clear();
    pipelineBuilder.SetInputTopology(vk::PrimitiveTopology::eTriangleList);
    pipelineBuilder.SetPolygonMode(vk::PolygonMode::eFill);
    pipelineBuilder.SetCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise);
    pipelineBuilder.SetMultisamplingNone();
    pipelineBuilder.DisableBlending(); //temp
    pipelineBuilder.SetColorAttachmentFormat(context.GetDrawImage().imageFormat);
    pipelineBuilder.SetDepthFormat(context.GetDepthImage().imageFormat);
  }

  void ComputeEffect::Build()
  {
    vk::Device& device = VulkanContext::Get().GetDevice();

    descriptorAllocator.Init(device, 10);

    descriptorSetLayout = descriptorLayoutBuilder.Build(device, vk::DescriptorSetLayoutCreateFlagBits());

    descriptorSet = descriptorAllocator.Allocate(device, descriptorSetLayout);

    descriptorWriter.UpdateSet(device, descriptorSet);

    pipelineBuilder.BuildPipeline(device, pipelineLayout, pipeline, descriptorSetLayout);
  }

  void ComputeEffect::Destroy()
  {
    VulkanContext& context = VulkanContext::Get();

    context.GetDevice().destroyPipelineLayout(pipelineLayout);

    context.GetDevice().destroyPipeline(pipeline);

    descriptorAllocator.DestroyPools(context.GetDevice());

    context.GetDevice().destroyDescriptorSetLayout(descriptorSetLayout);
  }

  void DescriptorAllocatorGrowable::Init(const vk::Device device, const uint32_t maxSets)
  {
    vk::DescriptorPool newPool = CreatePool(device, maxSets);

    setsPerPool = maxSets * 1.5;

    readyPools.emplace_back(newPool);
  }

  void DescriptorAllocatorGrowable::AddRatios(PoolSizeRatio poolRatios)
  {
    ratios.emplace_back(poolRatios);
  }

  void DescriptorAllocatorGrowable::ClearPools(const vk::Device device)
  {
    for (auto& p : readyPools) {
      device.resetDescriptorPool(p);
    }

    for (auto& p : fullPools) {
      device.resetDescriptorPool(p);
      readyPools.push_back(p);
    }

    fullPools.clear();
  }

  void DescriptorAllocatorGrowable::DestroyPools(const vk::Device device)
  {
    for (auto& p : readyPools) {
      device.destroyDescriptorPool(p);
    }

    readyPools.clear();

    for (auto& p : fullPools) {
      device.destroyDescriptorPool(p);
    }
    
    fullPools.clear();
  }

  vk::DescriptorSet DescriptorAllocatorGrowable::Allocate(const vk::Device device, const vk::DescriptorSetLayout layout,
                                                          const void* pNext)
  {
    vk::DescriptorPool poolToUse = GetPool(device);

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.pNext = pNext;
    allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    vk::DescriptorSet set;
    auto result = device.allocateDescriptorSets(&allocInfo, &set);

    if (result == vk::Result::eErrorOutOfPoolMemory || result == vk::Result::eErrorFragmentedPool) {

      fullPools.push_back(poolToUse);

      poolToUse = GetPool(device);
      allocInfo.descriptorPool = poolToUse;

      result = device.allocateDescriptorSets(&allocInfo, &set);
      VE_CORE_ASSERT(result == vk::Result::eSuccess, "Failed to allocate descriptor set!");
    }

    readyPools.emplace_back(poolToUse);

    return set;
  }

  vk::DescriptorPool DescriptorAllocatorGrowable::GetPool(const vk::Device device)
  {
    vk::DescriptorPool newPool;
    if (readyPools.size() != 0) {
      newPool = readyPools.back();
      readyPools.pop_back();
    } else {
      newPool = CreatePool(device, setsPerPool);

      setsPerPool *= 1.5;
      if (setsPerPool > 4092) { setsPerPool = 4092; }
    }

    return newPool;
  }

  vk::DescriptorPool DescriptorAllocatorGrowable::CreatePool(const vk::Device device, const uint32_t setCount)
  {
    std::vector<vk::DescriptorPoolSize> poolSizes;

    for (const auto& ratio : ratios) {
      vk::DescriptorPoolSize poolSize;
      poolSize.type = ratio.type;
      poolSize.descriptorCount = uint32_t(ratio.ratio * setCount);
      poolSizes.emplace_back(poolSize);
    }

    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.maxSets = setCount;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    vk::DescriptorPool newPool;
    newPool = device.createDescriptorPool(pool_info);

    return newPool;
  }
  void DescriptorWriter::WriteImage(const uint32_t binding, const vk::ImageView image, const vk::Sampler sampler,
                                    const vk::ImageLayout layout, const vk::DescriptorType type)
  {
    vk::DescriptorImageInfo& info = imageInfos.emplace_back(
      vk::DescriptorImageInfo(sampler, image, layout)
    );

    vk::WriteDescriptorSet write;
    write.sType = vk::StructureType::eWriteDescriptorSet;
    write.dstBinding = binding;
    write.dstSet = nullptr;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.emplace_back(write);
  }

  void DescriptorWriter::WriteBuffer(const uint32_t binding, const vk::Buffer buffer, const uint64_t size,
                                     const uint64_t offset, const vk::DescriptorType type)
  {
    vk::DescriptorBufferInfo& info = bufferInfos.emplace_back(
      vk::DescriptorBufferInfo(buffer, offset, size)
    );

    vk::WriteDescriptorSet write;
    write.sType = vk::StructureType::eWriteDescriptorSet;
    write.dstBinding = binding;
    write.dstSet = nullptr;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.emplace_back(write);
  }

  void DescriptorWriter::Clear()
  {
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
  }

  void DescriptorWriter::UpdateSet(const vk::Device device, const vk::DescriptorSet set)
  {
    for (auto& write : writes) {
      write.dstSet = set;
    }

    device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
  }
}