#include "Timer.h"

#include <GLFW/glfw3.h>

namespace VoxelEngine
{
	void Timer::Start()
	{
		m_StartTimeStamp = glfwGetTime();
	}

	void Timer::Stop()
	{
		m_StopTimeStamp = glfwGetTime();
		m_Elapsed = m_StopTimeStamp - m_StartTimeStamp;
	}

	double Timer::GetElapsedSec()
	{
		return m_Elapsed;
	}
}