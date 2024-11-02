#include "EventSystem.h"

namespace Helios
{
  EventSystem::~EventSystem()
  {
    for (auto& [type, queue] : m_EventQueues) {
      for (const Event* event : queue) {
        delete event;
      }
    }
  }

  void EventSystem::ProcessQueuedEvents()
  {
    //Timer timer, timerEvent;
    //timer.Start();
    for (auto& [type, queue] : m_EventQueues) {
      auto it = m_EventListeners.find(type);
      //timerEvent.Start();
      //CORE_TRACE("{0} event count: {1}", type.name(), queue.size());
      if (it != m_EventListeners.end() && !it->second.empty()) {
        for (const Event* event : queue) {
          for (const Listener& listener : it->second) {
            listener.callback(*event);
          }
          delete event;
        }
      }

      queue.clear();
      //timerEvent.Stop();
      //CORE_TRACE("{0} took: {1} us", type.name(), timerEvent.GetElapsedMicroSec());
    }
    //timer.Stop();
    //CORE_TRACE("Total event processing took: {0} us", timer.GetElapsedMicroSec());
  }
}