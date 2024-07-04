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
		VulkanVertexBuffer(const std::string& name, const uint32_t size);
		VulkanVertexBuffer(const std::string& name, const float* vertices, const uint32_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, const uint32_t size) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		const std::string& GetName() const { return m_Name; }

		std::vector<float>& GetVertices() { return m_Vertices; }

		AllocatedBuffer& GetBuffer() { return m_Buffer; }
		AllocatedBuffer& GetStagingBuffer() { return m_StagingBuffer; }

	private:
		std::string m_Name;
		BufferLayout m_Layout;

    std::vector<float> m_Vertices;

		AllocatedBuffer m_Buffer;
		AllocatedBuffer m_StagingBuffer;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const std::string& name, const uint32_t* indices, const uint32_t count);
		VulkanIndexBuffer(const std::string& name, const uint32_t size);
		virtual ~VulkanIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual uint32_t GetCount() const { return m_Count; }

		virtual void SetData(const void* data, const uint32_t size);

		const std::string& GetName() const { return m_Name; }

		const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

		AllocatedBuffer& GetBuffer() { return m_Buffer; }
		AllocatedBuffer& GetStagingBuffer() { return m_StagingBuffer; }

	private:
		std::string m_Name;
		uint32_t m_Count = 0;

		std::vector<uint32_t> m_Indices;

		AllocatedBuffer m_Buffer;
		AllocatedBuffer m_StagingBuffer;
	};
}