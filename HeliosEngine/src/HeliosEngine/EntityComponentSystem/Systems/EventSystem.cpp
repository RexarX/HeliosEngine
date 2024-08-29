#include "EventSystem.h"

#include "Events/ApplicationEvent.h"

namespace Helios
{
  void EventSystem::OnUpdate(entt::registry& registry)
  {
    ProcessQueuedEvents();
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
        while (!queue.empty()) {
          for (const Listener& listener : it->second) {
            listener.callback(*queue.front());
          }
          queue.pop();
        }
      }
      //timerEvent.Stop();
      //CORE_TRACE("{0} took: {1} ms", type.name(), timerEvent.GetElapsedMillisec());
    }
    //timer.Stop();
    //CORE_TRACE("Total event processing took: {0} ms", timer.GetElapsedMillisec());
  }
}