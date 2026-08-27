#include <pch.hpp>

#include <helios/sdl3/window/event_handlers.hpp>

#include <helios/ecs/world.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/window/event_handlers.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/settings.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <cstdint>
#include <optional>
#include <string>

namespace helios::sdl3::window {

namespace {

[[nodiscard]] auto EntityForWindowEvent(ecs::World& world,
                                        SDL_WindowID window_id)
    -> std::optional<ecs::Entity> {
  auto* window_map = world.TryReadResource<WindowMap>();
  if (window_map == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  const WindowMap::Entry* entry = window_map->TryGetByWindowId(window_id);
  if (entry == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  return entry->entity;
}

[[nodiscard]] auto DisplayIndex(SDL_DisplayID display_id)
    -> std::optional<::helios::window::MonitorId> {
  int count = 0;
  SDL_DisplayID* displays = SDL_GetDisplays(&count);
  if (displays == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  for (int index = 0; index < count; ++index) {
    if (displays[index] == display_id) {
      SDL_free(displays);
      return static_cast<::helios::window::MonitorId>(index);
    }
  }

  SDL_free(displays);
  return std::nullopt;
}

void HandleWindowEvent(const SDL_Event& event, ecs::World& world) {
  const auto entity = EntityForWindowEvent(world, event.window.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  switch (event.type) {
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.width = static_cast<uint32_t>(event.window.data1);
        component->properties.height =
            static_cast<uint32_t>(event.window.data2);
      }
      world.WriteMessages<::helios::window::ResizedMsg>().Write({
          .entity = *entity,
          .width = static_cast<uint32_t>(event.window.data1),
          .height = static_cast<uint32_t>(event.window.data2),
      });
      break;

    case SDL_EVENT_WINDOW_RESIZED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.client_width =
            static_cast<uint32_t>(event.window.data1);
        component->properties.client_height =
            static_cast<uint32_t>(event.window.data2);
      }
      world.WriteMessages<::helios::window::ClientResizedMsg>().Write({
          .entity = *entity,
          .width = static_cast<uint32_t>(event.window.data1),
          .height = static_cast<uint32_t>(event.window.data2),
      });
      break;

    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
      if (auto* window_map = world.TryWriteResource<WindowMap>();
          window_map != nullptr) {
        WindowMap::Entry* map_entry =
            window_map->TryGetByWindowId(event.window.windowID);
        if (map_entry != nullptr && map_entry->window != nullptr) {
          SDL_Window* sdl_window = map_entry->window;
          const float scale = SDL_GetWindowDisplayScale(sdl_window);
          if (auto* component =
                  world.TryWriteComponent<::helios::window::Window>(*entity)) {
            component->properties.content_scale_x = scale;
            component->properties.content_scale_y = scale;
          }
          world.WriteMessages<::helios::window::ContentScaleChangedMsg>().Write(
              {
                  .entity = *entity,
                  .scale_x = scale,
                  .scale_y = scale,
              });
        }
      }
      break;

    case SDL_EVENT_WINDOW_MOVED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.pos_x = event.window.data1;
        component->properties.pos_y = event.window.data2;
      }
      world.WriteMessages<::helios::window::PosChangedMsg>().Write(
          {.entity = *entity,
           .x = event.window.data1,
           .y = event.window.data2});
      break;

    case SDL_EVENT_WINDOW_SHOWN:
    case SDL_EVENT_WINDOW_RESTORED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.visible = true;
      }
      world.WriteMessages<::helios::window::VisibilityChangedMsg>().Write(
          {.entity = *entity, .visible = true});
      break;

    case SDL_EVENT_WINDOW_HIDDEN:
    case SDL_EVENT_WINDOW_MINIMIZED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.visible = false;
      }
      world.WriteMessages<::helios::window::VisibilityChangedMsg>().Write(
          {.entity = *entity, .visible = false});
      break;

    case SDL_EVENT_WINDOW_MAXIMIZED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.maximized = true;
      }
      world.WriteMessages<::helios::window::MaximizedChangedMsg>().Write(
          {.entity = *entity, .maximized = true});
      break;

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.focused = true;
      }
      if (auto* backend = world.TryWriteResource<Context>()) {
        backend->clipboard_dirty = true;
      }
      world.WriteMessages<::helios::window::FocusChangedMsg>().Write(
          {.entity = *entity, .focused = true});
      break;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.focused = false;
      }
      world.WriteMessages<::helios::window::FocusChangedMsg>().Write(
          {.entity = *entity, .focused = false});
      break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      world.WriteMessages<::helios::window::CloseRequestedMsg>().Write(
          {.entity = *entity});
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->RequestClose();
      }
      break;

    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.hovered = true;
      }
      world.WriteMessages<::helios::window::HoverChangedMsg>().Write(
          {.entity = *entity, .hovered = true});
      break;

    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      if (auto* component =
              world.TryWriteComponent<::helios::window::Window>(*entity)) {
        component->properties.hovered = false;
      }
      world.WriteMessages<::helios::window::HoverChangedMsg>().Write(
          {.entity = *entity, .hovered = false});
      break;

    default:
      break;
  }
}

void HandleDropEvent(const SDL_Event& event, ecs::World& world) {
  if (event.type != SDL_EVENT_DROP_FILE) {
    return;
  }

  const auto entity = EntityForWindowEvent(world, event.drop.windowID);
  if (!entity.has_value() || event.drop.data == nullptr) [[unlikely]] {
    return;
  }

  world.WriteMessages<::helios::window::DroppedFilesMsg>().Write({
      .entity = *entity,
      .paths = {std::string(event.drop.data)},
  });
}

void HandleDisplayEvent(const SDL_Event& event, ecs::World& world) {
  if (event.type < SDL_EVENT_DISPLAY_FIRST ||
      event.type > SDL_EVENT_DISPLAY_LAST) {
    return;
  }

  std::optional<::helios::window::MonitorId> monitor_id;
  if (event.type == SDL_EVENT_DISPLAY_REMOVED) {
    monitor_id = DisplayIndex(event.display.displayID);
  }

  if (auto* layout = world.TryWriteResource<::helios::window::Monitors>();
      layout != nullptr) {
    RefreshMonitors(*layout);
  }

  if (event.type == SDL_EVENT_DISPLAY_ADDED) {
    monitor_id = DisplayIndex(event.display.displayID);
  }

  if (monitor_id.has_value()) {
    if (event.type == SDL_EVENT_DISPLAY_ADDED) {
      world.WriteMessages<::helios::window::MonitorConnectedMsg>().Write(
          {.index = *monitor_id});
    } else if (event.type == SDL_EVENT_DISPLAY_REMOVED) {
      world.WriteMessages<::helios::window::MonitorDisconnectedMsg>().Write(
          {.index = *monitor_id});
    }
  }

  if (auto* native = world.TryWriteResource<NativeWindows>();
      native != nullptr) {
    MarkMonitorDependentDirty(world, *native);
  }
}

void HandleClipboardEvent(const SDL_Event& event, ecs::World& world) {
  if (event.type != SDL_EVENT_CLIPBOARD_UPDATE) {
    return;
  }

  if (auto* backend = world.TryWriteResource<Context>()) {
    backend->clipboard_dirty = true;
  }
}

void DispatchEvent(const SDL_Event& event, ecs::World& world) {
  if (event.type >= SDL_EVENT_WINDOW_FIRST &&
      event.type <= SDL_EVENT_WINDOW_LAST) {
    HandleWindowEvent(event, world);
    return;
  }

  if (event.type >= SDL_EVENT_DISPLAY_FIRST &&
      event.type <= SDL_EVENT_DISPLAY_LAST) {
    HandleDisplayEvent(event, world);
    return;
  }

  if (event.type == SDL_EVENT_DROP_FILE) {
    HandleDropEvent(event, world);
    return;
  }

  if (event.type == SDL_EVENT_CLIPBOARD_UPDATE) {
    HandleClipboardEvent(event, world);
  }
}

void HandleSdlEvent(const SDL_Event& event, ecs::World& world) {
  DispatchEvent(event, world);
}

}  // namespace

void RegisterEventHandlers(sdl3::EventDispatcher& dispatcher) {
  dispatcher.Register(HandleSdlEvent);
}

}  // namespace helios::sdl3::window
