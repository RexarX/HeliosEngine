#pragma once

#include <HeliosEngine.h>

class CameraController : public Helios::Script
{
public:
	CameraController(Helios::Camera& camera);
	virtual ~CameraController();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(const Helios::Timestep deltaTime) override;
	void OnEvent(Helios::Event& event) override;

	void SetCameraTranslationSpeed(const float speed) { m_CameraTranslationSpeed = speed; }
	void SetCameraRotationSpeed(const float speed) { m_CameraRotationSpeed = speed; }

	inline const glm::vec3& GetPosition() const { return m_Position; }
	inline const glm::vec3& GetRotation() const { return glm::vec3(m_Pitch, m_Yaw, 0.0f); }

private:
	const bool OnMouseMovedEvent(Helios::MouseMovedEvent& event);
	const bool OnWindowFocusedEvent(Helios::WindowFocusedEvent& event);
	const bool OnWindowLostFocusEvent(Helios::WindowLostFocusEvent& event);

private:
	Helios::Camera& m_Camera;

	glm::vec3 m_Position = glm::vec3(0.0f);

	float m_CameraTranslationSpeed = 5.0f;
	float m_CameraRotationSpeed = 0.1f;

	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	bool m_FirstInput = true;
	float m_LastX = 0.0f;
  float m_LastY = 0.0f;
};