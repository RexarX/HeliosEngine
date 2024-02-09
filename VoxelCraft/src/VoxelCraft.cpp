#include <VoxelEngine.h>
#include <VoxelEngine/Events/KeyEvent.h>

class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer()
		: Layer("Game")
	{
	}

	void OnUpdate() override
	{
		if (VoxelEngine::Input::IsKeyPressed(VoxelEngine::Key::Q)) {
			VE_TRACE("Tab key is pressed (poll)!");
		}
	}

	void OnEvent(VoxelEngine::Event& event) override
	{
		if (event.GetEventType() == VoxelEngine::EventType::KeyPressed) {
			VoxelEngine::KeyPressedEvent& e = (VoxelEngine::KeyPressedEvent&)event;
			if (e.GetKeyCode() == VoxelEngine::Key::Tab) { VE_TRACE("{0} key is pressed (event)!", e.GetKeyCode()); }
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