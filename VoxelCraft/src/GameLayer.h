#pragma once

#include <VoxelEngine.h>

class GameLayer : public VoxelEngine::Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(const VoxelEngine::Timestep ts) override;
	virtual void OnEvent(VoxelEngine::Event& event) override;
	virtual void OnImGuiRender(ImGuiContext* context) override;
};