#include <VoxelEngine.h>


class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("Game")
	{
	}

	void OnUpdate() override
	{

	}

	void OnEvent(VoxelEngine::Event& event) override
	{
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
};

class Minecraft : public VoxelEngine::Application
{
public:
	Minecraft()
	{
		PushLayer(new GameLayer());
		//PushOverlay(new VoxelEngine::ImGuiLayer());
	}

	~Minecraft()
	{

	}
};

VoxelEngine::Application* VoxelEngine::CreateApplication()
{
	return new Minecraft();
}