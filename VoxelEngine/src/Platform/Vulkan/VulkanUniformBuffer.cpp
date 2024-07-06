#include "VulkanUniformBuffer.h"
#include "VulkanContext.h"

namespace VoxelEngine
{
	VulkanUniformBuffer::VulkanUniformBuffer(const std::string& name, const uint32_t size, const uint32_t binding)
    : m_Name(name), m_Size(size)
	{
    VulkanContext& context = VulkanContext::Get();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

		CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging uniform buffer!");

		context.GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(context.GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
	}

	void VulkanUniformBuffer::SetData(const void* data, const uint32_t size, const uint32_t offset)
	{
		memcpy((int8_t*)m_Buffer.info.pMappedData + offset, data, size);

		m_Size = size;
	}
}