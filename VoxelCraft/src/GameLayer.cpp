#include "GameLayer.h"

GameLayer::GameLayer()
	: Layer("VoxelCraft"), m_CameraController(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -90.0f, 0.0f),
		VoxelEngine::Application::Get().GetWindow().GetWidth() / (float)VoxelEngine::Application::Get().GetWindow().GetHeight())
{
}

void GameLayer::OnAttach()
{
	//VoxelEngine::Application::Get().GetWindow().SetVSync(false);
	//VoxelEngine::Application::Get().GetWindow().SetFramerate(60.0);

	VoxelEngine::Renderer::SetAnisoLevel(16.0f);
	VoxelEngine::Renderer::SetGenerateMipmaps(true);

	m_Skybox = VoxelEngine::Texture::Create(VOXELCRAFT_DIR + "Assets/Textures/Skybox/right.png",
																					VOXELCRAFT_DIR + "Assets/Textures/Skybox/left.png",
																					VOXELCRAFT_DIR + "Assets/Textures/Skybox/top.png",
																					VOXELCRAFT_DIR + "Assets/Textures/Skybox/bottom.png",
																					VOXELCRAFT_DIR + "Assets/Textures/Skybox/front.png",
																					VOXELCRAFT_DIR + "Assets/Textures/Skybox/back.png"
	);

	m_CheckerboardTexture = VoxelEngine::Texture::Create(VOXELCRAFT_DIR + "Assets/Textures/Checkerboard.png");
	m_DirtTexture = VoxelEngine::Texture::Create(VOXELCRAFT_DIR + "Assets/Textures/dirt(14vert).png");

	for (float i = 0; i < 100; ++i) {
		for (float j = 0; j < 100; ++j) {
			m_Cube.push_back({ glm::vec3(i, -1.0f, j), glm::vec3(1.0f, 1.0f, 1.0f),
				glm::vec3(0.0f, 0.0f, 0.0f) });
		}
	}
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(const VoxelEngine::Timestep ts)
{
	if (VoxelEngine::Application::Get().GetWindow().IsFocused()) {
		m_CameraController.OnUpdate(ts);
	}
}

void GameLayer::OnEvent(VoxelEngine::Event& event)
{
	m_CameraController.OnEvent(event);

	if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
		VoxelEngine::KeyPressedEvent& e = (VoxelEngine::KeyPressedEvent&)event;
		switch (e.GetKeyCode())
		{
		case VoxelEngine::Key::Escape:
			if (VoxelEngine::Application::Get().GetWindow().IsFocused()) {
				VoxelEngine::Application::Get().GetWindow().SetFocused(false);
				m_CameraController.SetFirstInput();
			}
			else {
				VoxelEngine::Application::Get().GetWindow().SetFocused(true);
			}
			break;

		case VoxelEngine::Key::F1:
			if (VoxelEngine::Application::Get().GetWindow().IsVSync()) {
				VoxelEngine::Application::Get().GetWindow().SetVSync(false);
			}
			else {
				VoxelEngine::Application::Get().GetWindow().SetVSync(true);
			}
			break;

		case VoxelEngine::Key::F2:
			if (VoxelEngine::Application::Get().GetWindow().IsFullscreen()) {
				VoxelEngine::Application::Get().GetWindow().SetFullscreen(false);
			}
			else {
				VoxelEngine::Application::Get().GetWindow().SetFullscreen(true);
			}
			break;
		}
	}
}

void GameLayer::OnImGuiRender(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);

	ImGui::Begin("Debug menu");
	ImGui::Text("FPS: %f", VoxelEngine::Application::Get().GetDeltaTime().GetFramerate());
	ImGui::End();
}

void GameLayer::Draw()
{
	VoxelEngine::Renderer::BeginScene(m_CameraController.GetCamera());
	m_CameraController.GetFrustum().CreateFrustum(m_CameraController.GetCamera());
	for (uint32_t i = 0; i < m_Cube.size(); ++i) {
		if (m_CameraController.GetFrustum().IsParallelepipedInFrustrum(m_Cube[i][0], m_Cube[i][1])) {
			m_ToDraw.push_back(m_Cube[i]);
		}
	}

	VoxelEngine::Renderer::DrawSkybox(m_Skybox);
	VoxelEngine::Renderer::DrawCubesInstanced(m_ToDraw, m_DirtTexture);
	VoxelEngine::Renderer::DrawLine(glm::vec3(12.5f, 12.5f, 12.5f), glm::vec3(0.0f, 0.0f, 0.0f), 100.0f); // (ray origin, ray direction, ray lenght)

	m_ToDraw.clear();

	VoxelEngine::Renderer::EndScene();
}