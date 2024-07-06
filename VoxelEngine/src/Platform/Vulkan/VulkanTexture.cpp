#include "VulkanTexture.h"

#include "vepch.h"

#include <stb_image/stb_image.h>

namespace VoxelEngine
{
	bool VulkanTexture::m_GenerateMipmaps = false;
	float VulkanTexture::m_AnisoLevel = 0.0f;

	VulkanTexture::VulkanTexture(const std::string& name, const TextureSpecification& specification)
		: m_Name(name), m_Specification(specification), m_Width(m_Specification.Width),
			m_Height(m_Specification.Height)
	{
		if (!m_IsLoaded) { CORE_ERROR("Failed to load texture!"); return; }
	}

	VulkanTexture::VulkanTexture(const std::string& name, const std::string& path)
		: m_Name(name), m_Path(path)
	{
		VulkanContext& context = VulkanContext::Get();
		ComputeEffect& effect = context.GetComputeEffect(name);

		int32_t width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* imData = nullptr;
		imData = stbi_load(path.c_str(), &width, &height, &channels, 4);

		if (imData == nullptr) { CORE_ERROR("Failed to load texture!"); }
		else { m_IsLoaded = true; }

		m_Width = width;
		m_Height = height;

		if (channels == 4) {
			m_DataFormat = vk::Format::eR8G8B8A8Srgb;
		} else if (channels == 3) {
			m_DataFormat = vk::Format::eR8G8B8Srgb;
		}

		AllocatedImage image = create_image(imData, vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
																				vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled);

		vk::SamplerCreateInfo sampl;
		sampl.sType = vk::StructureType::eSamplerCreateInfo;
		sampl.magFilter = vk::Filter::eNearest;
		sampl.minFilter = vk::Filter::eNearest;

		samplerNearest = context.GetDevice().createSampler(sampl);

		sampl.magFilter = vk::Filter::eLinear;
		sampl.minFilter = vk::Filter::eLinear;

		samplerLinear = context.GetDevice().createSampler(sampl);

		context.GetDeletionQueue().push_function([&]() {
			context.GetDevice().destroySampler(samplerNearest);
      context.GetDevice().destroySampler(samplerLinear);
			});

		effect.descriptorAllocator.AddRatios(
			{ vk::DescriptorType::eCombinedImageSampler, 1 }
		);

		effect.descriptorLayoutBuilder.AddBinding(effect.binding, vk::DescriptorType::eCombinedImageSampler,
																							vk::ShaderStageFlagBits::eFragment);

		effect.descriptorWriter.WriteImage(effect.binding, image.imageView, samplerLinear,
																			 vk::ImageLayout::eShaderReadOnlyOptimal,
																			 vk::DescriptorType::eCombinedImageSampler);

		++effect.binding;
	}

	VulkanTexture::VulkanTexture(const std::string& name, const std::vector<std::string>& paths)
	{
	}

	VulkanTexture::VulkanTexture(const std::string& name,
															 const std::string& right, const std::string& left, const std::string& top,
															 const std::string& bottom, const std::string& front, const std::string& back)
		: m_Name(name), m_Paths({ right, left, top, bottom, front, back })
	{
		if (!m_IsLoaded) { CORE_ERROR("Failed to load cubemap!"); }
	}

	VulkanTexture::~VulkanTexture()
	{
	}

	void VulkanTexture::SetData(const void* data, const uint32_t size)
	{
	}

	void VulkanTexture::Bind(const uint32_t slot) const
	{
	}

	AllocatedImage VulkanTexture::create_image(const vk::Extent3D size, const vk::Format format, const vk::ImageUsageFlags usage) const
	{
		VulkanContext& context = VulkanContext::Get();

		AllocatedImage newImage = {};
		newImage.imageFormat = format;
		newImage.imageExtent = size;

		VkImageCreateInfo img_info = {};
		img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		img_info.imageType = VK_IMAGE_TYPE_2D;
		img_info.format = static_cast<VkFormat>(format);
		img_info.extent = size;
		img_info.mipLevels = 1;
		img_info.arrayLayers = 1;
		img_info.samples = VK_SAMPLE_COUNT_1_BIT;
		img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		img_info.usage = static_cast<VkImageUsageFlags>(usage);

		if (m_GenerateMipmaps) {
			img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto result = vmaCreateImage(context.GetAllocator(), &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr);
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

		vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor;
		if (format == vk::Format::eD32Sfloat) {
			aspectFlag = vk::ImageAspectFlagBits::eDepth;
		}

		vk::ImageViewCreateInfo view_info;
		view_info.sType = vk::StructureType::eImageViewCreateInfo;
		view_info.viewType = vk::ImageViewType::e2D;
		view_info.image = newImage.image;
		view_info.format = format;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;
		view_info.subresourceRange.aspectMask = aspectFlag;
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		result = static_cast<VkResult>(context.GetDevice().createImageView(&view_info, nullptr, &newImage.imageView));
    CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");

		context.GetDeletionQueue().push_function([&]() {
      vmaDestroyImage(context.GetAllocator(), newImage.image, newImage.allocation);
			context.GetDevice().destroyImageView(newImage.imageView);
    });

		return newImage;
	}

	AllocatedImage VulkanTexture::create_image(const void* data, const vk::Extent3D size, const vk::Format format,
																						 const vk::ImageUsageFlags usage) const
	{
		VulkanContext& context = VulkanContext::Get();

		uint64_t data_size = static_cast<uint64_t>(size.depth) * static_cast<uint64_t>(size.width) *
												 static_cast<uint64_t>(size.height) * 4;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = data_size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		AllocatedBuffer uploadbuffer = {};
		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo, &uploadbuffer.buffer,
																	&uploadbuffer.allocation, &uploadbuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create buffer!");

		if (data != nullptr) { memcpy(uploadbuffer.info.pMappedData, data, data_size); }

		AllocatedImage new_image = create_image(size, format, usage | vk::ImageUsageFlagBits::eTransferDst |
																																	vk::ImageUsageFlagBits::eTransferSrc);

		context.ImmediateSubmit([&](const vk::CommandBuffer cmd) {
			vk::ImageSubresourceRange subImage;
			subImage.aspectMask = vk::ImageAspectFlagBits::eColor;
			subImage.baseMipLevel = 0;
			subImage.levelCount = VK_REMAINING_MIP_LEVELS;
			subImage.baseArrayLayer = 0;
			subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

			vk::ImageMemoryBarrier2 imageBarrier;
			imageBarrier.sType = vk::StructureType::eImageMemoryBarrier2;
			imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
			imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
			imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
			imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
			imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
			imageBarrier.subresourceRange = subImage;
			imageBarrier.image = new_image.image;

			vk::DependencyInfo depInfo;
			depInfo.sType = vk::StructureType::eDependencyInfo;
			depInfo.imageMemoryBarrierCount = 1;
			depInfo.pImageMemoryBarriers = &imageBarrier;

			cmd.pipelineBarrier2(depInfo);

			vk::BufferImageCopy copyRegion;
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

      cmd.copyBufferToImage(uploadbuffer.buffer, new_image.image, vk::ImageLayout::eTransferDstOptimal, 1,
                           &copyRegion);

			imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imageBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

			cmd.pipelineBarrier2(depInfo);
			});

		vmaDestroyBuffer(context.GetAllocator(), uploadbuffer.buffer, uploadbuffer.allocation);

		return new_image;
	}
}