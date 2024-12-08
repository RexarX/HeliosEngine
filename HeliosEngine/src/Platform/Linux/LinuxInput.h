#pragma once

#include "HeliosEngine/Input.h"

namespace Helios {
	class LinuxInput final : public Input {
	protected:
		bool IsKeyPressedImpl(KeyCode keycode) const override;

		bool IsMouseButtonPressedImpl(MouseCode button) const override;
		std::pair<uint32_t, uint32_t> GetMousePositionImpl() const override;

		inline uint32_t GetMouseXImpl() const override {
			auto [x, y] = GetMousePositionImpl();
			return x;
		}

		inline uint32_t GetMouseYImpl() const override {
			auto [x, y] = GetMousePositionImpl();
			return y;
		}
	};
}