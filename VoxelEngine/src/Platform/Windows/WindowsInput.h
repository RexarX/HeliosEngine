#pragma once

#include "VoxelEngine/Input.h"

namespace Engine
{
	class WindowsInput : public Input
	{
	protected:
		virtual const bool IsKeyPressedImpl(const KeyCode keycode) override;

		virtual const bool IsMouseButtonPressedImpl(const MouseCode button) override;
		virtual const std::pair<float, float> GetMousePositionImpl() override;
		virtual const float GetMouseXImpl() override;
		virtual const float GetMouseYImpl() override;
	};
}