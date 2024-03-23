#include "Timer.h"

#include <GLFW/glfw3.h>

namespace VoxelEngine
{
	void Timer::Start()
	{
		startTimeStamp = glfwGetTime();
	}

	double Timer::Stop()
	{
		stopTimeStamp = glfwGetTime();

		return stopTimeStamp - startTimeStamp;
	}
}