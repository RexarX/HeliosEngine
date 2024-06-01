set(PROJECT_NAME VoxelEngine)

file(GLOB_RECURSE src
    "src/*.cpp"
    "src/*.h"
)

file(GLOB_RECURSE src__Platform__OpenGL
    "src/Platform/OpenGL/*.cpp"
    "src/Platform/OpenGL/*.h"
)

file(GLOB_RECURSE src__Platform__Vulkan
    "src/Platform/Vulkan/*.cpp"
    "src/Platform/Vulkan/*.h"
)

file(GLOB_RECURSE src__Platform__Windows
    "src/Platform/Windows/*.cpp"
    "src/Platform/Windows/*.h"
)

file(GLOB_RECURSE src__VoxelEngine
    "src/VoxelEngine/*.cpp"
    "src/VoxelEngine/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Events
    "src/VoxelEngine/Events/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Render
    "src/VoxelEngine/Render/*.cpp"
    "src/VoxelEngine/Render/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__ImGui
    "src/VoxelEngine/ImGui/*.cpp"
    "src/VoxelEngine/ImGui/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Utils
    "src/VoxelEngine/Utils/*.cpp"
    "src/VoxelEngine/Utils/*.h"
)

set(ALL_FILES
    ${src}
    ${src__Platform__OpenGL}
    ${src__Platform__Vulkan}
    ${src__Platform__Windows}
    ${src__VoxelEngine}
    ${src__VoxelEngine__Events}
    ${src__VoxelEngine__Render}
    ${src__VoxelEngine__ImGui}
    ${src__VoxelEngine__Utils}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/vepch.h>"
)

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE VoxelEngine)

set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "VoxelEngine"
    TARGET_NAME_RELEASE "VoxelEngine"
    TARGET_NAME_DIST    "VoxelEngine"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64/"
    OUTPUT_DIRECTORY_DIST    "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Dist-windows-x86_64/"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64"
    ARCHIVE_OUTPUT_DIRECTORY_DIST    "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Dist-windows-x86_64"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_NAME_DEBUG   "VoxelEngine"
    ARCHIVE_OUTPUT_NAME_RELEASE "VoxelEngine"
    ARCHIVE_OUTPUT_NAME_DIST    "VoxelEngine"
)

get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Dist>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>,$<CONFIG:Dist>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image;"
    "$ENV{VULKAN_SDK}/Include;"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "VE_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "VE_RELEASE"
    ">"
    "$<$<CONFIG:Dist>:"
        "VE_DIST"
    ">"
    "VE_BUILD_DLL;"
    "GLFW_INCLUDE_NONE;"
    "UNICODE;"
    "_UNICODE;"
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "VE_PLATFORM_WINDOWS;")
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Zi;
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy;
        >
        $<$<CONFIG:Dist>:
            /O2;
            /Oi;
            /Gy;
        >
        /W3;
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG;
            /INCREMENTAL;
        >
         $<$<CONFIG:Release>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        $<$<CONFIG:Dist>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        /SUBSYSTEM:WINDOWS
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	glad
    glfw
    stb_image
    imgui
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "opengl32"
	"$ENV{VULKAN_SDK}/Lib/vulkan-1.lib"
    "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib"
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LIBRARY_DEPENDENCIES})