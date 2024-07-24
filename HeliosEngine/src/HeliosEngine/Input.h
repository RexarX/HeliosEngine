#pragma once

#include "Core.h"

#include "MouseButtonCodes.h"
#include "KeyCodes.h"

namespace Helios
{
	class HELIOSENGINE_API Input
	{
	public:
		inline static const bool IsKeyPressed(const KeyCode keycode) { return m_Instance->IsKeyPressedImpl(keycode); }

		inline static const bool IsMouseButtonPressed(const KeyCode button) { return m_Instance->IsMouseButtonPressedImpl(button); }
		inline static const std::pair<float, float> GetMousePosition() { return m_Instance->GetMousePositionImpl(); }
		inline static const float GetMouseX() { return m_Instance->GetMouseXImpl(); }
		inline static const float GetMouseY() { return m_Instance->GetMouseYImpl(); }

	protected:
		virtual const bool IsKeyPressedImpl(const KeyCode keycode) = 0;

		virtual const bool IsMouseButtonPressedImpl(const MouseCode button) = 0;
		virtual const std::pair<float, float> GetMousePositionImpl() = 0;
		virtual const float GetMouseXImpl() = 0;
		virtual const float GetMouseYImpl() = 0;

	private:
		static std::unique_ptr<Input> m_Instance;
	};
}