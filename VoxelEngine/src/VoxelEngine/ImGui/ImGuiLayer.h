#pragma once

#include "VoxelEngine/Layer.h"

#include "VoxelEngine/Events/ApplicationEvent.h"
#include "VoxelEngine/Events/KeyEvent.h"
#include "VoxelEngine/Events/MouseEvent.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace VoxelEngine
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();
		void BlockEvents(const bool block) noexcept { m_BlockEvents = block; }

		uint32_t GetActiveWidgetID() const noexcept;

	private:
		bool m_BlockEvents = true;
	};
}