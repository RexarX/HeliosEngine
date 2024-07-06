#pragma once

#include "Render/Buffer.h"

namespace VoxelEngine
{
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(const std::string& name, const uint64_t size);
		OpenGLVertexBuffer(const std::string& name, const float* vertices, const uint64_t size);
		virtual ~OpenGLVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, const uint64_t size) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

	private:
		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(const std::string& name, const uint32_t* indices, const uint64_t count);
		OpenGLIndexBuffer(const std::string& name, const uint64_t size);
		virtual ~OpenGLIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual uint64_t GetCount() const { return m_Count; }

		virtual void SetData(const void* data, const uint64_t size);

	private:
		uint32_t m_RendererID;
		uint64_t m_Count;
	};
}