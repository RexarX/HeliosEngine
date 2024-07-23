#include "EntityComponentSystem/Systems/EventSystem.h"

#include "EntityComponentSystem/Manager/ECSManager.h"

#include "Events/ApplicationEvent.h"

namespace Engine
{
  void EventSystem::OnUpdate(ECSManager& ecs, const Timestep deltaTime)
  {
    AppUpdateEvent event(deltaTime);
    PushEvent(event);
    ProcessQueuedEvents();
  }

  void EventSystem::OnEvent(ECSManager& ecs, Event& event)
  {
  }

  void EventSystem::ProcessQueuedEvents()
  {
    //Timer timer, timerEvent;
    //timer.Start();
    for (auto& [type, queueBytes] : m_EventQueues) {
      //timerEvent.Start();
      auto it = m_EventListeners.find(type);
      if (it != m_EventListeners.end() && !it->second.empty()) {
        auto* queue = reinterpret_cast<EventQueue<Event>*>(queueBytes.data());
        //TRACE("{0} event count: {1}", type.name(), queue->events.size());
        while (!queue->events.empty()) {
          for (const auto& listener : it->second) {
            listener.callback(queue->events.front());
          }
          queue->events.pop();
        }
      }
      //timerEvent.Stop();
      //TRACE("{0} took: {1} ms", type.name(), timerEvent.GetElapsedMillisec());
    }
    //timer.Stop();
    //TRACE("Total event processing took: {0} ms", timer.GetElapsedMillisec());
  }
}