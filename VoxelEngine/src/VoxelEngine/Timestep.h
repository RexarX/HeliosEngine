#pragma once

namespace Engine
{
	class Timestep
	{
	public:
		Timestep(const double time = 0.0f) : m_Time(time) {}

		operator double() const { return m_Time; }

		inline const double GetSeconds() const { return m_Time; }
		inline const double GetMilliseconds() const { return m_Time * 1000.0f; }
		inline const double GetFramerate() const { return 1000.0f / GetMilliseconds(); }

	private:
		double m_Time;
	};
}