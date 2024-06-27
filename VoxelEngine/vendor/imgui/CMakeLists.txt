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
    "${CMAKE_CURRENT_SOURCE_DIR};"
    "$ENV{VULKAN_SDK}/Include;"
)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(imgui PRIVATE
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
    target_link_options(imgui PRIVATE
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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(imgui PRIVATE
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

target_link_libraries(imgui PRIVATE
    glad
    glfw
    "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib"
)