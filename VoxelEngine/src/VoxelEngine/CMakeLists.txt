set(APPLICATION application.h application.cpp)

set(CORE Core.h)

set(ENTRYPOINT EntryPoint.h)

set(INPUT Input.h)

set(KEYCODES KeyCodes.h)

set(LAYER Layer.h Layer.cpp)

set(LAYER_STACK LayerStack.h LayerStack.cpp)

set(LOG Log.cpp Log.h)

set(MOUSE_BUTTON_CODES MouseButtonCodes.h)

set(WINDOW Window.h)

set(LIBS ${APPLICATION} ${CORE} ${ENTRYPOINT} ${INPUT} ${KEYCODES} ${LAYER} ${LAYER_STACK} ${LOG} ${MOUSE_BUTTON_CODES} ${WINDOW} ${EVENTS} ${RENDERER})

add_library(VoxelEngine SHARED ${LIBS})

target_link_libraries(VoxelEngine spdlog::spdlog)

target_link_libraries(VoxelEngine glad)

target_include_directories(VoxelEngine PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS VoxelEngine)

add_subdirectory(Events)

add_subdirectory(Renderer)