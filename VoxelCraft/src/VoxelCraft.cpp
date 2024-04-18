#include <VoxelEngine.h>

#include <glm/glm.hpp>

class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("VoxelCraft"), m_CameraController(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -90.0f, 0.0f), 
			VoxelEngine::Application::Get().GetWindow().GetWidth() / (float)VoxelEngine::Application::Get().GetWindow().GetHeight())
	{
	}

	void OnAttach() override
	{
		VoxelEngine::Application::Get().GetWindow().SetVSync(false);
		//VoxelEngine::Application::Get().GetWindow().SetFramerate(60.0);

		m_CheckerboardTexture = VoxelEngine::Texture::Create(ROOT + "VoxelCraft/Assets/Textures/Checkerboard.png");
		m_DirtTexture = VoxelEngine::Texture::Create(ROOT + "VoxelCraft/Assets/Textures/Dirt.png");

		for (float i = 0; i < 100; ++i) {
			for (float j = 0; j < 100; ++j) {
				m_Cube.push_back({ glm::vec3(i, -1.0f, j), glm::vec3(1.0f, 1.0f, 1.0f),
					glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) });
			}
		}
	}

	void OnDetach() override
	{
	}

	void OnUpdate(const VoxelEngine::Timestep ts) override
	{
		if (VoxelEngine::Application::Get().GetWindow().IsFocused()) {
			m_CameraController.OnUpdate(ts);
		}
	}

	void OnEvent(VoxelEngine::Event& event) override
	{
		m_CameraController.OnEvent(event);

		if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
			VoxelEngine::KeyPressedEvent& e = (VoxelEngine::KeyPressedEvent&)event;
			if (e.GetKeyCode() == VoxelEngine::Key::Escape) {
				if (VoxelEngine::Application::Get().GetWindow().IsFocused()) {
					VoxelEngine::Application::Get().GetWindow().SetFocused(false);
					m_CameraController.SetFirstInput();
				}
				else {
					VoxelEngine::Application::Get().GetWindow().SetFocused(true);
				}
			}
		}
	}

	void Draw() override
	{
		VoxelEngine::Renderer::BeginScene(m_CameraController.GetCamera());
		m_CameraController.GetFrustum().CreateFrustum(m_CameraController.GetCamera());
		for (const auto& cube : m_Cube) {
			if (m_CameraController.GetFrustum().IsCubeInFrustrum(cube.Size.x, cube.Position)) {
				VoxelEngine::Renderer::DrawCube(cube.Position, cube.Rotation, cube.Size, m_DirtTexture);
			}
		}
		VoxelEngine::Renderer::DrawLine(glm::vec3(12.5f, 12.5f, 12.5f), glm::vec3(0.0f, 0.0f, 0.0f), 100.0f); // (ray origin, ray direction, ray lenght)

		VoxelEngine::Renderer::EndScene();
	}

private:
	struct CubeData
	{
		glm::vec3 Position;
		glm::vec3 Size;
		glm::vec3 Rotation;
		glm::vec2 TexCoord;
	};

	std::vector<CubeData> m_Cube;

	VoxelEngine::CameraController m_CameraController;

	std::shared_ptr<VoxelEngine::Texture> m_CheckerboardTexture;
	std::shared_ptr<VoxelEngine::Texture> m_DirtTexture;
};

class VoxelCraft : public VoxelEngine::Application
{
public:
	VoxelCraft()
		: VoxelEngine::Application()
	{
		PushLayer(new GameLayer());
		//PushOverlay(new VoxelEngine::ImGuiLayer());
	}

	~VoxelCraft()
	{
	}
};

VoxelEngine::Application* VoxelEngine::CreateApplication()
{
	return new VoxelCraft();
}