#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "rgb_keypad.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

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

struct App
{
	APP_MODE mode;
	u32 hidTaskTimer;
	u32 hidTaskRate;
	u32 updateTimer;
	u32 updateRate;
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

	rgbKeypad.set_colour( newMode, colourThemes[ newMode ], 1.0f );
}

static void send_hid_report( u8 reportID, u32 btn )
{
	if ( !tud_hid_ready() )
		return;

	switch ( reportID )
	{
	case REPORT_ID_KEYBOARD:
		{
			// use to avoid send multiple consecutive zero report for keyboard
			static bool hadKey = false;

			if ( btn )
			{
				u8 keycode[ 6 ] = { 0 };
				keycode[ 0 ] = HID_KEY_A;

				tud_hid_keyboard_report( REPORT_ID_KEYBOARD, 0, keycode );
				hadKey = true;
			}
			else
			{
				// send empty key report if previously has key pressed
				if ( hadKey )
					tud_hid_keyboard_report( REPORT_ID_KEYBOARD, 0, NULL );
				hadKey = false;
			}
		}
		break;

	case REPORT_ID_MOUSE:
		{
			i32 const delta = 5;

			// no button, right + down, no scroll, no pan
			tud_hid_mouse_report( REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0 );
		}
		break;

	case REPORT_ID_CONSUMER_CONTROL:
		{
			// use to avoid send multiple consecutive zero report
			static bool hadKey = false;

			if ( btn )
			{
				// volume down
				u16 volumeDec = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
				tud_hid_report( REPORT_ID_CONSUMER_CONTROL, &volumeDec, 2 );
				hadKey = true;
			}
			else
			{
				// send empty key report (release key) if previously has key pressed
				u16 emptyKey = 0;
				if ( hadKey )
					tud_hid_report( REPORT_ID_CONSUMER_CONTROL, &emptyKey, 2 );
				hadKey = false;
			}
		}
		break;

	case REPORT_ID_GAMEPAD:
		{
			// use to avoid send multiple consecutive zero report for keyboard
			static bool hadKey = false;

			hid_gamepad_report_t report =
			{
				.x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
				.hat = 0, .buttons = 0
			};

			if ( btn )
			{
				report.hat = GAMEPAD_HAT_UP;
				report.buttons = GAMEPAD_BUTTON_A;
				tud_hid_report( REPORT_ID_GAMEPAD, &report, sizeof( report ) );

				hadKey = true;
			}
			else
			{
				report.hat = GAMEPAD_HAT_CENTERED;
				report.buttons = 0;
				if ( hadKey )
					tud_hid_report( REPORT_ID_GAMEPAD, &report, sizeof( report ) );
				hadKey = false;
			}
		}
		break;

	default:
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

int main()
{
	board_init();
	tusb_init();

	rgbKeypad.init();

	app.mode = APP_MODE::COUNT;
	app.hidTaskTimer = 8;
	app.hidTaskRate = app.hidTaskTimer;
	app.updateTimer = 16;
	app.updateRate = app.updateTimer;

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
			else
			{
				send_hid_report( REPORT_ID_KEYBOARD, btn );
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
					break;

				case APP_MODE::PROGRAMMING_GBC:
					if ( keysPressed & KEY_4 )
					{
						// TODO : compile/setup enviro etc...
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

	return 0;
}