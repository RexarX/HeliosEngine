set(PROJECT_NAME HeliosEngine)

file(GLOB_RECURSE src
    "src/*.h" "src/*.cpp"
)

file(GLOB_RECURSE src__Engine
    "src/HeliosEngine/*.h" "src/HeliosEngine/*.cpp"
)

file(GLOB_RECURSE src__Engine__Events
    "src/HeliosEngine/Events/*.h" "src/HeliosEngine/Events/*.cpp"
)

file(GLOB_RECURSE src__EntityComponentSystem
    "src/HeliosEngine/EntityComponentSystem/*.h"
    "src/HeliosEngine/EntityComponentSystem/*.inl"
    "src/HeliosEngine/EntityComponentSystem/*.cpp"
    "src/HeliosEngine/EntityComponentSystem/Systems/*.h"
    "src/HeliosEngine/EntityComponentSystem/Systems/*.cpp"
)

file(GLOB_RECURSE src__Engine__Scene
    "src/HeliosEngine/Scene/*.h" "src/HeliosEngine/Scene/*.cpp"
)

file(GLOB_RECURSE src__Engine__Utils
    "src/HeliosEngine/Utils/*.h" "src/HeliosEngine/Utils/*.cpp"
)

file(GLOB_RECURSE src__Engine__Renderer
    "src/HeliosEngine/Renderer/*.h" "src/HeliosEngine/Renderer/*.cpp"
    "src/HeliosEngine/Renderer/Vulkan/*.h" "src/HeliosEngine/Renderer/Vulkan/*.cpp"
)

file(GLOB_RECURSE src__Engine__ShaderGraph
    "src/HeliosEngine/ShaderGraph/*.h" "src/HeliosEngine/ShaderGraph/*.cpp"
)

file(GLOB_RECURSE src__Platform__Windows
    "src/Platform/Windows/*.h" "src/Platform/Windows/*.cpp"
)

file(GLOB_RECURSE src__Engine__ImGui
    "src/HeliosEngine/ImGui/*.h" "src/HeliosEngine/ImGui/*.cpp"
)

set(ALL_FILES
    ${src}
    ${src__Engine}
    ${src__Engine__Events}
    ${src__EntityComponentSystem}
    ${src__Engine__Scene}
    ${src__Engine__Utils}
    ${src__Engine__Renderer}
    ${src__Engine__ShaderGraph}
    ${src__Platform__Windows}
    ${src__Engine__ImGui}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 UNITY_BUILD ON)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h>"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} UPOUTPUTCONFIG)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_${UPOUTPUTCONFIG} "HeliosEngine"
        ARCHIVE_OUTPUT_NAME_${UPOUTPUTCONFIG} "HeliosEngine"
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
    $<$<CONFIG:RelWithDebInfo>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_include_directories(${PROJECT_NAME} PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/entt/single_include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/HeliosEngine;"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "$ENV{VULKAN_SDK}/Include;"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        "DEBUG_MODE;"
    >
    $<$<CONFIG:RelWithDebInfo>:
        "RELEASE_WITH_BEDUG_INFO_MODE;"
    >
    $<$<CONFIG:Release>:
        "RELEASE_MODE;"
    >
    "BUILD_SHARED;"
    "GLFW_INCLUDE_NONE;"
    "UNICODE;"
    "_UNICODE;"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE;"
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_WINDOWS;")
elseif(UNIX)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_LINUX;")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /fsanitize=address>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        /GL
        /W3
    >
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:RelWithDebInfo>:-O3>
        $<$<CONFIG:Release>:-O3>
        -flto
    >
)

if (ENABLE_SIMD_SSE2)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse2>
		    $<$<CONFIG:Release>:-msse2>   
	    >
    )
elseif (ENABLE_SIMD_SSE3)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>   
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse3>
		    $<$<CONFIG:Release>:-msse3> 
	    >
    )
elseif (ENABLE_SIMD_SSSE3)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSSE3>
			$<$<CONFIG:Release>:/arch:SSSE3>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mssse3>
			$<$<CONFIG:Release>:-mssse3>
		>
	)
elseif (ENABLE_SIMD_SSE4_1)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.1
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.1>
			$<$<CONFIG:Release>:/arch:SSE4.1>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse4.1>
			$<$<CONFIG:Release>:-msse4.1>	
		>
	)
elseif (ENABLE_SIMD_SSE4_2)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.2
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.2>
			$<$<CONFIG:Release>:/arch:SSE4.2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-msse4.2>
			$<$<CONFIG:RelWithDebInfo>:-msse4.2>
		>
	)
elseif (ENABLE_SIMD_AVX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX>
			$<$<CONFIG:Release>:/arch:AVX>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mavx>
			$<$<CONFIG:Release>:-mavx>
			
		>
    )
elseif (ENABLE_SIMD_AVX2)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
			$<$<CONFIG:Release>:/arch:AVX2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mavx2>
			$<$<CONFIG:Release>:-mavx2>
		>
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<BOOL:ENABLE_PROFILING>:"ENABLE_PROFILING">
)

find_package(Boost REQUIRED COMPONENTS uuid)
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    imgui
    stb_image
    objloader
    EnTT::EnTT
    Vulkan::Vulkan
    Boost::uuid
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib"
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LIBRARY_DEPENDENCIES})