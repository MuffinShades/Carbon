#create le project
cmake_minimum_required(VERSION 3.15)
project(carbon VERSION 0.0.0 LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 20)

#adds all the needed C, CPP, HPP and H files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
    gl_lib/glad/include/glad/glad.h
    gl_lib/glad/include/KHR/khrplatform.h
    gl_lib/glad/src/glad.c
)

add_executable(carbon ${SOURCES})

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

#libs
target_include_directories(carbon PUBLIC 
   ${PROJECT_SOURCE_DIR}/gl_lib/glfw/include 
   ${PROJECT_SOURCE_DIR}/gl_lib/glad/include/glad
   ${PROJECT_SOURCE_DIR}/gl_lib/glad/include/KHR
)

if (WIN32) #Windows
    find_package(OpenGL REQUIRED)

    add_compile_definitions(WIN32)
    target_link_libraries(carbon PUBLIC
       ${PROJECT_SOURCE_DIR}/gl_lib/glfw/glfw3dll.lib
       OpenGL::GL
    )
else (WIN32) #UNIX / Mac
    add_compile_definitions(MAC)
    target_link_libraries(carbon PUBLIC
        ${PROJECT_SOURCE_DIR}/gl_lib/glfw/lib-arm64/libglfw3.a
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
    )
endif (WIN32)

include(CTest)
enable_testing()