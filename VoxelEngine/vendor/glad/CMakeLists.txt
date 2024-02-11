project(Glad)

set(GLAD "include/glad/glad.h" "include/KHR/khrplatform.h" "src/glad.c")

add_library(glad STATIC ${GLAD})

set_property(TARGET glad PROPERTY C_STANDARD 17)

target_include_directories(glad PUBLIC include/)