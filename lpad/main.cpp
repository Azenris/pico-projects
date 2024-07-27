
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/rand.h" 
#include "pico/util/queue.h"
#include "hardware/watchdog.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "rgb_keypad.h"
#include "random.h"
#include "utility.h"

#define ARRAY_LENGTH( arr )		( sizeof( arr ) / sizeof( arr[ 0 ] ) )

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

struct PhotonSmashPredefinedLevel
{
	u8 lightsCount;
	u8 lights[ RGBKeypad::NUM_PADS ];
};

constexpr PhotonSmashPredefinedLevel photonSmashPredefinedLevels[] =
{
	{
		.lightsCount = 5,
		.lights = { 1, 4, 5, 6, 9 }
	},
	{
		.lightsCount = 4,
		.lights = { 0, 3, 12, 15 }
	},
	{
		.lightsCount = 4,
		.lights = { 3, 6, 9, 12 }
	},
	{
		.lightsCount = 4,
		.lights = { 5, 6, 9, 10 }
	},
	{
		.lightsCount = 5,
		.lights = { 4, 8, 11, 13, 15 }
	},
	{
		.lightsCount = 4,
		.lights = { 0, 5, 10, 15 }
	},
	{
		.lightsCount = 6,
		.lights = { 1, 2, 4, 5, 6, 7 }
	},
	{
		.lightsCount = 8,
		.lights = { 1, 2, 5, 6, 9, 10, 13, 14 }
	},
	{
		.lightsCount = 12,
		.lights = { 0, 1, 2, 3, 4, 7, 8, 11, 12, 13, 14, 15 }
	},
	{
		.lightsCount = 14,
		.lights = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15 }
	},
	{
		.lightsCount = 16,
		.lights = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
	},
};

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

constexpr Colour COLOUR_WHITE = { 31, 31, 31 };
constexpr Colour COLOUR_RED = { 31, 0, 0 };
constexpr Colour COLOUR_ORANGE = { 31, 16, 1 };
constexpr Colour COLOUR_GREEN = { 0, 31, 0 };
constexpr Colour COLOUR_BLUE = { 0, 0, 31 };
constexpr Colour COLOUR_YELLOW = { 31, 31, 0 };
constexpr Colour COLOUR_MAGENTA = { 31, 0, 31 };
constexpr Colour COLOUR_AQUA = { 0, 31, 31 };

enum APP_MODE
{
	PROGRAMMING_LBOE,
	PROGRAMMING_GBC,
	PROGRAMMING_PICO_PROJECT,
	KEYBINDS,
	GAME_PHOTON_SMASH,
	COUNT,
};

enum PHOTON_SMASH_STATE
{
	GAME,
	WIN_ANIMATION,
	UNSOLVABLE_ANIMATION,
};

constexpr Colour colourThemes[ APP_MODE::COUNT ] =
{
	COLOUR_AQUA,				// APP_MODE::PROGRAMMING_LBOE
	COLOUR_GREEN,				// APP_MODE::PROGRAMMING_GBC
	COLOUR_ORANGE,				// APP_MODE::PROGRAMMING_PICO_PROJECT
	COLOUR_YELLOW,				// APP_MODE::KEYBINDS
	COLOUR_MAGENTA,				// APP_MODE::GAME_PHOTON_SMASH
};

struct QueuedKey
{
	u8 keys[ 6 ];
	u8 modifiers;
};

struct PhotonSmash
{
	APP_MODE prevMode;
	PHOTON_SMASH_STATE state;
	u8 level;
	Colour colour;
	u32 animationTime;
	bool rainbowLevel;
};

struct App
{
	APP_MODE mode;
	u32 hidTaskTimer;
	u32 hidTaskRate;
	u32 updateTimer;
	u32 updateRate;
	queue_t keyQueue;
	PhotonSmash photonSmash;
	bool startResetTimer;
	u32 rainbowColourTimer;
	u8 rainbowColourUpdateRate;
	Colour rainbowHSVColour;
};

App app;
RGBKeypad rgbKeypad;

static void toggle_light( u8 index )
{
	rgbKeypad.set_colour( index, app.photonSmash.colour, rgbKeypad.get_brightness( index ) == 0 ? 0.65f : 0 );
}

static bool photon_smash_solvability_check( u8 lights[ RGBKeypad::NUM_PADS ] )
{
	// Attempt to solve the state
	for ( i32 index = 4; index < RGBKeypad::NUM_PADS; ++index )
	{
		if ( lights[ index - 4 ] )
		{
			lights[ index ] = !lights[ index ];
			lights[ index - 4 ] = !lights[ index - 4 ];

			// Check its not at the bottom edge
			if ( index < 12 )
			{
				lights[ index + 4 ] = !lights[ index + 4 ];
			}

			// Check its not at the left edge
			if ( index % 4 != 0 )
			{
				lights[ index - 1 ] = !lights[ index - 1 ];
			}

			// Check its not at the right edge
			if ( ( index + 1 ) % 4 != 0 )
			{
				lights[ index + 1 ] = !lights[ index + 1 ];
			}
		}
	}

	// If any lights are still on it failed
	for ( i32 index = 0; index < RGBKeypad::NUM_PADS; ++index )
	{
		if ( lights[ index ] )
			return false;
	}

	return true;
}

static void system_reset()
{
	watchdog_enable( 100, 1 );
	while ( 1 );
}

static bool photon_smash_solvability_check()
{
	u8 lights[ RGBKeypad::NUM_PADS ];

	// Copy the state
	for ( i32 index = 0; index < RGBKeypad::NUM_PADS; ++index )
	{
		lights[ index ] = rgbKeypad.get_brightness( index ) != 0;
	}

	return photon_smash_solvability_check( lights );
}

static void default_selections()
{
	for ( i32 i = 0; i < APP_MODE::COUNT; ++i )
	{
		rgbKeypad.set_colour( i, colourThemes[ i ], 0.075f );
	}

	rgbKeypad.set_colour( app.mode, colourThemes[ app.mode ], 0.25f );

	rgbKeypad.set_colour( 7, COLOUR_RED, 0.15f );
}

static void app_switch_mode( APP_MODE newMode )
{
	APP_MODE prevAppMode = app.mode;

	app.mode = newMode;

	rgbKeypad.clear();

	switch ( newMode )
	{
	case APP_MODE::PROGRAMMING_LBOE:
		[[fallthrough]];
	case APP_MODE::PROGRAMMING_GBC:
		[[fallthrough]];
	case APP_MODE::PROGRAMMING_PICO_PROJECT:
		[[fallthrough]];
	case APP_MODE::KEYBINDS:
		default_selections();

		for ( i32 i = 8; i < 16; ++i )
		{
			rgbKeypad.set_colour( i, colourThemes[ newMode ], 0.2f );
		}
		break;

	case APP_MODE::GAME_PHOTON_SMASH:
		{
			if ( prevAppMode != GAME_PHOTON_SMASH )
			{
				app.photonSmash.prevMode = prevAppMode;
			}

			app.photonSmash.state = PHOTON_SMASH_STATE::GAME;

			app.photonSmash.colour =
			{
				static_cast<u8>( irandom( static_cast<u32>( 31 ) ) ),
				static_cast<u8>( irandom( static_cast<u32>( 31 ) ) ),
				static_cast<u8>( irandom( static_cast<i32>( 31 ) ) )
			};

			i32 lvl = app.photonSmash.level;

			if ( lvl < ARRAY_LENGTH( photonSmashPredefinedLevels ) )
			{
				const PhotonSmashPredefinedLevel *predefinedLevel = &photonSmashPredefinedLevels[ lvl ];

				for ( i32 i = 0, count = predefinedLevel->lightsCount; i < count; ++i )
				{
					rgbKeypad.set_colour( predefinedLevel->lights[ i ], app.photonSmash.colour, 0.75f );
				}

				// Check the predefined level can be completed, if not flash red
				if ( !photon_smash_solvability_check() )
				{
					app.photonSmash.state = PHOTON_SMASH_STATE::UNSOLVABLE_ANIMATION;
					app.photonSmash.animationTime = 0;
				}
			}
			else
			{
				i32 startWithLigjts = min( static_cast<i32>( 15 ), irandom_range( 1 + lvl / 10, lvl / 3 ) );
				u8 positions[ RGBKeypad::NUM_PADS ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
				i32 positionsCount = 15;

				while ( startWithLigjts-- > 0 )
				{
					i32 r = irandom_range( 0, positionsCount );
					rgbKeypad.set_colour( positions[ r ], app.photonSmash.colour, 0.65f );
					positions[ r ] = positions[ positionsCount-- ];
				}

				// Check if can be solved, if not, generate one in a safer method
				if ( !photon_smash_solvability_check() )
				{
					rgbKeypad.clear();

					i32 presses = min( static_cast<i32>( 50 ), irandom_range( 1 + lvl, lvl * 2 ) );

					while ( presses-- > 0 )
					{
						toggle_light( static_cast<u8>( irandom( RGBKeypad::NUM_PADS ) ) );
					}

					bool oneLit = false;
					for ( i32 i = 0; i < RGBKeypad::NUM_PADS; ++i )
					{
						if ( rgbKeypad.get_brightness( i ) != 0 )
						{
							oneLit = true;
							break;
						}
					}

					// If one wasn't lit or its not solvable, generate another
					if ( !oneLit || !photon_smash_solvability_check() )
					{
						rgbKeypad.clear();

						i32 position = irandom( RGBKeypad::NUM_PADS - 1 );
						toggle_light( position );

						position = ( position + irandom( RGBKeypad::NUM_PADS - 2 ) ) % RGBKeypad::NUM_PADS;
						toggle_light( position );

						if ( proc( 50 ) )
						{
							position = ( position + irandom( RGBKeypad::NUM_PADS - 2 ) ) % RGBKeypad::NUM_PADS;
							toggle_light( position );

							if ( proc( 25 ) )
							{
								position = ( position + irandom( RGBKeypad::NUM_PADS - 2 ) ) % RGBKeypad::NUM_PADS;
								toggle_light( position );

								if ( proc( 5 ) )
								{
									position = ( position + irandom( RGBKeypad::NUM_PADS - 2 ) ) % RGBKeypad::NUM_PADS;
									toggle_light( position );

									if ( proc( 1 ) )
									{
										position = ( position + irandom( RGBKeypad::NUM_PADS - 2 ) ) % RGBKeypad::NUM_PADS;
										toggle_light( position );
									}
								}
							}
						}
					}

					// Check if it can be completed, if not, use a random predefined level
					if ( !photon_smash_solvability_check() )
					{
						rgbKeypad.clear();

						i32 randomPredefinedLevel = irandom( static_cast<i32>( ARRAY_LENGTH( photonSmashPredefinedLevels ) - 1 ) );

						const PhotonSmashPredefinedLevel *predefinedLevel = &photonSmashPredefinedLevels[ randomPredefinedLevel ];

						for ( i32 i = 0, count = predefinedLevel->lightsCount; i < count; ++i )
						{
							rgbKeypad.set_colour( predefinedLevel->lights[ i ], app.photonSmash.colour, 0.75f );
						}
					}

					// Check it can be completed again, if not flash red
					if ( !photon_smash_solvability_check() )
					{
						app.photonSmash.state = PHOTON_SMASH_STATE::UNSOLVABLE_ANIMATION;
						app.photonSmash.animationTime = 0;
					}
				}
			}

			app.photonSmash.rainbowLevel = proc( 6 );
		}
		break;

	default:
		default_selections();
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
	// gpio_put( PICO_DEFAULT_LED_PIN, 1 );
}

// Invoked when device is unmounted
void tud_umount_cb()
{
	// gpio_put( PICO_DEFAULT_LED_PIN, 0 );
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

static void key_check( u8 modPrefix, u16 keysPressed, u16 keysDown )
{
	if ( keysPressed & KEY_8 )
	{
		add_key( modPrefix, HID_KEY_F13 );
	}
	else if ( keysPressed & KEY_9 )
	{
		add_key( modPrefix, HID_KEY_F14 );
	}
	else if ( keysPressed & KEY_10 )
	{
		add_key( modPrefix, HID_KEY_F15 );
	}
	else if ( keysPressed & KEY_11 )
	{
		add_key( modPrefix, HID_KEY_F16 );
	}
	else if ( keysPressed & KEY_12 )
	{
		add_key( modPrefix, HID_KEY_F17 );
	}
	else if ( keysPressed & KEY_13 )
	{
		add_key( modPrefix, HID_KEY_F18 );
	}
	else if ( keysPressed & KEY_14 )
	{
		add_key( modPrefix, HID_KEY_F19 );
	}
	else if ( keysPressed & KEY_15 )
	{
		if ( keysDown & KEY_12 )
		{
			system_reset();
		}
		else
		{
			add_key( modPrefix, HID_KEY_F20 );
		}
	}
}

static void mode_selection( u16 keysPressed, u16 keysDown )
{
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
		app_switch_mode( APP_MODE::PROGRAMMING_PICO_PROJECT );
	}
	else if ( keysPressed & KEY_3 )
	{
		app_switch_mode( APP_MODE::KEYBINDS );
	}
	else if ( keysPressed & KEY_4 )
	{
		app_switch_mode( APP_MODE::GAME_PHOTON_SMASH );
	}
	else if ( keysPressed & KEY_7 )
	{
		rgbKeypad.clear();
	}
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
	gpio_init( PICO_DEFAULT_LED_PIN );

	gpio_set_dir( PICO_DEFAULT_LED_PIN, GPIO_OUT );

	app.mode = APP_MODE::COUNT;
	app.hidTaskTimer = 8;							// ms
	app.hidTaskRate = app.hidTaskTimer;
	app.updateTimer = 16;							// ms
	app.updateRate = app.updateTimer;
	app.photonSmash.level = 0;
	app.startResetTimer = false;
	app.rainbowColourTimer = 0;
	app.rainbowColourUpdateRate = 1;				// updates before changing colour
	app.rainbowHSVColour = { 0, 31, 31 };

	queue_init( &app.keyQueue, sizeof( QueuedKey ), MAX_KEY_QUEUE_ELEMENTS );

	app_switch_mode( APP_MODE::PROGRAMMING_GBC );

	// In debug mode check all the predefined levels can be solved
	#ifdef DEBUG
		for ( i32 i = 0; i < ARRAY_LENGTH( photonSmashPredefinedLevels ); ++i )
		{
			u8 lights[ RGBKeypad::NUM_PADS ] = {};

			const PhotonSmashPredefinedLevel *predefinedLevel = &photonSmashPredefinedLevels[ i ];

			for ( i32 lightIdx = 0, count = predefinedLevel->lightsCount; lightIdx < count; ++lightIdx )
			{
				lights[ predefinedLevel->lights[ lightIdx ] ] = 1;
			}

			// Check the predefined level can be completed
			if ( !photon_smash_solvability_check( lights ) )
			{
				rgbKeypad.clear();

				rgbKeypad.set_colour( i, COLOUR_YELLOW, 1.0f );

				i -= 16;

				if ( i > 0 )
				{
					rgbKeypad.set_colour( i / 16, COLOUR_RED, 1.0f );
				}

				break;
			}
		}
	#endif

	u16 keysDownLast = 0;
	u32 time = board_millis();
	u32 lastTime = time;
	u32 timeDiff = 0;

	watchdog_enable( 200, 1 );

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

			if ( !app.startResetTimer )
			{
				if ( board_button_read() )
				{
					app.startResetTimer = true;
					watchdog_enable( 2 * 1000, 1 );
				}

				watchdog_update();
			}
			else
			{
				if ( !board_button_read() )
				{
					app.startResetTimer = false;
					watchdog_enable( 200, 1 );
					watchdog_update();
				}
			}

			u16 keysDown = rgbKeypad.get_button_states();
			u16 keysPressed = ~keysDownLast & keysDown;

			keysDownLast = keysDown;

			app.rainbowColourTimer += 1;

			if ( app.rainbowColourTimer >= app.rainbowColourUpdateRate )
			{
				app.rainbowColourTimer -= app.rainbowColourUpdateRate;
				app.rainbowHSVColour.r = ( app.rainbowHSVColour.r + 1 ) % 32;
			}

			switch ( app.mode )
			{
			case APP_MODE::PROGRAMMING_LBOE:
				{
					mode_selection( keysPressed, keysDown );
					key_check( KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT, keysPressed, keysDown );
				}
				break;

			case APP_MODE::PROGRAMMING_GBC:
				{
					mode_selection( keysPressed, keysDown );
					key_check( KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT, keysPressed, keysDown );
				}
				break;

			case APP_MODE::PROGRAMMING_PICO_PROJECT:
				{
					mode_selection( keysPressed, keysDown );
					key_check( KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL | KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT, keysPressed, keysDown );
				}
				break;

			case APP_MODE::KEYBINDS:
				{
					mode_selection( keysPressed, keysDown );
					key_check( KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT, keysPressed, keysDown );
				}
				break;

			case APP_MODE::GAME_PHOTON_SMASH:
				if ( board_button_read() )
				{
					// Exit game
					app_switch_mode( app.photonSmash.prevMode );
				}
				else
				{
					switch ( app.photonSmash.state )
					{
					case PHOTON_SMASH_STATE::GAME:
						if ( app.photonSmash.rainbowLevel )
						{
							rgbKeypad.set_colour( hsv_to_rgb( app.rainbowHSVColour ) );
						}

						if ( keysPressed )
						{
							i32 index = 0;
							while ( ( keysPressed & 1 ) == 0 )
							{
								++index;
								keysPressed >>= 1;
							}

							toggle_light( index );

							// Check its not at the top edge
							if ( index > 3 )
							{
								toggle_light( index - 4 );
							}

							// Check its not at the bottom edge
							if ( index < 12 )
							{
								toggle_light( index + 4 );
							}

							// Check its not at the left edge
							if ( index % 4 != 0 )
							{
								toggle_light( index - 1 );
							}

							// Check its not at the right edge
							if ( ( index + 1 ) % 4 != 0 )
							{
								toggle_light( index + 1 );
							}

							// Check win condition
							bool won = true;

							for ( u8 i = 0; i < RGBKeypad::NUM_PADS; ++i )
							{
								if ( rgbKeypad.get_brightness( i ) != 0 )
								{
									won = false;
									break;
								}
							}

							if ( won )
							{
								app.photonSmash.state = PHOTON_SMASH_STATE::WIN_ANIMATION;
								app.photonSmash.level += 1;
								app.photonSmash.animationTime = 0;
							}
						}
						break;

					case PHOTON_SMASH_STATE::WIN_ANIMATION:
						{
							if ( app.photonSmash.animationTime == 0 )
							{
								rgbKeypad.set_brightness( 0.5f );
							}

							app.photonSmash.animationTime += app.updateRate;

							rgbKeypad.set_colour( ( ( app.photonSmash.animationTime / 250 ) & 1 ) ? COLOUR_GREEN : COLOUR_WHITE );

							if ( app.photonSmash.animationTime >= 1000 )
							{
								app_switch_mode( APP_MODE::GAME_PHOTON_SMASH );
							}
						}
						break;

					case PHOTON_SMASH_STATE::UNSOLVABLE_ANIMATION:
						{
							if ( app.photonSmash.animationTime == 0 )
							{
								rgbKeypad.set_brightness( 0.5f );
							}

							app.photonSmash.animationTime += app.updateRate;

							rgbKeypad.set_colour( ( ( app.photonSmash.animationTime / 250 ) & 1 ) ? COLOUR_RED : COLOUR_YELLOW );

							if ( app.photonSmash.animationTime >= 1500 )
							{
								app_switch_mode( app.photonSmash.prevMode );
							}
						}
						break;
					}
				}
				break;
			}

			rgbKeypad.update();
		}
	}

	queue_free( &app.keyQueue );

	return 0;
}