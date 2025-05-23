cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(smart_light C CXX ASM)
pico_sdk_init()

add_executable(smart_light smart_light.c )

pico_generate_pio_header(smart_light ${CMAKE_CURRENT_LIST_DIR}/lib/ws2812.pio)

pico_set_program_name(smart_light "smart_light")
pico_set_program_version(smart_light "0.1")

pico_enable_stdio_uart(smart_light 1)
pico_enable_stdio_usb(smart_light 1)

target_link_libraries(smart_light
        pico_stdlib
        hardware_gpio
        pico_cyw43_arch_lwip_threadsafe_background)

target_include_directories(smart_light PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${PICO_SDK_PATH}/lib/lwip/src/include
        ${PICO_SDK_PATH}/lib/lwip/src/include/arch
        ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
)

pico_add_extra_outputs(smart_light)
