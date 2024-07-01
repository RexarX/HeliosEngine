#pragma once

#include "Render/UniformBuffer.h"

namespace VoxelEngine
{
	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(const char* name, const uint32_t size, const uint32_t binding);
		virtual ~OpenGLUniformBuffer();

		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset) override;

	private:
		uint32_t m_RendererID = 0;

		uint32_t m_Binding = 0;
	};
}