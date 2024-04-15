#pragma once

namespace VoxelEngine
{
	class Timer
	{
	public:
		void Start();
		double Stop();

	private:
		double m_StartTimeStamp;
		double m_StopTimeStamp;
	};
}