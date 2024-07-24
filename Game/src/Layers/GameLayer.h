#pragma once

#include <HeliosEngine.h>

class GameLayer : public Helios::Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(const Helios::Timestep ts) override;
	void OnEvent(Helios::Event& event) override;
	void OnImGuiRender(ImGuiContext* context) override;
};