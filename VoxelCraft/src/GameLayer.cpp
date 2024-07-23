#include "GameLayer.h"

GameLayer::GameLayer()
	: Layer("VoxelCraft")
{
}

void GameLayer::OnAttach()
{
	Engine::Window& window = Engine::Application::Get().GetWindow();
	//window.SetVSync(false);
	//window.SetFramerate(30.0);

	Engine::SceneManager::AddScene("GameScene");
	Engine::SceneManager::SetActiveScene("GameScene");

	Engine::Scene& gameScene = Engine::SceneManager::GetScene("GameScene");
	
	Engine::InputSystem& inputSystem = gameScene.RegisterSystem<Engine::InputSystem>(0);
	Engine::EventSystem& eventSystem = gameScene.RegisterSystem<Engine::EventSystem>(1);

	Engine::SceneNode* player = gameScene.AddNode("Player");

	Engine::Camera& camera = gameScene.EmplaceNodeComponent<Engine::Camera>(
		player, glm::vec3(0.0f), glm::vec3(0.0f),
		static_cast<float>(window.GetWidth()) / window.GetHeight()
	);

	Engine::CameraController& cameraController = gameScene.EmplaceNodeComponent<Engine::CameraController>(
		player, gameScene.GetECSManager(), camera
	);
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(const Engine::Timestep ts)
{
	Engine::Scene& scene = Engine::SceneManager::GetScene("GameScene");
	scene.OnUpdate(ts);
}

void GameLayer::OnEvent(Engine::Event& event)
{
	Engine::Scene& scene = Engine::SceneManager::GetScene("GameScene");
	scene.OnEvent(event);
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