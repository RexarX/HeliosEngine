#pragma once

#include "pch.h"

#include <entt/entt.hpp>

namespace Helios {
  class HELIOSENGINE_API EventSystem {
  public:
    EventSystem() = default;
    EventSystem(const EventSystem&) = delete;
    EventSystem(EventSystem&&) noexcept = default;
    ~EventSystem() = default;

    void OnUpdate();

    template <EventTrait T>
    void Emit(const T& event);

    template <EventTrait T>
    void AddListener(const void* instance, std::function<void(T&)>&& callback);

    template <EventTrait T>
    void RemoveListener(const void* instance);

    template <EventTrait T>
    void PushEvent(const T& event);

    EventSystem& operator=(const EventSystem&) = delete;
    EventSystem& operator=(EventSystem&&) noexcept = default;

  private:
    void ProcessEvents();

  private:
    struct Listener {
      const void* instance = nullptr;
      std::function<void(Event&)> callback;
    };

    std::map<EventType, std::vector<std::byte>> m_Events;
    std::map<EventType, uint64_t> m_EventSizes;
    std::map<EventType, std::vector<Listener>> m_EventListeners;
  };

  template <EventTrait T>
  void EventSystem::Emit(const T& event) {
    auto it = m_EventListeners.find(T::GetStaticType());
    if (it != m_EventListeners.end()) {
      for (const Listener& listener : it->second) {
        listener.callback(event);
      }
    }
  }

  template <EventTrait T>
  void EventSystem::AddListener(const void* instance, std::function<void(T&)>&& callback) {
    m_EventListeners[T::GetStaticType()].emplace_back(
      instance,
      [callback = std::forward<std::function<void(T&)>>(callback)](Event& event) {
        callback(static_cast<T&>(event));
      }
    );
  }

  template <EventTrait T>
  void EventSystem::RemoveListener(const void* instance) {
    std::erase_if(m_EventListeners[T::GetStaticType()], [instance](const Listener& listener) {
      return listener.instance == instance;
    });
  }

  template <EventTrait T>
  void EventSystem::PushEvent(const T& event) {
    EventType type = T::GetStaticType();
    m_EventSizes[type] = sizeof(T);

    const std::byte* begin = reinterpret_cast<const std::byte*>(std::addressof(event));
    const std::byte* end = begin + sizeof(T);

    std::vector<std::byte>& events = m_Events[type];
    events.insert(events.cend(), begin, end);
  }
}