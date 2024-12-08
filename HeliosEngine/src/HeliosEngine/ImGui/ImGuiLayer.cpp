#include "ImGuiLayer.h"

#include "HeliosEngine/Application.h"

#include "Events/Event.h"

namespace Helios {
	void ImGuiLayer::OnAttach() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

		float fontSize = 16.0f;

		const char* file = "Assets/Fonts/Cousine-Regular.ttf";
		CORE_ASSERT(std::filesystem::exists(file), "'{}' does not exists!", file);
		io.Fonts->AddFontFromFileTTF(file, fontSize);
		
		file = "Assets/Fonts/DroidSans.ttf";
		CORE_ASSERT(std::filesystem::exists(file), "'{}' does not exists!", file);
		io.FontDefault = io.Fonts->AddFontFromFileTTF(file, fontSize);
		
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
		if (m_BlockEvents) { return; }
	}
}