#pragma once

#include "Render/Buffer.h"

namespace VoxelEngine
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const uint32_t size);
		VulkanVertexBuffer(const float* vertices, const uint32_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, const uint32_t size) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

	private:
		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const uint32_t* indices, const uint32_t count);
		VulkanIndexBuffer(const uint32_t size);
		virtual ~VulkanIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual uint32_t GetCount() const { return m_Count; }

		virtual void SetData(const void* data, const uint32_t size);

	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};
}