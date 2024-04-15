#pragma once

namespace VoxelEngine
{
	class Timer
	{
	public:
		void Start();
		void Stop();
		double GetElapsedSec();

	private:
		double m_StartTimeStamp;
		double m_StopTimeStamp;
		double m_Elapsed;
	};
}