#include "VulkanBuffer.h"

#include "VulkanContext.h"

namespace VoxelEngine
{
	VulkanVertexBuffer::VulkanVertexBuffer(const char* name, const uint32_t size)
    : m_Name(name)
	{
		VulkanContext& context = VulkanContext::Get();

		context.SetCurrentComputeEffect(m_Name);

		m_Vertices.reserve(size / sizeof(float));

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);
		
		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging vertex buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    context.GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(context.GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
      vmaDestroyBuffer(context.GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const char* name, const float* vertices, const uint32_t size)
    : m_Name(name)
	{
		VulkanContext& context = VulkanContext::Get();

		context.SetCurrentComputeEffect(m_Name);

    m_Vertices.reserve(size / sizeof(float));

		if (vertices != nullptr) {
			m_Vertices.insert(m_Vertices.begin(), vertices, vertices + size / sizeof(float));
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging vertex buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    context.GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(context.GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
      vmaDestroyBuffer(context.GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
	}

	void VulkanVertexBuffer::Bind() const
	{
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);
	}

	void VulkanVertexBuffer::Unbind() const
	{
	}

	void VulkanVertexBuffer::SetData(const void* data, const uint32_t size)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const char* name, const uint32_t* indices, const uint32_t count)
		: m_Name(name), m_Count(count)
	{
		VulkanContext& context = VulkanContext::Get();

		context.SetCurrentComputeEffect(m_Name);

		m_Indices.reserve(count);
		if (indices != nullptr) {
			m_Indices.insert(m_Indices.begin(), indices, indices + count);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = count * sizeof(uint32_t);
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging index buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    context.GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(context.GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
      vmaDestroyBuffer(context.GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const char* name, const uint32_t size)
    : m_Name(name)
	{
		VulkanContext& context = VulkanContext::Get();

		context.SetCurrentComputeEffect(m_Name);

		m_Indices.reserve(size / sizeof(uint32_t));

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
																	&m_StagingBuffer.buffer, &m_StagingBuffer.allocation, &m_StagingBuffer.info);

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging index buffer!");

		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(context.GetAllocator(), &bufferInfo, &vmaallocInfo,
														 &m_Buffer.buffer, &m_Buffer.allocation, &m_Buffer.info);

    context.GetDeletionQueue().push_function([&]() {
			vmaDestroyBuffer(context.GetAllocator(), m_StagingBuffer.buffer, m_StagingBuffer.allocation);
      vmaDestroyBuffer(context.GetAllocator(), m_Buffer.buffer, m_Buffer.allocation);
			});

		VE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
	}

	void VulkanIndexBuffer::Bind() const
	{
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);
	}

	void VulkanIndexBuffer::Unbind() const
	{
	}

	void VulkanIndexBuffer::SetData(const void* data, const uint32_t size)
	{
	}
}
