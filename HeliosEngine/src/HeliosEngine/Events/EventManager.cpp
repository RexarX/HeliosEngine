#include "EventManager.h"

namespace Helios
{
  std::queue<std::reference_wrapper<Event>> EventManager::m_EventQueue;
  std::function<void(Event&)> EventManager::m_Callback;

  void EventManager::ProcessQueuedEvents()
  {
    //Timer timer;
    //timer.Start();
    //CORE_TRACE("Processing {0} queued events.", m_EventQueue.size());
    while (!m_EventQueue.empty()) {
      m_Callback(m_EventQueue.front().get());
      m_EventQueue.pop();
    }
    //timer.Stop();
    //CORE_TRACE("Event processing took: {0} ms.", timerEvent.GetElapsedMillisec());
  }
}