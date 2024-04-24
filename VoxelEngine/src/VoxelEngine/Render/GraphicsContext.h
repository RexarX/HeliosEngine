#pragma once

namespace VoxelEngine
{
	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void ClearBuffer() = 0;
		virtual void SetViewport(const uint32_t width, const uint32_t height) = 0;

		static std::unique_ptr<GraphicsContext> Create(void* window);
	};
}