set(PROJECT_NAME VoxelEngine)

file(GLOB_RECURSE src
    "src/*.cpp"
    "src/*.h"
)

file(GLOB_RECURSE src__VoxelEngine
    "src/VoxelEngine/*.cpp"
    "src/VoxelEngine/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Events
    "src/VoxelEngine/Events*.cpp"
    "src/VoxelEngine/Events*.h"
)

file(GLOB_RECURSE src__EntityComponentSystem
    "src/VoxelEngine/EntityComponentSystem/*.h"

    "src/VoxelEngine/EntityComponentSystem/Components/*.cpp"
    "src/VoxelEngine/EntityComponentSystem/Components/*.h"

    "src/VoxelEngine/EntityComponentSystem/Entity/*.cpp"
    "src/VoxelEngine/EntityComponentSystem/Entity/*.h"

    "src/VoxelEngine/EntityComponentSystem/Manager/*.cpp"
    "src/VoxelEngine/EntityComponentSystem/Manager/*.h"

    "src/VoxelEngine/EntityComponentSystem/Systems/*.cpp"
    "src/VoxelEngine/EntityComponentSystem/Systems/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Scene
    "src/VoxelEngine/Scene/*.cpp"
    "src/VoxelEngine/Scene/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Utils
    "src/VoxelEngine/Utils/*.cpp"
    "src/VoxelEngine/Utils/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__Renderer__Vulkan
    "src/VoxelEngine/Renderer/Vulkan/*.cpp"
    "src/VoxelEngine/Renderer/Vulkan/*.h"
)

file(GLOB_RECURSE src__Platform__Windows
    "src/Platform/Windows/*.cpp"
    "src/Platform/Windows/*.h"
)

file(GLOB_RECURSE src__VoxelEngine__ImGui
    "src/VoxelEngine/ImGui/*.cpp"
    "src/VoxelEngine/ImGui/*.h"
)

set(ALL_FILES
    ${src}
    ${src__VoxelEngine}
    ${src__VoxelEngine__Events}
    ${src__EntityComponentSystem}
	${src__VoxelEngine__Scene}
    ${src__VoxelEngine__Utils}
    ${src__VoxelEngine__Renderer__Vulkan}
    ${src__Platform__Windows}
    ${src__VoxelEngine__ImGui}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h>"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} UPOUTPUTCONFIG)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_${UPOUTPUTCONFIG} "VoxelEngine"
        ARCHIVE_OUTPUT_NAME_${UPOUTPUTCONFIG} "VoxelEngine"
        RUNTIME_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
        ARCHIVE_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
    )
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

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
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/VoxelEngine;"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "$ENV{VULKAN_SDK}/Include;"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        "DEBUG_MODE;"
    >
    $<$<CONFIG:Release>:
        "RELEASE_MODE;"
    >
    $<$<CONFIG:Dist>:
        "DIST_MODE;"
    >
    "GLFW_INCLUDE_NONE;"
    "UNICODE;"
    "_UNICODE;"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE;"
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_WINDOWS;" PRIVATE "BUILD_DLL;")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CONFIG:Debug>:
			-O0;
			-g;
		>
		$<$<CONFIG:Release>:
			-O3;
		>
		$<$<CONFIG:Dist>:
			-O3;
		>
		-Wno-switch
	)
endif()

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    imgui
    stb_image
    objloader
    Vulkan::Vulkan
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib"
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LIBRARY_DEPENDENCIES})