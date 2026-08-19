cmake_minimum_required(VERSION 3.16)

project(pio-i2s-host-tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_library(unity STATIC unity/unity.c)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity)

target_compile_definitions(unity PUBLIC UNITY_SUPPORT_64=1)

add_library(pio_i2s STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../src/pio-i2s.c)
target_include_directories(pio_i2s PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/host/mocks
)
target_compile_definitions(pio_i2s PUBLIC
    PIOI2S_TEST_HOST=1
    PioI2S_ZERO_ON_UNDERRUN=1
)
target_compile_options(pio_i2s PRIVATE
    -Wall -Wextra -Wpedantic
    -fsanitize=address,undefined
    -fno-sanitize-recover=all
)

add_executable(pio_i2s_host_tests
    host/main.c
    host/mocks/mocks.c
    shared/test-support.c
)

target_include_directories(pio_i2s_host_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/host
    ${CMAKE_CURRENT_SOURCE_DIR}/host/mocks
)

target_compile_options(pio_i2s_host_tests PRIVATE
    -Wall -Wextra -Wpedantic
    -fsanitize=address,undefined
    -fno-sanitize-recover=all
)

target_link_options(pio_i2s_host_tests PRIVATE
    -fsanitize=address,undefined
)

target_link_libraries(pio_i2s_host_tests PRIVATE pio_i2s unity m)

enable_testing()
add_test(NAME pio_i2s_host_tests COMMAND pio_i2s_host_tests)
