#pragma once

#include "VoxelEngine/Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Engine
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