#pragma once

#include <HeliosEngine.h>

class GameLayer final : public Helios::Layer
{
public:
	GameLayer() : Layer("Game") {}

	virtual ~GameLayer() = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Helios::Timestep ts) override;
	void OnEvent(const Helios::Event& event) override;
	void Draw() override;
	void OnImGuiRender(ImGuiContext* context) override;
};