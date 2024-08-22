#include "Utils/ThreadPool.h"

namespace Helios::Utils
{
  ThreadPool::ThreadPool(uint32_t numThreads)
  {
    for (uint32_t i = 0; i < numThreads; ++i) {
      m_Workers.emplace_back([this] {
        while (true) {
          std::unique_lock<std::mutex> lock(m_QueueMutex);
          m_Condition.wait(lock, [this] {
            return m_Stop || !m_Tasks.empty();
          });
          if (m_Stop && m_Tasks.empty()) { return; }
          m_Tasks.front()();
          m_Tasks.pop();
        }
      });
    }
  }

  ThreadPool::~ThreadPool()
  {
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    m_Stop = true;
    m_Condition.notify_all();
    for (std::thread& worker : m_Workers) {
      worker.join();
    }
  }
}