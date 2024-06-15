#include "VulkanUniformBuffer.h"

#include "VulkanContext.h"

namespace VoxelEngine
{
	VulkanUniformBuffer::VulkanUniformBuffer(const char* name, const uint32_t size, const uint32_t binding)
    : m_Name(name), m_Size(size)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VulkanContext::Get().GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging uniform buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

		VulkanContext::Get().GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create uniform buffer!");
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
	}

	void VulkanUniformBuffer::SetData(const void* data, const uint32_t size, const uint32_t offset)
	{
		void* dst_data = m_StagingBuffer.info.pMappedData;

		memcpy(dst_data, data, size);

		VulkanContext::Get().ImmediateSubmit([&](const vk::CommandBuffer cmd) {
			vk::BufferCopy uniformCopy;
			uniformCopy.dstOffset = offset;
			uniformCopy.srcOffset = 0;
			uniformCopy.size = size;

			cmd.copyBuffer(m_StagingBuffer.buffer, m_Buffer.buffer, 1, &uniformCopy);
			});

		m_Data = data;
		m_Size = size;
		m_Offset = offset;
	}
}