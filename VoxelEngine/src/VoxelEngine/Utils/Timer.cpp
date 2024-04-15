#include "Timer.h"

#include <GLFW/glfw3.h>

namespace VoxelEngine
{
	void Timer::Start()
	{
		m_StartTimeStamp = glfwGetTime();
	}

	double Timer::Stop()
	{
		m_StopTimeStamp = glfwGetTime();

		return m_StopTimeStamp - m_StartTimeStamp;
	}
}