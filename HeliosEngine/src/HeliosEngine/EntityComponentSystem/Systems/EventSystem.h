#pragma once

#include "Events/Event.h"

#include "HeliosEngine/Timestep.h"

#include <entt/entt.hpp>

namespace Helios
{
  struct Listener
  {
    std::weak_ptr<void> instance;
    std::function<void(Event&)> callback;
  };

  class HELIOSENGINE_API EventSystem
  {
  public:
    EventSystem() = default;
    ~EventSystem() = default;

    void OnUpdate(entt::registry& registry);

    template <typename T>
    void Emit(T& event);

    template <typename T>
    void AddListener(const std::shared_ptr<void>& instance, const std::function<void(T&)>& callback);

    template <typename T>
    void RemoveListener(const std::shared_ptr<void>& instance);

    template <typename T>
    void PushEvent(T& event);

  private:
    void ProcessQueuedEvents();

  private:
    std::map<std::type_index, std::queue<std::reference_wrapper<Event>>> m_EventQueues;
    std::map<std::type_index, std::vector<Listener>> m_EventListeners;
  };

  template <typename T>
  void EventSystem::Emit(T& event)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    auto it = m_EventListeners.find(typeid(T));
    if (it != m_EventListeners.end()) {
      for (const Listener& listener : it->second) {
        listener.callback(event);
      }
    }
  }

  template <typename T>
  void EventSystem::AddListener(const std::shared_ptr<void>& instance, const std::function<void(T&)>& callback)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    m_EventListeners[typeid(T)].push_back(
      Listener{
        .instance = instance,
        .callback = [callback](Event& e) { callback(static_cast<T&>(e)); },
      }
    );
  }

  template <typename T>
  void EventSystem::RemoveListener(const std::shared_ptr<void>& instance)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    auto it = m_EventListeners.find(typeid(T));
    if (it != m_EventListeners.end()) {
      it->second.erase(
        std::remove_if(
          it->second.begin(), it->second.end(),
          [instance](const auto& listener) {
            return listener.instance == instance;
          }
        ),
        it->second.end()
      );
    }
  }

  template <typename T>
  void EventSystem::PushEvent(T& event)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");
    m_EventQueues[typeid(T)].push(event);
  }
}