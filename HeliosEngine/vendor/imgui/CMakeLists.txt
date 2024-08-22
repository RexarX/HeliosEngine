project(Imgui)

file(GLOB_RECURSE imgui__src
    "*.h"
    "*.cpp"
)

file(GLOB_RECURSE imgui__backends
    "backends/*.h"
    "backends/*.cpp"
)

file(GLOB_RECURSE imgui__misc
    "misc/cpp/*.h" "misc/cpp/*.cpp"
    "misc/fonts/*.h" "misc/fonts/*.cpp"
)

set(IMGUI 
	${imgui__src}
    ${imgui__backends}
    ${imgui__misc}
)

add_library(imgui STATIC ${IMGUI})

target_include_directories(imgui PUBLIC
    "${CMAKE_INCLUDE_CURRENT_DIR};"
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS imgui)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Dist>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Dist>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(imgui PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_compile_options(imgui PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        $<$<CONFIG:Dist>:/O2 /Oi /Gy>
        /W3
    >
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Dist>:-O3>
        -Wno-switch
    >
)

find_package(Vulkan REQUIRED)

target_link_libraries(imgui PRIVATE
    glfw
    Vulkan::Vulkan
)