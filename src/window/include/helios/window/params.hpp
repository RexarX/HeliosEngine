#pragma once

#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param_traits.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>

namespace helios::window {

/// @brief Mutable query over all `Window` components.
struct Windows {
  ecs::Query<Window&> query;
};

/// @brief Read-only query over all `Window` components.
struct WindowsView {
  ecs::Query<const Window&> query;
};

/// @brief Mutable query over primary windows.
struct PrimaryWindows {
  ecs::Query<Window&, ecs::With<Primary>> query;
};

/// @brief Mutable query over primary windows.
struct PrimaryWindowsView {
  ecs::Query<const Window&, ecs::With<Primary>> query;
};

/// @brief Window create / close / failure message readers.
struct LifecycleMessages {
  ecs::MessageReader<CreatedMsg> created;
  ecs::MessageReader<ClosedMsg> closed;
  ecs::MessageReader<CloseRequestedMsg> close_requested;
  ecs::MessageReader<CreationFailedMsg> failed;
};

/// @brief Framebuffer, client-area, scale, and position message readers.
struct GeometryMessages {
  ecs::MessageReader<ResizedMsg> resized;
  ecs::MessageReader<ClientResizedMsg> client_resized;
  ecs::MessageReader<ContentScaleChangedMsg> content_scale;
  ecs::MessageReader<PosChangedMsg> pos;
};

/// @brief Chrome and presentation message readers.
struct AppearanceMessages {
  ecs::MessageReader<ModeChangedMsg> mode;
  ecs::MessageReader<CursorModeChangedMsg> cursor_mode;
  ecs::MessageReader<VisibilityChangedMsg> visibility;
  ecs::MessageReader<FocusChangedMsg> focus;
  ecs::MessageReader<MaximizedChangedMsg> maximized;
  ecs::MessageReader<IconChangedMsg> icon;
  ecs::MessageReader<ResizableChangedMsg> resizable;
  ecs::MessageReader<DecoratedChangedMsg> decorated;
  ecs::MessageReader<OpacityChangedMsg> opacity;
  ecs::MessageReader<FloatingChangedMsg> floating;
  ecs::MessageReader<HoverChangedMsg> hover;
  ecs::MessageReader<MousePassthroughChangedMsg> mouse_passthrough;
};

/// @brief Clipboard, file-drop, and monitor message readers.
struct PlatformMessages {
  ecs::MessageReader<ClipboardChangedMsg> clipboard;
  ecs::MessageReader<DroppedFilesMsg> dropped_files;
  ecs::MessageReader<MonitorsChangedMsg> monitors;
};

/// @brief All window message readers.
struct Messages {
  LifecycleMessages lifecycle;
  GeometryMessages geometry;
  AppearanceMessages appearance;
  PlatformMessages platform;
};

/// @brief Window create / close / failure message writers.
struct LifecycleWriters {
  ecs::MessageWriter<CreatedMsg> created;
  ecs::MessageWriter<ClosedMsg> closed;
  ecs::MessageWriter<CloseRequestedMsg> close_requested;
  ecs::MessageWriter<CreationFailedMsg> failed;
};

/// @brief Framebuffer, client-area, scale, and position message writers.
struct GeometryWriters {
  ecs::MessageWriter<ResizedMsg> resized;
  ecs::MessageWriter<ClientResizedMsg> client_resized;
  ecs::MessageWriter<ContentScaleChangedMsg> content_scale;
  ecs::MessageWriter<PosChangedMsg> pos;
};

/// @brief Chrome and presentation message writers.
struct AppearanceWriters {
  ecs::MessageWriter<ModeChangedMsg> mode;
  ecs::MessageWriter<CursorModeChangedMsg> cursor_mode;
  ecs::MessageWriter<VisibilityChangedMsg> visibility;
  ecs::MessageWriter<FocusChangedMsg> focus;
  ecs::MessageWriter<MaximizedChangedMsg> maximized;
  ecs::MessageWriter<IconChangedMsg> icon;
  ecs::MessageWriter<ResizableChangedMsg> resizable;
  ecs::MessageWriter<DecoratedChangedMsg> decorated;
  ecs::MessageWriter<OpacityChangedMsg> opacity;
  ecs::MessageWriter<FloatingChangedMsg> floating;
  ecs::MessageWriter<HoverChangedMsg> hover;
  ecs::MessageWriter<MousePassthroughChangedMsg> mouse_passthrough;
};

/// @brief Clipboard, file-drop, and monitor message writers.
struct PlatformWriters {
  ecs::MessageWriter<ClipboardChangedMsg> clipboard;
  ecs::MessageWriter<DroppedFilesMsg> dropped_files;
  ecs::MessageWriter<MonitorsChangedMsg> monitors;
};

/// @brief All window message writers.
struct Writers {
  LifecycleWriters lifecycle;
  GeometryWriters geometry;
  AppearanceWriters appearance;
  PlatformWriters platform;
};

/// @brief Writers used when a backend creates a native window.
struct CreationWriters {
  ecs::MessageWriter<CreatedMsg> created;
  ecs::MessageWriter<ContentScaleChangedMsg> content_scale;
  ecs::MessageWriter<CreationFailedMsg> failed;
};

}  // namespace helios::window

namespace helios::ecs {

template <>
struct SystemParamTraits<helios::window::Windows>
    : CompositeSystemParam<helios::window::Windows,
                           Query<helios::window::Window&>> {};

template <>
struct SystemParamTraits<helios::window::WindowsView>
    : CompositeSystemParam<helios::window::WindowsView,
                           Query<const helios::window::Window&>> {};

template <>
struct SystemParamTraits<helios::window::PrimaryWindows>
    : CompositeSystemParam<
          helios::window::PrimaryWindows,
          Query<helios::window::Window&, With<helios::window::Primary>>> {};

template <>
struct SystemParamTraits<helios::window::PrimaryWindowsView>
    : CompositeSystemParam<
          helios::window::PrimaryWindowsView,
          Query<const helios::window::Window&, With<helios::window::Primary>>> {
};

template <>
struct SystemParamTraits<helios::window::LifecycleMessages>
    : CompositeSystemParam<helios::window::LifecycleMessages,
                           MessageReader<helios::window::CreatedMsg>,
                           MessageReader<helios::window::ClosedMsg>,
                           MessageReader<helios::window::CloseRequestedMsg>,
                           MessageReader<helios::window::CreationFailedMsg>> {};

template <>
struct SystemParamTraits<helios::window::GeometryMessages>
    : CompositeSystemParam<
          helios::window::GeometryMessages,
          MessageReader<helios::window::ResizedMsg>,
          MessageReader<helios::window::ClientResizedMsg>,
          MessageReader<helios::window::ContentScaleChangedMsg>,
          MessageReader<helios::window::PosChangedMsg>> {};

template <>
struct SystemParamTraits<helios::window::AppearanceMessages>
    : CompositeSystemParam<
          helios::window::AppearanceMessages,
          MessageReader<helios::window::ModeChangedMsg>,
          MessageReader<helios::window::CursorModeChangedMsg>,
          MessageReader<helios::window::VisibilityChangedMsg>,
          MessageReader<helios::window::FocusChangedMsg>,
          MessageReader<helios::window::MaximizedChangedMsg>,
          MessageReader<helios::window::IconChangedMsg>,
          MessageReader<helios::window::ResizableChangedMsg>,
          MessageReader<helios::window::DecoratedChangedMsg>,
          MessageReader<helios::window::OpacityChangedMsg>,
          MessageReader<helios::window::FloatingChangedMsg>,
          MessageReader<helios::window::HoverChangedMsg>,
          MessageReader<helios::window::MousePassthroughChangedMsg>> {};

template <>
struct SystemParamTraits<helios::window::PlatformMessages>
    : CompositeSystemParam<helios::window::PlatformMessages,
                           MessageReader<helios::window::ClipboardChangedMsg>,
                           MessageReader<helios::window::DroppedFilesMsg>,
                           MessageReader<helios::window::MonitorsChangedMsg>> {
};

template <>
struct SystemParamTraits<helios::window::Messages>
    : CompositeSystemParam<
          helios::window::Messages, helios::window::LifecycleMessages,
          helios::window::GeometryMessages, helios::window::AppearanceMessages,
          helios::window::PlatformMessages> {};

template <>
struct SystemParamTraits<helios::window::LifecycleWriters>
    : CompositeSystemParam<helios::window::LifecycleWriters,
                           MessageWriter<helios::window::CreatedMsg>,
                           MessageWriter<helios::window::ClosedMsg>,
                           MessageWriter<helios::window::CloseRequestedMsg>,
                           MessageWriter<helios::window::CreationFailedMsg>> {};

template <>
struct SystemParamTraits<helios::window::GeometryWriters>
    : CompositeSystemParam<
          helios::window::GeometryWriters,
          MessageWriter<helios::window::ResizedMsg>,
          MessageWriter<helios::window::ClientResizedMsg>,
          MessageWriter<helios::window::ContentScaleChangedMsg>,
          MessageWriter<helios::window::PosChangedMsg>> {};

template <>
struct SystemParamTraits<helios::window::AppearanceWriters>
    : CompositeSystemParam<
          helios::window::AppearanceWriters,
          MessageWriter<helios::window::ModeChangedMsg>,
          MessageWriter<helios::window::CursorModeChangedMsg>,
          MessageWriter<helios::window::VisibilityChangedMsg>,
          MessageWriter<helios::window::FocusChangedMsg>,
          MessageWriter<helios::window::MaximizedChangedMsg>,
          MessageWriter<helios::window::IconChangedMsg>,
          MessageWriter<helios::window::ResizableChangedMsg>,
          MessageWriter<helios::window::DecoratedChangedMsg>,
          MessageWriter<helios::window::OpacityChangedMsg>,
          MessageWriter<helios::window::FloatingChangedMsg>,
          MessageWriter<helios::window::HoverChangedMsg>,
          MessageWriter<helios::window::MousePassthroughChangedMsg>> {};

template <>
struct SystemParamTraits<helios::window::PlatformWriters>
    : CompositeSystemParam<helios::window::PlatformWriters,
                           MessageWriter<helios::window::ClipboardChangedMsg>,
                           MessageWriter<helios::window::DroppedFilesMsg>,
                           MessageWriter<helios::window::MonitorsChangedMsg>> {
};

template <>
struct SystemParamTraits<helios::window::Writers>
    : CompositeSystemParam<
          helios::window::Writers, helios::window::LifecycleWriters,
          helios::window::GeometryWriters, helios::window::AppearanceWriters,
          helios::window::PlatformWriters> {};

template <>
struct SystemParamTraits<helios::window::CreationWriters>
    : CompositeSystemParam<
          helios::window::CreationWriters,
          MessageWriter<helios::window::CreatedMsg>,
          MessageWriter<helios::window::ContentScaleChangedMsg>,
          MessageWriter<helios::window::CreationFailedMsg>> {};

}  // namespace helios::ecs
