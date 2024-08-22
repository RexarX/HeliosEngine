#pragma once

namespace Helios
{
	class Timestep
	{
	public:
		Timestep(double time = 0.0f) : m_Time(time) {}

		operator float() const { return static_cast<float>(m_Time); }
		operator double() const { return m_Time; }

		inline double GetSeconds() const { return m_Time; }
		inline double GetMilliseconds() const { return m_Time * 1000.0f; }
		inline double GetFramerate() const { return 1000.0f / GetMilliseconds(); }

	private:
		double m_Time;
	};
}