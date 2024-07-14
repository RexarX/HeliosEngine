#include "GameLayer.h"

GameLayer::GameLayer()
	: Layer("VoxelCraft")
{
}

void GameLayer::OnAttach()
{
	//Engine::Window& window = Engine::Application::Get().GetWindow();
	//window.SetVSync(false);
	//window.SetFramerate(60.0);
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(const Engine::Timestep ts)
{
}

void GameLayer::OnEvent(Engine::Event& event)
{
}

void GameLayer::OnImGuiRender(ImGuiContext* context)
{
	Engine::Timestep ts = Engine::Application::Get().GetDeltaTime();

	ImGui::SetCurrentContext(context);

	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %f", ts.GetFramerate());
	ImGui::Text("Frametime: %f", ts.GetMilliseconds());
	ImGui::End();
}