#include "ImGuiLayer.h"

#include "HeliosEngine/Application.h"

#include "Events/Event.h"

namespace Helios {
	void ImGuiLayer::OnAttach() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

		float fontSize = 16.0f;
		io.Fonts->AddFontFromFileTTF(std::format("{}Assets/Fonts/DroidSans.ttf", HELIOSENGINE_DIR).c_str(), fontSize);
		io.FontDefault = io.Fonts->AddFontFromFileTTF(std::format("{}Assets/Fonts/Cousine-Regular.ttf", HELIOSENGINE_DIR).c_str(), fontSize);
		
		ImGui::StyleColorsDark();
		
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 0.75f;
		}

		Application::Get().GetWindow().InitImGui();
	}

	void ImGuiLayer::OnDetach() {
		Application::Get().GetWindow().ShutdownImGui();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& event) {
		if (m_BlockEvents) {
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}
}