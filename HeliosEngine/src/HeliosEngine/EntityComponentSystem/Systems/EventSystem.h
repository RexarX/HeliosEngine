#pragma once

#include "Events/Event.h"

#include <entt/entt.hpp>

namespace Helios
{
  using EventQueue = std::queue<std::unique_ptr<Event>>;

  class HELIOSENGINE_API EventSystem
  {
  public:
    EventSystem() = default;
    EventSystem(const EventSystem&) = delete;
    EventSystem(EventSystem&&) noexcept = default;
    ~EventSystem() = default;

    void OnUpdate() { ProcessQueuedEvents(); }

    template <typename T> requires std::is_base_of<Event, T>::value
    void Emit(const T& event);

    template <typename T> requires std::is_base_of<Event, T>::value
    void AddListener(const void* instance, const std::function<void(const T&)>& callback);

    template <typename T> requires std::is_base_of<Event, T>::value
    void RemoveListener(const void* instance);

    template <typename T> requires std::is_base_of<Event, T>::value
    void PushEvent(const T& event) {
      m_EventQueues[typeid(T)].push(std::make_unique<T>(event));
    }

    EventSystem& operator=(const EventSystem&) = delete;
    EventSystem& operator=(EventSystem&&) noexcept = default;

  private:
    void ProcessQueuedEvents();

  private:
    struct Listener
    {
      const void* instance = nullptr;
      std::function<void(const Event&)> callback;
    };

    std::map<std::type_index, EventQueue> m_EventQueues;
    std::map<std::type_index, std::vector<Listener>> m_EventListeners;
  };

  template <typename T> requires std::is_base_of<Event, T>::value
  void EventSystem::Emit(const T& event)
  {
    auto it = m_EventListeners.find(typeid(T));
    if (it != m_EventListeners.end()) {
      for (const Listener& listener : it->second) {
        listener.callback(event);
      }
    }
  }

  template <typename T> requires std::is_base_of<Event, T>::value
  void EventSystem::AddListener(const void* instance, const std::function<void(const T&)>& callback)
  {
    m_EventListeners[typeid(T)].emplace_back(instance,
      [callback](Event& event) { callback(static_cast<T&>(event)); }
    );
  }

  template <typename T> requires std::is_base_of<Event, T>::value
  void EventSystem::RemoveListener(const void* instance)
  {
    std::erase_if(m_EventListeners[typeid(T)], [instance](const Listener& listener) {
      return listener.instance == instance;
    });
  }
}