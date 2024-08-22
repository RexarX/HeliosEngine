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
      if (it != m_EventListeners.end() && !it->second.empty()) {
        // Remove expired listeners
        it->second.erase(
          std::remove_if(
            it->second.begin(), it->second.end(),
            [](const Listener& listener) {
              return listener.instance.expired();
            }
          ),
          it->second.end()
        );

        //timerEvent.Start();
        //TRACE("{0} event count: {1}", type.name(), queue.size());
        // Process events
        while (!queue.empty()) {
          for (const Listener& listener : it->second) {
            if (auto instance = listener.instance.lock()) {
              listener.callback(queue.front().get());
            }
          }
          queue.pop();
        }
      }
      //timerEvent.Stop();
      //TRACE("{0} took: {1} ms", type.name(), timerEvent.GetElapsedMillisec());
    }
    //timer.Stop();
    //TRACE("Total event processing took: {0} ms", timer.GetElapsedMillisec());
  }
}