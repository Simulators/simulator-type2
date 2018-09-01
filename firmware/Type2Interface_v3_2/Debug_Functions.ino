/*
Simulator Interface v3.2 Beta
Debug Functions

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
*                                 Function debugThisChannel()                               *
*********************************************************************************************
*/

// If debugging is enabled, and debugging is turned on for this channel in the mask, return TRUE.
// Otherwise return FALSE. debugMode and debugChannelMask are global, so no need to pass them in
// via the function.

boolean	debugThisChannel( int channel ) {

	if ( debugMode && bitRead( debugChannelMask, channel ) ) {
		// Debugging is enabled and turned on for this channel.
		return true;
	}
	else
	{
		return false;
	}
	
}

/*
*********************************************************************************************
*                                Function isDebugFlagSet()                                  *
*********************************************************************************************
*/

// If debugging is enabled, if any of the specified bits are set in the debugLevelMask,
// return TRUE. Otherwise return FALSE. debugFlagsMask is global, so no need to pass them in
// via the function. We only call this if we are known to be in debug mode already.

// The flags parameter is either a single bit value (1/2/4/8/etc), or more than one value
// ORd together in the invocation of the function.

boolean	isDebugFlagSet( word flags ) {

	if ( debugFlagsMask & flags ) {
		// One or more of the specified flags are set.
		return true;
	}
	else
	{
		return false;
	}
	
}

/*
*********************************************************************************************
*                               Function printDebugFlagName()                               *
*********************************************************************************************
*/

// Passed the numeric debug flag bit number, prints a textual representation of that debug
// flag, followed by a newline.

void printDebugFlagName( int level ) {

	// So this is reasonably horrible, but as I can't do F() globally,
	// this will have to do to get the text version into flash...	
	switch ( level ) {
	case 0: // DEBUG_PULSE_TIMER
		Serial.print(F("DEBUG_PULSE_TIMER"));
		break;
	case 1: // DEBUG_SHOW_MISFIRES
		Serial.print(F("DEBUG_SHOW_MISFIRES"));
		break;
	case 2: // DEBUG_SHOW_DEBOUNCE
		Serial.print(F("DEBUG_SHOW_DEBOUNCE"));
		break;
	case 3: // DEBUG_SHOW_LED
		Serial.print(F("DEBUG_SHOW_LED"));
		break;
	default: // Some other debugLevel value
		Serial.print(F("Invalid"));
		break;
	}
	
}

/*
*********************************************************************************************
*                             Function printDebugFlagsSet()                                 *
*********************************************************************************************
*/

// List the debug flags which are currently set in debugFlagsMask, as a space separated list
void printDebugFlagsSet( void ) {
	int i;
	for ( i = 0; i < maxDebugFlags; i++ ) {
		// Print the value of the bits.
		if ( bitRead( debugFlagsMask, i ) ) {
			// The bit is set
			Serial.print("\t");
			printDebugFlagName( i );
			Serial.println("");
		}
	}
}
