#include "GameLayer.h"

#include "Scripts/Player/CameraController.h"

void GameLayer::OnAttach()
{
	Helios::Window& window = Helios::Application::Get().GetWindow();
	//window.SetVSync(true);
	//window.SetFramerate(30.0);

	Helios::Scene& gameScene = Helios::SceneManager::AddScene("GameScene");

	Helios::Entity& root = gameScene.GetRootEntity();

	Helios::Entity camera = gameScene.CreateEntity("Camera");
	camera.EmplaceComponent<Helios::Camera>();
	camera.EmplaceScriptComponent<CameraController>();

	root.AddChild(camera);

	gameScene.SetActive(true);
	gameScene.Load();
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Helios::Timestep ts)
{
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnUpdate(ts);
}

void GameLayer::OnEvent(Helios::Event& event)
{
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnEvent(event);
}

void GameLayer::Draw()
{
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.Draw();
}

void GameLayer::OnImGuiRender(ImGuiContext* context)
{
	Helios::Timestep ts = Helios::Application::Get().GetDeltaTime();

	ImGui::SetCurrentContext(context);

	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %f", ts.GetFramerate());
	ImGui::Text("Frametime: %f ms", ts.GetMilliseconds());
	ImGui::End();
}