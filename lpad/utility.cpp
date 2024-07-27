
#include <math.h>

#include "pico/stdlib.h"

#include "utility.h"

[[nodiscard]] inline static Colour make_colour( f32 a, f32 b, f32 c )
{
	return { static_cast<u8>( a * 31 ), static_cast<u8>( b * 31 ), static_cast<u8>( c * 31 ) };
}

[[nodiscard]] Colour rgb_to_hsv( Colour colourIn )
{
	f32 r = colourIn.r / 31.f;
	f32 g = colourIn.g / 31.f;
	f32 b = colourIn.b / 31.f;
	f32 h, s, v;
	f32 min, max, delta;

	min = r < g ? r : g;
	min = min < b ? min : b;

	max = r > g ? r : g;
	max = max > b ? max : b;

	delta = max - min;

	v = max;													// value

	if ( delta < 0.00001f )
	{
		s = 0;
		h = 0;
		return make_colour( h, s, v );
	}

	if ( max > 0.0f )
	{
		s = ( delta / max );									// saturation
	}
	else
	{
		s = 0.0;
		h = NAN;
		return make_colour( h, s, v );
	}

	if ( r >= max )
	{
		h = ( g - b ) / delta;									// between yellow & magenta
	}
	else
	{
		if ( g >= max )
			h = 2.0f + ( b - r ) / delta;						// between cyan & yellow
		else
			h = 4.0f + ( r - g ) / delta;						// between magenta & cyan
	}

	h *= 60.0f;													// degrees

	if ( h < 0.0f )
		h += 360.0f;

	h /= 360.0f;

	return make_colour( h, s, v );
}

[[nodiscard]] Colour hsv_to_rgb( Colour colourIn )
{
	f32 h = colourIn.r / 31.f;
	f32 s = colourIn.g / 31.f;
	f32 v = colourIn.b / 31.f;

	f32 r, g, b;

	if ( s <= 0.0f )
	{
		return make_colour( v, v, v );
	}

	h *= 360.0f;
	if ( h < 0.0f || h > 360.0f )
		h = 0.0f;
	h /= 60.0f;

	u32 i = static_cast<u32>( h );
	f32 ff = h - i;
	f32 p = v * ( 1.0f - s );
	f32 q = v * ( 1.0f - ( s * ff ) );
	f32 t = v * ( 1.0f - ( s * ( 1.0f - ff ) ) );

	switch ( i )
	{
	case 0:
		r = v;
		g = t;
		b = p;
		break;

	case 1:
		r = q;
		g = v;
		b = p;
		break;

	case 2:
		r = p;
		g = v;
		b = t;
		break;

	case 3:
		r = p;
		g = q;
		b = v;
		break;

	case 4:
		r = t;
		g = p;
		b = v;
		break;

	default:
		r = v;
		g = p;
		b = q;
		break;
	}

	return make_colour( r, g, b );
}