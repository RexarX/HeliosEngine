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

	virtual void Draw() override;

private:
	VoxelEngine::CameraController m_CameraController;

	std::vector<glm::mat3> m_Cube;
	std::vector<glm::mat3> m_ToDraw;

	std::shared_ptr<VoxelEngine::Texture> m_Skybox;
	std::shared_ptr<VoxelEngine::Texture> m_CheckerboardTexture;
	std::shared_ptr<VoxelEngine::Texture> m_DirtTexture;
};