project(Stb_image)

set(STB_IMAGE "stb_image.h" "stb_image.cpp")

add_library(stb_image STATIC ${STB_IMAGE})

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
set_target_properties(stb_image PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(stb_image PRIVATE
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
    target_link_options(stb_image PRIVATE
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
	target_compile_options(stb_image PRIVATE
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