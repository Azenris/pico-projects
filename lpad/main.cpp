#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/rand.h" 
#include "pico/util/queue.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "rgb_keypad.h"
#include "random.h"

template <typename T>
T min( T a, T b )
{
	return a <= b ? a : b;
}

template <typename T>
T max( T a, T b )
{
	return a >= b ? a : b;
}

enum
{
	 KEY_0 = ( 1 << 0 ),
	 KEY_1 = ( 1 << 1 ),
	 KEY_2 = ( 1 << 2 ),
	 KEY_3 = ( 1 << 3 ),
	 KEY_4 = ( 1 << 4 ),
	 KEY_5 = ( 1 << 5 ),
	 KEY_6 = ( 1 << 6 ),
	 KEY_7 = ( 1 << 7 ),
	 KEY_8 = ( 1 << 8 ),
	 KEY_9 = ( 1 << 9 ),
	 KEY_10 = ( 1 << 10 ),
	 KEY_11 = ( 1 << 11 ),
	 KEY_12 = ( 1 << 12 ),
	 KEY_13 = ( 1 << 13 ),
	 KEY_14 = ( 1 << 14 ),
	 KEY_15 = ( 1 << 15 ),
};

constexpr u32 MAX_KEY_QUEUE_ELEMENTS = 16;

constexpr Colour COLOUR_RED = { 31, 0, 0 };
constexpr Colour COLOUR_GREEN = { 0, 31, 0 };
constexpr Colour COLOUR_BLUE = { 0, 0, 31 };
constexpr Colour COLOUR_YELLOW = { 31, 31, 0 };
constexpr Colour COLOUR_MAGENTA = { 31, 0, 31 };
constexpr Colour COLOUR_AQUA = { 0, 31, 31 };

enum APP_MODE
{
	PROGRAMMING_LBOE,
	PROGRAMMING_GBC,
	GAME_FLASHOUT,
	COUNT,
};

constexpr Colour colourThemes[ APP_MODE::COUNT ] =
{
	COLOUR_AQUA,				// APP_MODE::PROGRAMMING_LBOE
	COLOUR_GREEN,				// APP_MODE::PROGRAMMING_GBC
	COLOUR_MAGENTA,				// APP_MODE::GAME_FLASHOUT
};

struct QueuedKey
{
	u8 keys[ 6 ] = {};
	u8 modifiers = 0;
};

struct App
{
	APP_MODE mode;
	u32 hidTaskTimer;
	u32 hidTaskRate;
	u32 updateTimer;
	u32 updateRate;
	queue_t keyQueue;
	u8 flashoutLevel;
};

App app;
RGBKeypad rgbKeypad;

void app_switch_mode( APP_MODE newMode )
{
	app.mode = newMode;

	rgbKeypad.clear();

	for ( i32 i = 0; i < APP_MODE::COUNT; ++i )
	{
		rgbKeypad.set_colour( i, colourThemes[ i ], 0.075f );
	}

	switch ( newMode )
	{
	case PROGRAMMING_LBOE:
		rgbKeypad.set_colour( newMode, colourThemes[ newMode ], 1.0f );

		for ( i32 i = 4; i < 4; ++i )
		{
			rgbKeypad.set_colour( i, colourThemes[ newMode ], 0.25f );
		}
		break;

	case PROGRAMMING_GBC:
		rgbKeypad.set_colour( newMode, colourThemes[ newMode ], 1.0f );

		for ( i32 i = 4; i < 4; ++i )
		{
			rgbKeypad.set_colour( i, colourThemes[ newMode ], 0.25f );
		}
		break;

	case GAME_FLASHOUT:
		{
			i32 lvl = app.flashoutLevel;
			i32 startWithLigjts = min( static_cast<i32>( 15 ), irandom_range( 1 + lvl / 10, lvl / 3 ) );
			// TODO : 
		}
		break;
	}
}

// Invoked when sent REPORT successfully to host
void tud_hid_report_complete_cb( u8 instance, u8 const *report, u16 len )
{
	(void) instance;
	(void) report;
	(void) len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
u16 tud_hid_get_report_cb( u8 instance, u8 reportID, hid_report_type_t reportType, u8 *buffer, u16 reqlen )
{
	(void) instance;
	(void) reportID;
	(void) reportType;
	(void) buffer;
	(void) reqlen;

	return 0;
}

// Invoked when device is mounted
void tud_mount_cb()
{
}

// Invoked when device is unmounted
void tud_umount_cb()
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb( bool remoteWakeupEn )
{
	(void) remoteWakeupEn;
}

// Invoked when usb bus is resumed
void tud_resume_cb()
{
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb( u8 instance, u8 reportID, hid_report_type_t reportType, u8 const *buffer, u16 bufsize )
{
	(void) instance;

	if ( reportType == HID_REPORT_TYPE_OUTPUT )
	{
		if ( reportID == REPORT_ID_KEYBOARD )
		{
			// bufsize should be (at least) 1
			if ( bufsize < 1 )
				return;

			u8 const kbd_leds = buffer[ 0 ];

			if ( kbd_leds & KEYBOARD_LED_CAPSLOCK )	
			{
				// Capslock is on
			}
			else
			{
				// Capslock is off
			}
		}
	}
}

static void add_key( QueuedKey key )
{
	queue_try_add( &app.keyQueue, &key );
	key = {};
	queue_try_add( &app.keyQueue, &key );
}

static void add_key( u8 modifiers, u8 key )
{
	QueuedKey qKey = { .keys = { key }, .modifiers = modifiers };
	queue_try_add( &app.keyQueue, &qKey );
	qKey = {};
	queue_try_add( &app.keyQueue, &qKey );
}

int main()
{
	{
		u64 seed[ 9 ] =
		{
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			get_rand_64(),
			board_millis() * 0x1986,
		};

		random_set_seed( seed );
	}

	board_init();
	tusb_init();

	rgbKeypad.init();

	app.mode = APP_MODE::COUNT;
	app.hidTaskTimer = 8;
	app.hidTaskRate = app.hidTaskTimer;
	app.updateTimer = 16;
	app.updateRate = app.updateTimer;
	app.flashoutLevel = 0;

	queue_init( &app.keyQueue, sizeof( QueuedKey ), MAX_KEY_QUEUE_ELEMENTS );

	app_switch_mode( APP_MODE::PROGRAMMING_GBC );

	u16 keysDownLast = 0;
	u32 time = board_millis();
	u32 lastTime = time;
	u32 timeDiff = 0;

	while ( true )
	{
		tud_task();

		lastTime = time;
		time = board_millis();
		timeDiff = time - lastTime;

		app.hidTaskTimer += timeDiff;
		app.updateTimer += timeDiff;

		// Update every 8ms
		if ( app.hidTaskTimer >= app.hidTaskRate )
		{
			app.hidTaskTimer -= app.hidTaskRate;

			u32 const btn = board_button_read();

			if ( tud_suspended() && btn )
			{
				tud_remote_wakeup();
			}
			else if ( tud_hid_ready() )
			{
				if ( !queue_is_empty( &app.keyQueue ) )
				{
					QueuedKey key;

					if ( queue_try_remove( &app.keyQueue, &key ) )
					{
						tud_hid_keyboard_report( REPORT_ID_KEYBOARD, key.modifiers, key.keys );
					}
				}
			}
		}

		// Update every 16ms
		if ( app.updateTimer >= app.updateRate )
		{
			app.updateTimer -= app.updateRate;

			u16 keysDown = rgbKeypad.get_button_states();
			u16 keysPressed = ~keysDownLast & keysDown;

			keysDownLast = keysDown;

			if ( keysPressed & KEY_0 )
			{
				app_switch_mode( APP_MODE::PROGRAMMING_LBOE );
			}
			else if ( keysPressed & KEY_1 )
			{
				app_switch_mode( APP_MODE::PROGRAMMING_GBC );
			}
			else if ( keysPressed & KEY_2 )
			{
				app_switch_mode( APP_MODE::GAME_FLASHOUT );
			}
			else if ( keysPressed & KEY_3 )
			{
				rgbKeypad.clear();
			}
			else
			{
				switch ( app.mode )
				{
				case APP_MODE::PROGRAMMING_LBOE:
					{
						constexpr u8 modPrefix = KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT;

						if ( keysPressed & KEY_4 )
						{
							// Setup working on the LostBookOfElements game
							add_key( modPrefix, HID_KEY_F1 );
						}
						else if ( keysPressed & KEY_5 )
						{
							// Compile LostBookOfElements Game
							add_key( modPrefix, HID_KEY_F2 );
						}
						else if ( keysPressed & KEY_6 )
						{
							// Compile LostBookOfElements Engine+Game
							add_key( modPrefix, HID_KEY_F3 );
						}
						else if ( keysPressed & KEY_7 )
						{
							// Open LostBookOfElements Game Folder
							add_key( modPrefix, HID_KEY_F4 );
						}
					}
					break;

				case APP_MODE::PROGRAMMING_GBC:
					{
						constexpr u8 modPrefix = KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT;

						if ( keysPressed & KEY_4 )
						{
							// Setup working on the MoondustCompanions game
							add_key( modPrefix, HID_KEY_F1 );
						}
						else if ( keysPressed & KEY_5 )
						{
							// Compile MoondustCompanions Game
							add_key( modPrefix, HID_KEY_F2 );
						}
						else if ( keysPressed & KEY_6 )
						{
							// currently unused
							add_key( modPrefix, HID_KEY_F3 );
						}
						else if ( keysPressed & KEY_7 )
						{
							// Open MoondustCompanions Game Folder
							add_key( modPrefix, HID_KEY_F4 );
						}
					}
					break;

				case APP_MODE::GAME_FLASHOUT:
					if ( board_button_read() )
					{
						// Exit game
						app_switch_mode( APP_MODE::COUNT );
					}
					else if ( keysPressed )
					{
						// TODO change lights, check win condition
					}
					break;
				}
			}

			rgbKeypad.update();
		}
	}

	queue_free( &app.keyQueue );

	return 0;
}