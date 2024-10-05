#pragma once

namespace Helios
{
	class Timestep
	{
	public:
		Timestep(double time = 0.0) : m_Time(time) {}

		inline operator float() const { return static_cast<float>(m_Time); }
		inline operator double() const { return m_Time; }

		inline double GetSeconds() const { return m_Time; }
		inline double GetMilliseconds() const { return m_Time * 1000.0; }
		inline double GetFramerate() const { return 1000.0 / GetMilliseconds(); }

	private:
		double m_Time = 0.0;
	};
}