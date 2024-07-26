#include "GameLayer.h"

#include "Scripts/Player/CameraController.h"

GameLayer::GameLayer()
	: Layer("VoxelCraft")
{
}

void GameLayer::OnAttach()
{
	Helios::Window& window = Helios::Application::Get().GetWindow();
	//window.SetVSync(true);
	//window.SetFramerate(30.0);

	Helios::SceneManager::AddScene("GameScene");
	Helios::SceneManager::SetActiveScene("GameScene");

	Helios::Scene& gameScene = Helios::SceneManager::GetScene("GameScene");

	gameScene.RegisterSystem<Helios::RenderingSystem>(0);
	gameScene.RegisterSystem<Helios::ScriptSystem>(1);
	gameScene.RegisterSystem<Helios::EventSystem>(2);

	Helios::SceneNode* player = gameScene.AddNode("Player");

	Helios::Camera& camera = gameScene.EmplaceNodeComponent<Helios::Camera>(
		player, glm::vec3(0.0f), glm::vec3(0.0f),
		static_cast<float>(window.GetWidth()) / window.GetHeight()
	);

	CameraController cameraController(camera);
	gameScene.AttachScript(player, cameraController);
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(const Helios::Timestep ts)
{
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnUpdate(ts);
}

void GameLayer::OnEvent(Helios::Event& event)
{
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnEvent(event);
}

void GameLayer::OnImGuiRender(ImGuiContext* context)
{
	Helios::Timestep ts = Helios::Application::Get().GetDeltaTime();

	ImGui::SetCurrentContext(context);

	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %f", ts.GetFramerate());
	ImGui::Text("Frametime: %f", ts.GetMilliseconds());
	ImGui::End();
}