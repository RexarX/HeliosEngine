#pragma once

#include <HeliosEngine.h>

class GameLayer : public Helios::Layer
{
public:
	GameLayer() : Layer("VoxelCraft") {}

	virtual ~GameLayer() = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Helios::Timestep ts) override;
	void OnEvent(Helios::Event& event) override;
	void Draw() override;
	void OnImGuiRender(ImGuiContext* context) override;
};