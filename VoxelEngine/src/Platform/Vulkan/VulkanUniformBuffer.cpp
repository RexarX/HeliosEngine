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
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging uniform buffer!");

		VulkanContext::Get().GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
	}

	void VulkanUniformBuffer::SetData(const void* data, const uint32_t size, const uint32_t offset)
	{
		memcpy(m_Buffer.info.pMappedData, data, size);

		m_Data = data;
		m_Size = size;
		m_Offset = offset;
	}
}