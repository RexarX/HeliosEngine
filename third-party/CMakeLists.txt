add_subdirectory(ctti)

add_library(helios::ctti ALIAS ctti)

# Note: Installation is handled by cmake/Install.cmake to avoid duplicate exports
