#include "GameLayer.h"

#include "Scripts/Player/CameraController.h"

void GameLayer::OnAttach() {
	Helios::Window& window = Helios::Application::Get().GetWindow();
	//window.SetVSync(true);
	//window.SetFramerate(60.0);

	Helios::Scene& gameScene = Helios::SceneManager::AddScene("GameScene");

	Helios::Entity& root = gameScene.GetRootEntity();

	Helios::Entity& camera = gameScene.CreateEntity("Camera");
	camera.EmplaceComponent<Helios::Camera>().currect = true;
	camera.EmplaceComponent<Helios::Transform>();
	camera.EmplaceScriptComponent<CameraController>();

	root.AddChild(camera);

	gameScene.SetActive(true);
	gameScene.Load();
}

void GameLayer::OnDetach() {}

void GameLayer::OnUpdate(Helios::Timestep ts) {
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnUpdate(ts);
}

void GameLayer::OnEvent(Helios::Event& event) {
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.OnEvent(event);
}

void GameLayer::Draw() {
	Helios::Scene& scene = Helios::SceneManager::GetScene("GameScene");
	scene.Draw();
}

void GameLayer::OnImGuiRender(ImGuiContext* context) {
	PROFILE_FUNCTION();

	const Helios::Application& app = Helios::Application::Get();
	Helios::Timestep ts = app.GetDeltaTime();

	ImGui::SetCurrentContext(context);
	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %u", static_cast<uint32_t>(ts.GetFramerate()));
	ImGui::Text("Frametime: %f ms", ts.GetMilliseconds());
	ImGui::Text("Frames rendered: %u", app.GetFrameNumber());
	ImGui::End();
}