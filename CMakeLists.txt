cmake_minimum_required(VERSION 3.12...3.30)

# SDK setup must precede project() and only run for the standalone build.
# PROJECT_IS_TOP_LEVEL is not set until project(), so compare source dirs here.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(PICO_PLATFORM rp2350)
    set(PICO_BOARD pico2_w)

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()

    set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-sdk)
    include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
endif()

project(pio-i2s C CXX ASM)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 20)
    add_compile_options(-Wall)

    pico_sdk_init()
endif()

add_library(pio-i2s-lib STATIC
    src/pio-i2s.c
)

target_include_directories(pio-i2s-lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pio-i2s-lib PUBLIC
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
)

set_source_files_properties(src/pio-i2s.c PROPERTIES
    COMPILE_OPTIONS "-Wextra;-Wpedantic")

add_library(pio-i2s::lib ALIAS pio-i2s-lib)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(pio-i2s
        src/example/main.cpp
    )

    set_source_files_properties(src/example/main.cpp PROPERTIES
        COMPILE_OPTIONS "-Wextra;-Wpedantic")

    target_link_libraries(pio-i2s
        pio-i2s-lib
        pico_bootsel_via_double_reset
    )

    pico_add_extra_outputs(pio-i2s)

    add_subdirectory(test/device)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pio-i2s.uf2
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        DESTINATION .
    )

    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    set(CPACK_GENERATOR "ZIP" "TGZ")
    include(CPack)
endif()
