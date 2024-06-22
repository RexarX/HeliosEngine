project(Glad)

set(GLAD "include/glad/glad.h" "include/KHR/khrplatform.h" "src/glad.c")

add_library(glad STATIC ${GLAD})

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Dist>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Dist>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(glad PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

if(MSVC)
    target_compile_options(glad PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Zi;
        >
        $<$<CONFIG:Dist>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        /w;
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(glad PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG;
            /INCREMENTAL;
        >
        $<$<CONFIG:Dist>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        $<$<CONFIG:Release>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

target_include_directories(glad PUBLIC include/)