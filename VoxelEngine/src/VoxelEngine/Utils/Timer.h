#pragma once

namespace VoxelEngine
{
	class Timer
	{
	public:
		void Start();
		double Stop();

	private:
		double startTimeStamp;
		double stopTimeStamp;
	};
}