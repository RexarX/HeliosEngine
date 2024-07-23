#pragma once

#include "EntityComponentSystem/Systems/System.h"

namespace Engine
{
  struct Listener
  {
    void* instance;
    std::function<void(Event&)> callback;
    uint32_t id;
  };

  template<typename T>
  struct EventQueue
  {
    std::queue<T> events;
  };

  class VOXELENGINE_API EventSystem : public System
  {
  public:
    EventSystem() = default;
    virtual ~EventSystem() = default;

    std::shared_ptr<System> Clone() const override
    {
      return std::make_shared<EventSystem>(*this);
    }

    void OnUpdate(ECSManager& ecs, const Timestep deltaTime) override;
    void OnEvent(ECSManager& ecs, Event& event) override;

    template<typename T>
    void Emit(T& event);

    template<typename T>
    void AddListener(void* instance, const std::function<void(T&)>& callback);

    template<typename T>
    void RemoveListener(void* instance);

    template<typename T>
    void PushEvent(T& event);

    void ProcessQueuedEvents();

  private:
    template<typename T>
    EventQueue<T>& GetEventQueue();

  private:
    std::unordered_map<std::type_index, std::vector<std::byte>> m_EventQueues;
    std::unordered_map<std::type_index, std::vector<Listener>> m_EventListeners;
    uint32_t m_NextListenerId = 0;
  };

  template<typename T>
  void EventSystem::Emit(T& event)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    auto it = m_EventListeners.find(typeid(T));
    if (it != m_EventListeners.end()) {
      for (const auto& listener : it->second) {
        listener.callback(event);
      }
    }
  }

  template<typename T>
  void EventSystem::AddListener(void* instance, const std::function<void(T&)>& callback)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    m_EventListeners[typeid(T)].push_back(
      Listener{
        .instance = instance,
        .callback = [callback](Event& e) { callback(static_cast<T&>(e)); },
        .id = m_NextListenerId++
      }
    );
  }

  template<typename T>
  void EventSystem::RemoveListener(void* instance)
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

  template<typename T>
  void EventSystem::PushEvent(T& event)
  {
    static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event!");

    auto& queue = GetEventQueue<T>();
    queue.events.push(event);
  }

  template<typename T>
  EventQueue<T>& EventSystem::GetEventQueue()
  {
    auto it = m_EventQueues.find(typeid(T));
    if (it == m_EventQueues.end()) {
      auto [insertedIt, success] = m_EventQueues.emplace(
        typeid(T),
        std::vector<std::byte>(sizeof(EventQueue<T>))
      );
      it = insertedIt;
      new (it->second.data()) EventQueue<T>();
    }
    return *reinterpret_cast<EventQueue<T>*>(it->second.data());
  }
}