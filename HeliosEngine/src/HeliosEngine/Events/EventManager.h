#pragma once

#include "Event.h"

namespace Helios
{
  class HELIOSENGINE_API EventManager
  {
  public:
    EventManager() = default;
    ~EventManager() = default;

    template<typename T>
    static void PushEvent(T& event);

    friend class Application;

  private:
    static void SetCallback(const std::function<void(Event&)>& callback) { m_Callback = callback; }
    static void ProcessQueuedEvents();

  private:
    static std::queue<std::reference_wrapper<Event>> m_EventQueue;
    static std::function<void(Event&)> m_Callback;
  };

  template<typename T>
  void EventManager::PushEvent(T& event)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");
    m_EventQueue.push(event);
  }
}