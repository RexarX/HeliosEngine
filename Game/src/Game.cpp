#include <HeliosEngine.h>
#include <HeliosEngine/EntryPoint.h>

#include "Layers/GameLayer.h"

class Game final : public Helios::Application {
public:
	Game() : Helios::Application("Game") {
		PushLayer(new GameLayer());
	}

	virtual ~Game() = default;
};

inline Helios::Application* Helios::CreateApplication() {
	return new Game();
}