#include "EventSystem.h"

namespace Helios {
  void EventSystem::OnUpdate() {
    PROFILE_FUNCTION();

    ProcessEvents();
  }

  void EventSystem::ProcessEvents() {
    for (auto& [type, events] : m_Events) {
      auto it = m_EventListeners.find(type);
      const std::vector<Listener>& listeners = it->second;
      if (it != m_EventListeners.end() && !listeners.empty()) {
        uint64_t eventSize = m_EventSizes[type];
        for (uint64_t offset = 0; offset < events.size(); offset += eventSize) {
          Event* event = reinterpret_cast<Event*>(events.data() + offset);
          for (const Listener& listener : listeners) {
            listener.callback(*event);
          }
        }
      }
      events.clear();
    }
  }
}