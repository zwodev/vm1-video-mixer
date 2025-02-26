# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
set(TINYUSB ${PICO_SDK_PATH}/lib/tinyusb) # jj
set(LWIP ${PICO_SDK_PATH}/lib/lwip) # jj

message("MOIN MOIN") # jj
message(${TINYUSB}/hw/bsp/family_support.cmake) # jj
include(${TINYUSB}/hw/bsp/family_support.cmake) # jj

if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(20250224_PiPico-VSCodeExtension-Test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(20250224_PiPico-VSCodeExtension-Test
    main.c
    usb_descriptors.c # jj
)

pico_set_program_name(20250224_PiPico-VSCodeExtension-Test "20250224_PiPico-VSCodeExtension-Test")
pico_set_program_version(20250224_PiPico-VSCodeExtension-Test "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(20250224_PiPico-VSCodeExtension-Test 1)
pico_enable_stdio_usb(20250224_PiPico-VSCodeExtension-Test 0)

# Add the standard library to the build
target_link_libraries(20250224_PiPico-VSCodeExtension-Test
    pico_stdlib
    tinyusb_device # jj
    tinyusb_host # jj
)

# Add the standard include files to the build
target_include_directories(20250224_PiPico-VSCodeExtension-Test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${TINYUSB}/src # jj
    ${TINYUSB}/lib/networking # jj
    ${TINYUSB}/hw # jj
    ${LWIP}/src/include # jj
    ${LWIP}/src/include/ipv4 # jj
    ${LWIP}/src/include/lwip/apps # jj
)

pico_add_extra_outputs(20250224_PiPico-VSCodeExtension-Test)

# ====================================================================================
