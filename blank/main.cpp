
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"

int main()
{
	gpio_init( PICO_DEFAULT_LED_PIN );

	gpio_set_dir( PICO_DEFAULT_LED_PIN, GPIO_OUT );

	while ( true )
	{
	}

	return 0;
}