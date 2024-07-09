#pragma once

#include "Render/Buffer.h"

#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.h>

struct AllocatedBuffer
{
	VkBuffer buffer;

	VmaAllocation allocation;
	VmaAllocationInfo info;
};

namespace VoxelEngine
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const uint64_t size);
		VulkanVertexBuffer(const float* vertices, const uint64_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, const uint64_t size) override;

		virtual inline const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		inline const std::vector<float>& GetVertices() const { return m_Vertices; }

		inline AllocatedBuffer& GetBuffer() { return m_Buffer; }
		inline AllocatedBuffer& GetStagingBuffer() { return m_StagingBuffer; }

	private:
		BufferLayout m_Layout;

    std::vector<float> m_Vertices;

		AllocatedBuffer m_Buffer;
		AllocatedBuffer m_StagingBuffer;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const uint32_t* indices, const uint32_t count);
		VulkanIndexBuffer(const uint64_t size);
		virtual ~VulkanIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual inline const uint32_t GetCount() const { return m_Count; }

		virtual void SetData(const void* data, const uint64_t size);

		const inline std::vector<uint32_t>& GetIndices() const { return m_Indices; }

		inline AllocatedBuffer& GetBuffer() { return m_Buffer; }
		inline AllocatedBuffer& GetStagingBuffer() { return m_StagingBuffer; }

	private:
		uint32_t m_Count = 0;

		std::vector<uint32_t> m_Indices;

		AllocatedBuffer m_Buffer;
		AllocatedBuffer m_StagingBuffer;
	};
}