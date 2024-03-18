#include <VoxelEngine.h>
#include <glm/glm.hpp>


class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("VoxelCraft"), m_CameraController(1280.0f / 720.0f)
	{

	}

	void OnAttach() override
	{
		m_CheckerboardTexture = VoxelEngine::Texture::Create("../../../Assets/Textures/Checkerboard.png");
	}

	void OnDetach() override
	{
	}

	void OnUpdate(VoxelEngine::Timestep ts) override
	{
		m_CameraController.OnUpdate(ts);
		glm::vec3 pos = { 0.0f, 0.0f, -3.0f };
		glm::vec3 rot = { 0.0f, 0.0f, 0.0f };
		VoxelEngine::Renderer::DrawCube(pos, rot, m_CheckerboardTexture);
	}

	void OnEvent(VoxelEngine::Event& event) override
	{
		m_CameraController.OnEvent(event);

		if (event.GetEventType() == VoxelEngine::EventType::MouseMoved) {
			VoxelEngine::MouseMovedEvent& e = (VoxelEngine::MouseMovedEvent&)event;
			VE_TRACE("Mouse moved to {0}x{1}!", e.GetX(), e.GetY());
		}

		if (event.GetEventType() == VoxelEngine::EventType::MouseButtonPressed) {
			VoxelEngine::MouseButtonPressedEvent& e = (VoxelEngine::MouseButtonPressedEvent&)event;
			VE_TRACE("Mouse button {0} pressed!", e.GetMouseButton());
		}

		if (event.GetEventType() == VoxelEngine::EventType::MouseButtonReleased) {
			VoxelEngine::MouseButtonReleasedEvent& e = (VoxelEngine::MouseButtonReleasedEvent&)event;
			VE_TRACE("Mouse button {0} released!", e.GetMouseButton());
		}

		if (event.GetEventType() == VoxelEngine::EventType::MouseScrolled) {
			VoxelEngine::MouseScrolledEvent& e = (VoxelEngine::MouseScrolledEvent&)event;
			VE_TRACE("Mouse scrolled ({0}, {1})!", e.GetXOffset(), e.GetYOffset());
		}

		if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
			VoxelEngine::KeyPressedEvent& e = (VoxelEngine::KeyPressedEvent&)event;
			VE_TRACE("{0} key is pressed!", e.GetKeyCode());
		}

		if (event.GetEventType() == VoxelEngine::EventType::KeyReleased) {
			VoxelEngine::KeyReleasedEvent& e = (VoxelEngine::KeyReleasedEvent&)event;
			VE_TRACE("{0} key is released!", e.GetKeyCode());
		}

		if (event.GetEventType() == VoxelEngine::EventType::WindowResize) {
			VoxelEngine::WindowResizeEvent& e = (VoxelEngine::WindowResizeEvent&)event;
			VE_TRACE("Window resized to {0}x{1}", e.GetWidth(), e.GetHeight());
		}

		if (event.GetEventType() == VoxelEngine::EventType::WindowClose) {
			VoxelEngine::WindowCloseEvent& e = (VoxelEngine::WindowCloseEvent&)event;
			VE_TRACE("Window closed");
		}
	}

private:
	VoxelEngine::CameraController m_CameraController;

	std::shared_ptr<VoxelEngine::VertexArray> m_SquareVA;
	std::shared_ptr<VoxelEngine::Shader> m_FlatColorShader;

	std::shared_ptr<VoxelEngine::Texture> m_CheckerboardTexture;
};

class VoxelCraft : public VoxelEngine::Application
{
public:
	VoxelCraft()
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