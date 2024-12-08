#pragma once

#include "Core.h"

#include "MouseButtonCodes.h"
#include "KeyCodes.h"

namespace Helios {
	class HELIOSENGINE_API Input {
	public:
		static inline bool IsKeyPressed(KeyCode keycode) { return m_Instance->IsKeyPressedImpl(keycode); }

		static inline bool IsMouseButtonPressed(KeyCode button) { return m_Instance->IsMouseButtonPressedImpl(button); }
		static inline std::pair<uint32_t, uint32_t> GetMousePosition()  { return m_Instance->GetMousePositionImpl(); }
		static inline uint32_t GetMouseX() { return m_Instance->GetMouseXImpl(); }
		static inline uint32_t GetMouseY() { return m_Instance->GetMouseYImpl(); }

	protected:
		virtual bool IsKeyPressedImpl(KeyCode keycode) const = 0;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMousePositionImpl() const = 0;
		virtual inline uint32_t GetMouseXImpl() const = 0;
		virtual inline uint32_t GetMouseYImpl() const = 0;

	private:
		static std::unique_ptr<Input> m_Instance;
	};
}