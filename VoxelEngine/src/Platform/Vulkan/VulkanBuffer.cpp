#include "VulkanBuffer.h"

#include "vepch.h"

namespace VoxelEngine
{
	VulkanVertexBuffer::VulkanVertexBuffer(const uint32_t size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const float* vertices, const uint32_t size)
	{
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
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t size)
	{
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
