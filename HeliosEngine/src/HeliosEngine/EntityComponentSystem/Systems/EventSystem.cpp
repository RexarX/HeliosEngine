#include "EventSystem.h"

namespace Helios {
  EventSystem::~EventSystem() {
    for (const auto& [type, events] : m_Events) {
      for (const Event* event : events) {
        delete event;
      }
    }
  }

  void EventSystem::OnUpdate() {
    PROFILE_FUNCTION();
    ProcessEvents();
  }

  void EventSystem::ProcessEvents() {
    std::for_each(std::execution::par, m_Events.begin(), m_Events.end(),
      [this](auto& pair) {
        std::vector<Event*>& events = pair.second;
        EventType type = pair.first;
        auto it = m_EventListeners.find(type);
        if (it != m_EventListeners.end() && !it->second.empty()) {
          for (Event* event : events) {
            for (const Listener& listener : it->second) {
              listener.callback(*event);
            }
            delete event;
          }
        }
        events.clear();
      });
  }
}