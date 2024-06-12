#include "VulkanBuffer.h"

#include "VulkanContext.h"

#include "vepch.h"

namespace VoxelEngine
{
	VulkanVertexBuffer::VulkanVertexBuffer(const uint32_t size)
	{
		m_Vertices.reserve(size / sizeof(float));

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging vertex buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const float* vertices, const uint32_t size)
	{
    m_Vertices.reserve(size / sizeof(float));

		if (vertices != nullptr) {
			m_Vertices.insert(m_Vertices.begin(), vertices, vertices + size / sizeof(float));
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging vertex buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
	}

	void VulkanVertexBuffer::Bind() const
	{
	}

	void VulkanVertexBuffer::Unbind() const
	{
	}

	void VulkanVertexBuffer::SetData(const void* data, const uint32_t size)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t* indices, const uint32_t count)
		: m_Count(count)
	{
		m_Indices.reserve(count);
		if (indices != nullptr) {
			m_Indices.insert(m_Indices.begin(), indices, indices + count);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = count * sizeof(uint32_t);
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging index buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t size)
	{
		m_Indices.reserve(size / sizeof(uint32_t));

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging index buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(VulkanContext::Get().GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    VulkanContext::Get().GetDeletionQueue().push_function([&]() {
      vmaDestroyBuffer(VulkanContext::Get().GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
	}

	void VulkanIndexBuffer::Bind() const
	{
	}

	void VulkanIndexBuffer::Unbind() const
	{
	}

	void VulkanIndexBuffer::SetData(const void* data, const uint32_t size)
	{
	}
}
