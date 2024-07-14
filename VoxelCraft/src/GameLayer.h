#pragma once

#include <VoxelEngine.h>

class GameLayer : public Engine::Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(const Engine::Timestep ts) override;
	void OnEvent(Engine::Event& event) override;
	void OnImGuiRender(ImGuiContext* context) override;
};