# The on-device unit tests for PioI2S.

set(TEST_NAME pio-i2s-tests)

add_executable(${TEST_NAME}
    main.c
    ../shared/test-support.c
    ../unity/unity.c
    ../../src/pio-i2s.c
)

set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(${TEST_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../unity
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# UNITY_SUPPORT_64 enables Unity's 64-bit assertions.
target_compile_definitions(${TEST_NAME} PRIVATE
    PIOI2S_TEST_DEVICE=1
    UNITY_SUPPORT_64=1
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/../unity/unity.c
    PROPERTIES COMPILE_OPTIONS "-w"
)

target_link_libraries(${TEST_NAME}
    pico_bootsel_via_double_reset
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
    pico_cyw43_arch_none
)

pico_generate_pio_header(${TEST_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/capture.pio
)

pico_add_extra_outputs(${TEST_NAME})

pico_enable_stdio_usb(${TEST_NAME} 1)
pico_enable_stdio_uart(${TEST_NAME} 0)
