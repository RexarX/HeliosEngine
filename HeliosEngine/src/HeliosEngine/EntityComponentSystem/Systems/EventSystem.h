#pragma once

#include "pch.h"

#include <entt/entt.hpp>

namespace Helios {
  class HELIOSENGINE_API EventSystem {
  public:
    EventSystem() = default;
    EventSystem(const EventSystem&) = delete;
    EventSystem(EventSystem&&) noexcept = default;
    ~EventSystem();

    void OnUpdate();

    template <typename T> requires std::is_base_of_v<Event, T>
    void Emit(const T& event);

    template <typename T> requires std::is_base_of_v<Event, T>
    void AddListener(const void* instance, const std::function<void(T&)>& callback);

    template <typename T> requires std::is_base_of_v<Event, T>
    void RemoveListener(const void* instance);

    template <typename T> requires std::is_base_of_v<Event, T>
    void PushEvent(const T& event) { m_Events[typeid(T)].push_back(new T(event)); }

    EventSystem& operator=(const EventSystem&) = delete;
    EventSystem& operator=(EventSystem&&) noexcept = default;

  private:
    void ProcessEvents();

  private:
    struct Listener {
      const void* instance = nullptr;
      std::function<void(Event&)> callback;
    };

    std::map<EventType, std::vector<Event*>> m_Events;
    std::map<EventType, std::vector<Listener>> m_EventListeners;
  };

  template <typename T> requires std::is_base_of_v<Event, T>
  void EventSystem::Emit(const T& event) {
    auto it = m_EventListeners.find(typeid(T));
    if (it != m_EventListeners.end()) {
      for (const Listener& listener : it->second) {
        listener.callback(event);
      }
    }
  }

  template <typename T> requires std::is_base_of_v<Event, T>
  void EventSystem::AddListener(const void* instance, const std::function<void(T&)>& callback) {
    m_EventListeners[typeid(T)].emplace_back(instance,
      [&callback](Event& event) { callback(static_cast<T&>(event)); }
    );
  }

  template <typename T> requires std::is_base_of_v<Event, T>
  void EventSystem::RemoveListener(const void* instance) {
    std::erase_if(m_EventListeners[typeid(T)], [instance](const Listener& listener) {
      return listener.instance == instance;
    });
  }
}