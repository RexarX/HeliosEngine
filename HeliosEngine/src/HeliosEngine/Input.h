#pragma once

#include "Core.h"

#include "MouseButtonCodes.h"
#include "KeyCodes.h"

namespace Helios
{
	class HELIOSENGINE_API Input
	{
	public:
		inline static bool IsKeyPressed(KeyCode keycode) { return m_Instance->IsKeyPressedImpl(keycode); }

		inline static bool IsMouseButtonPressed(KeyCode button) { return m_Instance->IsMouseButtonPressedImpl(button); }
		inline static std::pair<float, float> GetMousePosition() { return m_Instance->GetMousePositionImpl(); }
		inline static float GetMouseX() { return m_Instance->GetMouseXImpl(); }
		inline static float GetMouseY() { return m_Instance->GetMouseYImpl(); }

	protected:
		virtual bool IsKeyPressedImpl(KeyCode keycode) = 0;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) = 0;
		virtual std::pair<float, float> GetMousePositionImpl() = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;

	private:
		static std::unique_ptr<Input> m_Instance;
	};
}