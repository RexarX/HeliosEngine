# RPG Text Game Example

include(TargetUtils)
include(SimdUtils)

set(RPG_EXAMPLE_NAME rpg_text_game)

# Source files
set(RPG_SOURCES
    src/main.cpp
)

# Header files (for IDE visibility)
set(RPG_HEADERS
    include/components.hpp
    include/events.hpp
    include/modules.hpp
    include/resources.hpp
    include/systems.hpp
)

# Create executable
add_executable(${RPG_EXAMPLE_NAME}
    ${RPG_SOURCES}
    ${RPG_HEADERS}
)

# Apply standard configurations using modular helpers
helios_target_set_cxx_standard(${RPG_EXAMPLE_NAME} STANDARD 23)
helios_target_set_platform(${RPG_EXAMPLE_NAME})
helios_target_set_optimization(${RPG_EXAMPLE_NAME})
helios_simd_apply_to_target(TARGET ${RPG_EXAMPLE_NAME})
helios_target_set_warnings(${RPG_EXAMPLE_NAME})
helios_target_set_output_dirs(${RPG_EXAMPLE_NAME} CUSTOM_FOLDER examples/rpg_text_game)
helios_target_set_folder(${RPG_EXAMPLE_NAME} "Helios/Examples/RPGTextGame")

# Include directories
target_include_directories(${RPG_EXAMPLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link to helios core
target_link_libraries(${RPG_EXAMPLE_NAME} PRIVATE
    helios::core
)

# Enable LTO if configured
if(HELIOS_ENABLE_LTO AND HELIOS_IPO_SUPPORTED)
    set_property(TARGET ${RPG_EXAMPLE_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

message(STATUS "Added example: ${RPG_EXAMPLE_NAME}")
