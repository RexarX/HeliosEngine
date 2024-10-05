#pragma once

#include "HeliosEngine/Layer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Helios
{
	class Event;

	class ImGuiLayer final : public Layer
	{
	public:
		ImGuiLayer() : Layer("ImGuiLayer") {}
		~ImGuiLayer() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(const Event& e) override;

		void BlockEvents(bool block) noexcept { m_BlockEvents = block; }

		inline uint32_t GetActiveWidgetID() const noexcept { return ImGui::GetActiveID(); }

	private:
		bool m_BlockEvents = true;
	};
}