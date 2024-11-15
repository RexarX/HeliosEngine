#pragma once

#include <HeliosEngine.h>

class CameraController final : public Helios::Scriptable {
public:
	CameraController() = default;
	virtual ~CameraController() { OnDetach(); }

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Helios::Timestep deltaTime) override;
	void OnEvent(Helios::Event& event) override;

	void SetCameraTranslationSpeed(float speed) { m_CameraTranslationSpeed = speed; }
	void SetCameraRotationSpeed(float speed) { m_CameraRotationSpeed = speed; }

private:
	bool OnMouseMoveEvent(Helios::MouseMoveEvent& event);
	bool OnWindowFocusEvent(Helios::WindowFocusEvent& event);
	bool OnWindowLostFocusEvent(Helios::WindowLostFocusEvent& event);

private:
	float m_CameraTranslationSpeed = 5.0f;
	float m_CameraRotationSpeed = 0.1f;

	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	bool m_FirstInput = true;
	float m_LastX = 0.0f;
  float m_LastY = 0.0f;
};