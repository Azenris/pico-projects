cmake_minimum_required( VERSION 3.12 )

function( message )
	if ( NOT MESSAGE_QUIET )
		_message( ${ARGN} )
	endif()
endfunction()

set( PROJECT "blank" )

set( PICO_SDK_PATH "${PICO_PATH}/pico-sdk" )
set( PICO_PLATFORM "rp2040" )
set( PICO_TOOLCHAIN_PATH "${PICO_PATH}/gcc-arm-none-eabi/bin" )
set( PICO_COMPILER "pico_arm_gcc" )

include( "${PICO_SDK_PATH}/external/pico_sdk_import.cmake" )

set( MESSAGE_QUIET ON )

project( ${PROJECT} C CXX )

pico_sdk_init()

add_executable( ${PROJECT} main.c )

target_link_libraries( ${PROJECT} pico_stdlib )

pico_add_extra_outputs( ${PROJECT} )
pico_enable_stdio_usb( ${PROJECT} 1 )
pico_enable_stdio_uart( ${PROJECT} 0 )

add_compile_options( -Wall -Wno-format )

set( MESSAGE_QUIET OFF )