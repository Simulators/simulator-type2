/*
Simulator Interface v3.2 Beta
Signal LED Functions

Copyright 2014-2018 Andrew J Instone-Cowie.

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
*********************************************************************************************
*                                    Function blinkLED()                                    *
*********************************************************************************************
*/

/*
Signal statuses using a couple of LEDs, zero or more long flashes followed
by zero or more short flashes. We can't use the serial port for debugging.

1 long & N short = MBI protocol commands
2 long & N short = Arduino config commands (EEPROM or volatile)
0 long & 1 short = CLI keypress (status, etc)
3 long & 0 short = Serial Data Error
*/

void blinkLED( int led, int x, int y) {

	// duration of flashes in ms
	int longflash=250;
	int shortflash=75;

	// long flashes...
	while (x > 0) {
		digitalWrite(led, HIGH);
		delay(longflash);
		digitalWrite(led, LOW);
		delay(longflash);
		x--;
	}

	// ...then short flashes
	while (y > 0) {
		digitalWrite(led, HIGH);
		delay(shortflash);
		digitalWrite(led, LOW);
		delay(longflash); // So you can see the short flashes
		y--;
	}

}

/*
*********************************************************************************************
*                                  Function strikeLED()                                     *
*********************************************************************************************
*/

/*
Use the LED to signal strikes on channel 1 (Always starts at channel 1, even for <16 bells).
Turn on when the pulse is received, off when guard timer expires. Array is zero indexed.

If we are in debug level DEBUG_LED, the the LED operates for all channels which have debug
enabled. This is intended for sensor testing in the belfry.
*/

void strikeLED( int led, int channel, boolean ledOn ) {

	if ( ( channel == 0 || ( debugThisChannel( channel ) && isDebugFlagSet( DEBUG_SHOW_LED ) ) ) && ledOn ) {
		digitalWrite(led, HIGH);   
	}
	else if ( channel == 0 || ( debugThisChannel( channel ) && isDebugFlagSet( DEBUG_SHOW_LED ) ) )  { 
		digitalWrite(led, LOW);   
	}

}