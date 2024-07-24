cmake_minimum_required( VERSION 3.12 )

function( message )
	if ( NOT MESSAGE_QUIET )
		_message( ${ARGN} )
	endif()
endfunction()

set( PROJECT "lpad" )

set( PICO_SDK_PATH "${PICO_PATH}/pico-sdk" )
set( PICO_PLATFORM "rp2040" )
set( PICO_TOOLCHAIN_PATH "${PICO_PATH}/gcc-arm-none-eabi/bin" )
set( PICO_COMPILER "pico_arm_gcc" )
set( FAMILY "rp2040" )
set( PICO_BOARD "raspberry_pi_pico" )
set( CMAKE_BUILD_TYPE "Debug" )

include( "${PICO_SDK_PATH}/external/pico_sdk_import.cmake" )

set( MESSAGE_QUIET ON )

project( ${PROJECT} C CXX )

pico_sdk_init()

add_executable( ${PROJECT} main.cpp rgb_keypad.cpp random.cpp usb_descriptors.c )

# Make sure TinyUSB can find tusb_config.h
target_include_directories( ${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

target_link_libraries( ${PROJECT} PRIVATE pico_stdlib )
target_link_libraries( ${PROJECT} PRIVATE hardware_i2c )
target_link_libraries( ${PROJECT} PRIVATE hardware_spi )
target_link_libraries( ${PROJECT} PRIVATE pico_unique_id )
target_link_libraries( ${PROJECT} PRIVATE pico_util )
target_link_libraries( ${PROJECT} PRIVATE pico_rand )
target_link_libraries( ${PROJECT} PRIVATE tinyusb_board )
target_link_libraries( ${PROJECT} PRIVATE tinyusb_device )

target_compile_definitions( ${PROJECT} PRIVATE "$<$<CONFIG:Debug>:DEBUG>$<$<CONFIG:Release>:NDEBUG>" )

pico_add_extra_outputs( ${PROJECT} )

add_compile_options( -Wall -Wno-format )

set( CMAKE_CXX_STANDARD 23 )

set( MESSAGE_QUIET OFF )
