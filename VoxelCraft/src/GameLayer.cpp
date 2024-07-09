#include "GameLayer.h"

GameLayer::GameLayer()
	: Layer("VoxelCraft")
{
}

void GameLayer::OnAttach()
{
	//VoxelEngine::Window& window = VoxelEngine::Application::Get().GetWindow();
	//window.SetVSync(false);
	//window.SetFramerate(60.0);

	VoxelEngine::Renderer::SetAnisoLevel(16.0f);
	VoxelEngine::Renderer::SetGenerateMipmaps(true);

	VoxelEngine::ModelLoader::LoadModel(VOXELCRAFT_DIR + "Assets/Models/untitled");
	
	VoxelEngine::SceneManager::AddScene("default");
  VoxelEngine::SceneManager::SetActiveScene("default");

	VoxelEngine::Scene& scene = VoxelEngine::SceneManager::GetActiveScene();
	VoxelEngine::Object& obj = VoxelEngine::ModelLoader::GetObj();

	for (uint32_t i = 0; i < 1; ++i) {
		for (uint32_t j = 0; j < 1; ++j) {
			obj.SetPosition(glm::vec3(i, 0, j));
			scene.AddObject(obj);
		}
	}
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(const VoxelEngine::Timestep ts)
{
	VoxelEngine::Scene& scene = VoxelEngine::SceneManager::GetActiveScene();

	if (VoxelEngine::Application::Get().GetWindow().IsFocused()) {
		VoxelEngine::SceneManager::GetActiveScene().OnUpdate(ts);
	}

  VoxelEngine::SceneManager::GetActiveScene().Render();
}

void GameLayer::OnEvent(VoxelEngine::Event& event)
{
	VoxelEngine::Scene& scene = VoxelEngine::SceneManager::GetActiveScene();

	if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
		VoxelEngine::Window& window = VoxelEngine::Application::Get().GetWindow();

		VoxelEngine::KeyPressedEvent& e = (VoxelEngine::KeyPressedEvent&)event;
		switch (e.GetKeyCode())
		{
		case VoxelEngine::Key::Escape:
			if (window.IsFocused()) {
				window.SetFocused(false);
				scene.GetCameraController().SetFirstInput();
			}
			else { window.SetFocused(true); }
			break;

		case VoxelEngine::Key::F1:
			window.SetVSync(!window.IsVSync());
			break;

		case VoxelEngine::Key::F2:
			window.SetFullscreen(!window.IsFullscreen());
			break;

		case VoxelEngine::Key::F3:
			window.SetImGuiState(!window.IsImGuiEnabled());
			break;
		}
	}
}

void GameLayer::OnImGuiRender(ImGuiContext* context)
{
	VoxelEngine::Timestep ts = VoxelEngine::Application::Get().GetDeltaTime();

	ImGui::SetCurrentContext(context);

	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %f", ts.GetFramerate());
	ImGui::Text("Frametime: %f", ts.GetMilliseconds());
	ImGui::End();
}