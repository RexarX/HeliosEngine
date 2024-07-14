#include "ImGuiLayer.h"

#include "VoxelEngine/Application.h"

namespace Engine
{
  ImGuiLayer::ImGuiLayer()
    : Layer("ImGuiLayer")
  {
  }

	void ImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

		float fontSize = 16.0f;
		io.Fonts->AddFontFromFileTTF((VOXELENGINE_DIR + "Assets/Fonts/DroidSans.ttf").c_str(), fontSize);
		io.FontDefault = io.Fonts->AddFontFromFileTTF((VOXELENGINE_DIR + "Assets/Fonts/Cousine-Regular.ttf").c_str(), fontSize);
		
		ImGui::StyleColorsDark();
		
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 0.75f;
		}

		Application::Get().GetWindow().InitImGui();
	}

	void ImGuiLayer::OnDetach()
	{
		Application::Get().GetWindow().ShutdownImGui();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents) {
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		Application::Get().GetWindow().Begin();

		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
																 ImGuiDockNodeFlags_PassthruCentralNode);
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::GetIO().DisplaySize = ImVec2((float)Application::Get().GetWindow().GetWidth(),
																				(float)Application::Get().GetWindow().GetHeight());

		Application::Get().GetWindow().End();
	}

	uint32_t ImGuiLayer::GetActiveWidgetID() const noexcept
	{
    return ImGui::GetActiveID();
	}
}