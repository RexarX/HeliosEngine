#pragma once

#include "Renderer/UniformBuffer.h"

namespace VoxelEngine
{
	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(const uint32_t size, const uint32_t binding);
		virtual ~OpenGLUniformBuffer();

		virtual void SetData(const void* data, const uint32_t size, const uint32_t offset = 0) override;

	private:
		uint32_t m_RendererID = 0;
	};
}