#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

namespace Helios::Utils
{
  class ThreadPool
  {
  public:
    ThreadPool(const uint32_t numThreads);
    ~ThreadPool();

    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
      using return_type = typename std::invoke_result<F, Args...>::type;

      auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
      );

      std::future<return_type> res = task->get_future();
      std::unique_lock<std::mutex> lock(m_QueueMutex);
      if (m_Stop) {
        CORE_ERROR("Enqueue on stopped ThreadPool!");
      }
      m_Tasks.emplace([task]() { (*task)(); });
      m_Condition.notify_one();
      return res;
    }

  private:
    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;

    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    bool m_Stop = false;
  };
}