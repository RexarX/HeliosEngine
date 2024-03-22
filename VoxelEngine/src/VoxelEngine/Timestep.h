#pragma once

namespace VoxelEngine
{
	class Timestep
	{
	public:
		Timestep(const float time = 0.0f)
			: m_Time(time)
		{
		}

		operator float() const { return m_Time; }

		float GetSeconds() const { return m_Time; }
		float GetMilliseconds() const { return m_Time * 1000.0f; }
		float GetFramerate() const { return 1000.0f / GetMilliseconds(); }

	private:
		float m_Time;
	};
}