cmake_minimum_required(VERSION 3.0)
project(timer)

set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_C_FLAGS_RELEASE "$ENV${CFLAGS} -O3 -Wall")

include_directories(source)

add_subdirectory(source)
add_subdirectory(examples)

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

include (CTest)

add_test(example examples/example)

