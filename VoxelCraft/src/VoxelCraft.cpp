#include <VoxelEngine.h>

#include <glm/glm.hpp>

class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("VoxelCraft"), m_CameraController(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -90.0f, 0.0f), 
			VoxelEngine::Application::Get().GetWindow().GetWidth() / (float)VoxelEngine::Application::Get().GetWindow().GetHeight())
	{
		m_Cube.Position = { 0.0f, 0.0f, 0.0f };
		m_Cube.Size = { 2.0f, 2.0f, 2.0f };
    m_Cube.Rotation = { 0.0f, 0.0f, 0.0f };
		m_Cube.TexCoord = { 0.0f, 0.0f };
	}

	void OnAttach() override
	{
		//VoxelEngine::Application::Get().GetWindow().SetVSync(false);
		//VoxelEngine::Application::Get().GetWindow().SetFramerate(60.0);

		m_CheckerboardTexture = VoxelEngine::Texture::Create(ROOT + "VoxelCraft/Assets/Textures/Dirt.png");
	}

	void OnDetach() override
	{
	}

	void OnUpdate(const VoxelEngine::Timestep ts) override
	{
		m_CameraController.OnUpdate(ts);
		
		if (m_Cube.Rotation.x >= 360.0f) { m_Cube.Rotation.x = 360.0f - m_Cube.Rotation.x; }
    else if (m_Cube.Rotation.y >= 360.0f) { m_Cube.Rotation.y = 360.0f - m_Cube.Rotation.y; }
    else if (m_Cube.Rotation.z >= 360.0f) { m_Cube.Rotation.z = 360.0f - m_Cube.Rotation.z; }

		//m_Cube.Rotation.x += 30.0f * ts;
    //m_Cube.Rotation.y += 30.0f * ts;
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

	glm::vec3 cubePositions[6] = {
		glm::vec3(0.0f, 0.0f, 4.0f), // front
		glm::vec3(0.0f, 0.0f, -4.0f), // back
		glm::vec3(0.0f, 4.0f, 0.0f), // up
		glm::vec3(0.0f, -4.0f, 0.0f), // down
		glm::vec3(4.0f, 0.0f, 0.0f), // left
		glm::vec3(-4.0f, 0.0f, 0.0f), // right
	};

	void Draw() override
	{
		VoxelEngine::Renderer::BeginScene(m_CameraController.GetCamera());

		for (const auto& pos : cubePositions) {
			VoxelEngine::Renderer::DrawCube(pos, m_Cube.Rotation, m_Cube.Size, m_CheckerboardTexture);
		}

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

	CubeData m_Cube;

	VoxelEngine::CameraController m_CameraController;

	std::shared_ptr<VoxelEngine::Texture> m_CheckerboardTexture;
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