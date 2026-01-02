add_subdirectory(ctti)

add_library(helios::ctti::ctti ALIAS ctti)

# Create helios::ctti convenience target that brings in all ctti targets
add_library(_helios_ctti_all INTERFACE)
target_link_libraries(_helios_ctti_all INTERFACE ctti)
add_library(helios::ctti ALIAS _helios_ctti_all)

# Note: Installation is handled by cmake/Install.cmake to avoid duplicate exports
