#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "rgb_keypad.h"

enum PIN
{
	SDA		= 4,
	SCL		= 5,
	CS		= 17,
	SCK		= 18,
	MOSI	= 19
};

void RGBKeypad::init( f32 defaultBrightness )
{
	memset( buffer, 0, sizeof( buffer ) );

	ledData = buffer + 4;

	set_brightness( defaultBrightness );

	i2c_init( i2c0, 400000 );

	gpio_set_function( PIN::SDA, GPIO_FUNC_I2C );
	gpio_pull_up( PIN::SDA );

	gpio_set_function( PIN::SCL, GPIO_FUNC_I2C );
	gpio_pull_up( PIN::SCL );

	spi_init( spi0, 4 * 1024 * 1024 );
	gpio_set_function( PIN::CS, GPIO_FUNC_SIO );
	gpio_set_dir( PIN::CS, GPIO_OUT );
	gpio_put( PIN::CS, 1 );
	gpio_set_function( PIN::SCK, GPIO_FUNC_SPI );
	gpio_set_function( PIN::MOSI, GPIO_FUNC_SPI );

	update();
}

void RGBKeypad::update()
{
	gpio_put( PIN::CS, 0 );
	spi_write_blocking( spi0, buffer, sizeof( buffer ) );
	gpio_put( PIN::CS, 1 );
}

void RGBKeypad::clear()
{
	u8 *ptr = ledData;

	for ( i32 i = 0; i < NUM_PADS; ++i )
	{
		*ptr++ = 0b11100000;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
	}
}

void RGBKeypad::free()
{
	clear();
	update();
}

void RGBKeypad::set_brightness( f32 brightness )
{
	if ( brightness < 0.0f || brightness > 1.0f )
		return;

	for ( i32 i = 0; i < NUM_PADS; ++i )
		ledData[ i * 4 ] = 0b11100000 | static_cast<u8>( brightness * 31.f );
}

void RGBKeypad::set_colour( u8 x, u8 y, u8 r, u8 g, u8 b )
{
	if ( x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT )
		return;

	i32 offset = ( x + ( y * WIDTH ) ) * 4;

	ledData[ offset + 1 ] = b;
	ledData[ offset + 2 ] = g;
	ledData[ offset + 3 ] = r;
}

void RGBKeypad::set_colour( u8 index, u8 r, u8 g, u8 b )
{
	if ( index < 0 || index >= NUM_PADS )
		return;

	i32 offset = index * 4;

	ledData[ offset + 1 ] = b;
	ledData[ offset + 2 ] = g;
	ledData[ offset + 3 ] = r;
}

void RGBKeypad::set_colour( u8 index, u8 r, u8 g, u8 b, f32 brightness )
{
	if ( index < 0 || index >= NUM_PADS )
		return;

	i32 offset = index * 4;

	ledData[ offset + 0 ] = 0b11100000 | static_cast<u8>( brightness * 31.f );
	ledData[ offset + 1 ] = b;
	ledData[ offset + 2 ] = g;
	ledData[ offset + 3 ] = r;
}

void RGBKeypad::set_colour( u8 index, Colour colour )
{
	set_colour( index, colour.r, colour.g, colour.b );
}

void RGBKeypad::set_colour( u8 index, Colour colour, f32 brightness )
{
	set_colour( index, colour.r, colour.g, colour.b, brightness );
}

u16 RGBKeypad::get_button_states()
{
	u8 i2c_read_buffer[ 2 ];
	u8 reg = 0;

	i2c_write_blocking( i2c0, KEYPAD_ADDRESS, &reg, 1, true );
	i2c_read_blocking( i2c0, KEYPAD_ADDRESS, i2c_read_buffer, 2, false );

	return ~( ( i2c_read_buffer[ 0 ] ) | ( i2c_read_buffer[ 1 ] << 8 ) );
}
