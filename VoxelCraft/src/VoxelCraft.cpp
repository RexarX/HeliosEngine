#include <VoxelEngine.h>

#include <glm/glm.hpp>

class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("VoxelCraft"), m_CameraController(glm::vec3(0.0f, 0.0f, 12.0f), glm::vec3(0.0f, -90.0f, 0.0f), 
			VoxelEngine::Application::Get().GetWindow().GetWidth() / (float)VoxelEngine::Application::Get().GetWindow().GetHeight())
	{
	}

	void OnAttach() override
	{
		//VoxelEngine::Application::Get().GetWindow().SetVSync(false);
		//VoxelEngine::Application::Get().GetWindow().SetFramerate(60.0);

		m_CheckerboardTexture = VoxelEngine::Texture::Create(ROOT + "VoxelCraft/Assets/Textures/Checkerboard.png");
		m_DirtTexture = VoxelEngine::Texture::Create(ROOT + "VoxelCraft/Assets/Textures/Dirt.png");

		for (float i = 0; i < 25; ++i) {
			for (float j = 0; j < 25; ++j) {
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
		static double LastFramerate;

		m_CameraController.OnEvent(event);

		if (event.GetEventType() == VoxelEngine::EventType::MouseMoved) {
			VoxelEngine::MouseMovedEvent& e = (VoxelEngine::MouseMovedEvent&)event;
			VE_TRACE("Mouse moved to {0}x{1}!", e.GetX(), e.GetY());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::MouseButtonPressed) {
			VoxelEngine::MouseButtonPressedEvent& e = (VoxelEngine::MouseButtonPressedEvent&)event;
			VE_TRACE("Mouse button {0} pressed!", e.GetMouseButton());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::MouseButtonReleased) {
			VoxelEngine::MouseButtonReleasedEvent& e = (VoxelEngine::MouseButtonReleasedEvent&)event;
			VE_TRACE("Mouse button {0} released!", e.GetMouseButton());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::MouseScrolled) {
			VoxelEngine::MouseScrolledEvent& e = (VoxelEngine::MouseScrolledEvent&)event;
			VE_TRACE("Mouse scrolled ({0}, {1})!", e.GetXOffset(), e.GetYOffset());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
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
			VE_TRACE("{0} key is pressed!", e.GetKeyCode());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::KeyReleased) {
			VoxelEngine::KeyReleasedEvent& e = (VoxelEngine::KeyReleasedEvent&)event;
			VE_TRACE("{0} key is released!", e.GetKeyCode());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::WindowResize) {
			VoxelEngine::WindowResizeEvent& e = (VoxelEngine::WindowResizeEvent&)event;
			if (e.GetHeight() == 0 && e.GetWidth() == 0) {
				VoxelEngine::Application::Get().GetWindow().SetMinimized(true);
			}
			else if (VoxelEngine::Application::Get().GetWindow().IsMinimized()) {
				VoxelEngine::Application::Get().GetWindow().SetMinimized(false);
			}
			VE_TRACE("Window resized to {0}x{1}", e.GetWidth(), e.GetHeight());
		}

		else if (event.GetEventType() == VoxelEngine::EventType::WindowClose) {
			VoxelEngine::WindowCloseEvent& e = (VoxelEngine::WindowCloseEvent&)event;
			VE_TRACE("Window closed");
		}
	}

	void Draw() override
	{
		VoxelEngine::Renderer::BeginScene(m_CameraController.GetCamera());
		m_CameraController.GetFrustum().CreateFrustum(m_CameraController.GetCamera());

		int cubeCount = 0;
		for (const auto& cube : m_Cube) {
			if (m_CameraController.GetFrustum().IsCubeInFrustrum(cube.Size.x, cube.Position)) {
				VoxelEngine::Renderer::DrawCube(cube.Position, cube.Rotation, cube.Size, m_DirtTexture);
				++cubeCount;
			}
			else {
        VoxelEngine::Renderer::DrawCube(cube.Position, cube.Rotation, cube.Size, m_CheckerboardTexture);
			}
		}
		std::string debug = std::to_string(cubeCount) + " | " + std::to_string(m_Cube.size());
		VE_TRACE(std::string_view(debug));

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