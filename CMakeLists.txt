cmake_minimum_required(VERSION 3.21)

set(PICO_BOARD pico CACHE STRING "Board type")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Include PICO SDK 
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()


project(msc_pico_failure C CXX ASM) # TODO: change name


# initialize PICO SDK
pico_sdk_init()


# Project-specific configuration
add_compile_options(-Wall
        )

add_executable(${PROJECT_NAME}
	# Slightly modified sources from the Pico SDK
	src-override/pico-sdk/usb.cpp
	src-override/pico-sdk/reset_interface.c
	src-override/pico-sdk/pico/stdio_usb.h
	
	# mutated file, added msc
	src-override/pico-sdk/tusb_config.h

	# simplified descriptor
	desc.cpp

	# "Main" files
	msc.cpp
	main.cpp

)

# for copied files
target_include_directories(${PROJECT_NAME} PRIVATE 
	src-override/pico-sdk/
)
target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_ENTER_USB_BOOT_ON_EXIT=1)

pico_set_program_name(${PROJECT_NAME} "TestTINY")
pico_set_program_version(${PROJECT_NAME} "0.1")


# enable usb output, disable uart output, but do it on our terms, so disable both
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
if (TARGET tinyusb_device)
else()
	message(WARNING "Need TinyUSB")
endif()

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# https://github.com/raspberrypi/pico-sdk/issues/1029 
#target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--no-warn-rwx-segments")

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
	tinyusb_device
	hardware_uart
	hardware_exception

	
	# from USB stdio
	pico_stdio
	pico_time
	pico_unique_id
	pico_usb_reset_interface_headers
	pico_flash
)

