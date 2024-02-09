project(Glad)

set(GLAD "include/glad/glad.h" "include/KHR/khrplatform.h" "src/glad.c")

add_library(glad STATIC ${GLAD})

target_include_directories(glad PUBLIC include/)