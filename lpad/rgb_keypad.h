#pragma once

#include "types.h"

struct RGBKeypad
{
	static constexpr u8 KEYPAD_ADDRESS = 0x20;
	static constexpr f32 DEFAULT_BRIGHTNESS = 0.5f;
	static constexpr i32 WIDTH = 4;
	static constexpr i32 HEIGHT = 4;
	static constexpr i32 NUM_PADS = WIDTH * HEIGHT;
	static constexpr i32 BUFFER_SIZE = ( NUM_PADS * 4 ) + 8;

	u8 buffer[ BUFFER_SIZE ];
	u8 *ledData;

	void init( f32 defaultBrightness = DEFAULT_BRIGHTNESS );
	void update();
	void clear();
	void free();

	void set_brightness( f32 brightness );
	void set_colour( u8 x, u8 y, u8 r, u8 g, u8 b );
	void set_colour( u8 index, u8 r, u8 g, u8 b );
	void set_colour( u8 index, u8 r, u8 g, u8 b, f32 brightness );
	void set_colour( u8 index, Colour colour );
	void set_colour( u8 index, Colour colour, f32 brightness );

	u16 get_button_states();
};