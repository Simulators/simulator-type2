/*
Simulator Interface v3.3 Beta
Serial CLI Functions

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
*                                   Function dumpData()                                     *
*********************************************************************************************
*/

// Dump current configuration data to the serial port.
void dumpData( void ) {

	// Position the cursor and clear the screen.
	termClear();

	// generic loop counter
	int i;
	
	// What version of software are we running?
	termSetFor( TERM_HEADING );
	Serial.print(F("Software Version: "));
	Serial.print( majorVersion );
	Serial.print(F("."));
	Serial.println( minorVersion );
	termSetFor( TERM_DEFAULT );

	// What version of hardware are we compiled for? 
	Serial.print(F("Hardware Version: "));
#if defined ARDUINO_AVR_SIMULATOR_TYPE2
	Serial.println("Type 2");
#else
	Serial.println(F("Unknown"));
#endif
	
	// How many channels are we scanning? 
	if ( debugMode ) {
		Serial.print(F("Highest Active Channel: "));
		Serial.println( numChannels );
	}
	
	// What are the debounce timer values? 
	Serial.print(F("Active Debounce Timer (ms): "));
	Serial.println( debounceDelay );
	Serial.print(F("EEPROM Debounce Timer (ms): "));
	Serial.println( EEPROM.read( EEPROM_DEBOUNCE_DELAY ) );

	// What are the guard timer values? 
	Serial.print(F("Active Guard Timer (cs): "));
	Serial.println( guardDelay );
	Serial.print(F("EEPROM Guard Timer (cs): "));
	Serial.println( EEPROM.read( EEPROM_GUARD_DELAY ) );

	// Start of Channels Status

	// Heading & space-separated list of channel numbers
	printChannelList();
	
	// Which channels are enabled?
	Serial.print(F("Enabled: "));
	printEnabledStatus( enabledChannelMask, maxNumChannels );	
	
	// What is the current output mapping?
	showActiveMapping();
	
	// What are the most recent sensor values?
	showSensorInputs();
	
	// End of Channels Status
	
	// Check for duplicate mappings and warn
	duplicateMapCheck();
	
	// What is the configured serial port speed? 
	Serial.print(F("Serial Port Speed: "));
	serialSpeed = int( EEPROM.read( EEPROM_SERIAL_SPEED ) );
	// setup sets a default 2400 if the EEPROM value is bad...
	if ( serialSpeed < 0 || serialSpeed > 2 ) {
		serialSpeed = defaultSerialSpeed;
	}
	Serial.println( serialSpeeds[serialSpeed] );  
	
	Serial.print(F("Free Memory: "));
	Serial.println( freeMemory() );  

	// Show debug mode status 
	Serial.print(F("Debug Mode: "));
	if ( debugMode ) { 
		Serial.println(F("ON"));
		// Serial.print(F("Debug Flags Mask: "));
		// printMask( debugFlagsMask, maxDebugFlags );
		printChannelList();
		Serial.print(F("DebugEn: "));
		printEnabledStatus( debugChannelMask, maxNumChannels );
		Serial.println(F("Debug Flags Set: "));
		printDebugFlagsSet();
	}
	else
	{
		Serial.println (F("OFF"));
	}

#ifdef USE_CRO
	Serial.print(F("CRO Timing Pin Enabled: "));
	Serial.println( CROpin );  
#endif

}

/*
*********************************************************************************************
*                                   Function showCLI()                                      *
*********************************************************************************************
*/

// Display the CLI prompt to the serial port.
void showCLI( void ) {

	// If you call showCLI(), you need to supply the carriage return later, e.g. after
	// you have echoed the keypress.

	// The CLI is different when in debug mode.

	// Set the terminal modes for the CLI.
	termSetFor( TERM_CLI );

	// Print the first part of the CLI
	Serial.print(F(" : B/G/E/S/P/R/D/"));

	if ( debugMode )
	{
		// Print the debug mode options - in this version all debug options
		// are available at any debug level.
		Serial.print(F("d/M/0-9/L/Z/"));
	}

	// Print the trailing part of the CLI
	Serial.print(F("H/T/? "));

	// Set the terminal modes back to normal.
	termSetFor( TERM_DEFAULT );

}

/*
*********************************************************************************************
*                                  Function handleCLI()                                     *
*********************************************************************************************
*/

// Handle CLI input.
void handleCLI( byte commandByte ) {

	// This function is passed the first (and possibly only) byte from the serial buffer,
	// once we know it's not an MBI command or new (erroneously sent) delay data.

	// generic loop counter
	int i;

	// generic scratch variable for vtSerial.ReadLong
	int readval;

	switch ( commandByte ) {

	case '?':    // ?  = Dump values to serial for debugging
		Serial.println( char( commandByte ) );
		dumpData();
		// blink the LED to indicate we got a valid command
		// just a quick blink so as not to slow the CLI down too much when cycling
		blinkLED( LED, 0, 1 ); //one quick
		break;

	case 'B':    // B  = Set the debounce timer value (ms)
	case 'b':
		Serial.println( char( commandByte ) );
		readval = 0; // Set the value to be read deliberately out of range.
		do {
			termSetFor( TERM_INPUT );
			Serial.print(F(" -> Enter debounce timer [1-20ms]: "));
			termSetFor( TERM_DEFAULT );
			readval = vtSerial.ReadLong();       // read integer
			Serial.println("");
		} while ( readval < 1 || readval > maxDebounceDelay );
		debounceDelay = readval;
		termSetFor( TERM_CONFIRM );
		Serial.print(F("Debounce Timer: "));
		Serial.print( debounceDelay );
		Serial.println(F("ms"));
		blinkLED( LED, 2, 3 );
		break;

	case 'G':    // G  = Set the guard timer value (cs)
	case 'g':
		Serial.println( char( commandByte ) );
		readval = 0; // Set the value to be read deliberately out of range.
		do {
			termSetFor( TERM_INPUT );
			Serial.print(F(" -> Enter guard timer [1-50cs]: "));
			termSetFor( TERM_DEFAULT );
			readval = vtSerial.ReadLong();       // read integer
			Serial.println("");
		} while ( readval < 1 || readval > maxGuardDelay );
		guardDelay = readval;
		termSetFor( TERM_CONFIRM );
		Serial.print(F("Guard Timer: "));
		Serial.print( guardDelay );
		Serial.println(F("cs"));
		blinkLED( LED, 2, 4 );
		break;
		
	case 'S':    // S  = Save settings to EEPROM as bytes
	case 's':
		Serial.println( char( commandByte ) );
		saveToEEPROM();
		// blink the LED to indicate we got a valid command
		blinkLED( LED, 2, 5 );
		break;

	case 'E':    // E  = Enable/Disable a channel (non-persistently)
	case 'e':
		Serial.println( char( commandByte ) );
		readval = 0; // Set the value to be read deliberately out of range.
		do {
			termSetFor( TERM_INPUT );
			// show the enabled sensors, LSB (treble) first
			printChannelList();
			Serial.print(F("Enabled: "));
			printEnabledStatus( enabledChannelMask, maxNumChannels ); // includes a CR
			// Prompt the user for a sensor number to toggle on or off
			Serial.print(F(" -> Toggle channel value [1-"));
			Serial.print( maxNumChannels );
			Serial.print(F(", 0=Done]: "));
			termSetFor( TERM_DEFAULT );
			readval = vtSerial.ReadLong(); // read integer ( expecting 1 - 16, 0 to finish)
			Serial.println("");
			// Toggle the channel bit in the enable mask by XOR-ing the current value with 1
			// (remembering that channel numbers are 1-16, bits/channels are 0-15). Max 16 bits.
			toggleMaskBit( &enabledChannelMask, readval - 1, maxNumChannels );
			// Disabling ALL the sensors would be silly... so prevent it
			if( enabledChannelMask == 0 ) {
				termSetFor( TERM_DEBUG );
				Serial.println(F("Cannot disable ALL the sensors!"));
				termSetFor ( TERM_DEFAULT );
				// Toggle it back again...
				toggleMaskBit( &enabledChannelMask, readval - 1, maxNumChannels );
			}
		} while ( readval > 0 && readval <= maxNumChannels );
		// Set the enable state for all sensors
		enableChannels( enabledChannelMask, maxNumChannels );
		numChannels = getNumChannels( enabledChannelMask );
		// finish by showing the updated enabled sensor list
		termSetFor( TERM_CONFIRM );
		printChannelList();
		Serial.print(F("Enabled: "));
		printEnabledStatus( enabledChannelMask, maxNumChannels ); // includes a CR
		blinkLED( LED, 2, 6 );
		break; 

	case 'P':    // P  = Set the serial port speed
	case 'p':
		Serial.println( char( commandByte ) );
		readval = 9; // Set the value to be read deliberately out of range. (0 is valid value)
		do {
			termSetFor( TERM_INPUT );
			Serial.print(F(" -> Enter serial port speed (0=2400/1=4800/2=9600) [0/1/2]: "));
			termSetFor( TERM_DEFAULT );
			readval = vtSerial.ReadLong();       // read integer
			Serial.println("");
		} while ( readval < 0 || readval > 2 );
		serialSpeed = readval;
		termSetFor( TERM_CONFIRM );
		Serial.print(F("Serial Port Speed: "));
		Serial.println( serialSpeeds[serialSpeed] );
		if ( EEPROM.read( EEPROM_SERIAL_SPEED ) != byte( serialSpeed ) ) {
			EEPROM.write( EEPROM_SERIAL_SPEED, byte( serialSpeed ) ); // Save to EEPROM
		}
		Serial.println(F("Serial speed set in EEPROM, please reboot interface"));
		// blink the LED to indicate we got a valid command
		blinkLED( LED, 2, 7 ); 
		break;
		
	case 'D':    // D  = Set debug mode ON, or cycle debug level 1 to maxDebugLevel
		Serial.println( char( commandByte ) );
		if ( ! debugMode ) {
			// Turn on debug mode
			debugMode = true;
			termSetFor( TERM_DEBUG );
			Serial.println(F("Debug Mode ON"));
			// Debug flags may already be set, so list them
			printChannelList();
			Serial.print(F("DebugEn: "));
			printEnabledStatus( debugChannelMask, maxNumChannels );
			Serial.println(F("Debug Flags Set: "));
			printDebugFlagsSet();
			// blink the LED to indicate we got a valid command
			blinkLED( LED, 2, 8 );
		}
		else
		{
			// Debug mode is already on, so set the debug flags from 0 to maxDebugFlags
			// For each flag from 0 to maxDebugFlags-1
			// Prompt to toggle this flag by name
			// Get response, 1 = on, 0 = off
			// bitWrite the value in the debugFlagsMask
			// Note that you can turn all the flags off and still be in debug mode.
			for ( i = 0; i < maxDebugFlags; i++ ) {
				readval = 9; // Set the value to be read deliberately out of range.
				do {
					termSetFor( TERM_INPUT );
					Serial.print(F(" -> Enable "));
					printDebugFlagName( i );
					Serial.print(F(" (0=Off/1=On) [0/1]: "));
					termSetFor( TERM_DEFAULT );
					readval = vtSerial.ReadLong();       // read integer
					Serial.println("");
				} while ( readval < 0 || readval > 1 );
				if ( readval == 0 ) {
					// Clear the bit
					bitWrite( debugFlagsMask, i, 0 );
				}
				else if ( readval == 1 ) {
					// Set the bit
					bitWrite( debugFlagsMask, i, 1 );
				}
			}
			termSetFor( TERM_DEBUG );
			Serial.println(F("Debug Flags Set: "));
			printDebugFlagsSet();
			// just a quick blink so as not to slow the CLI down too much when cycling
			blinkLED( LED, 0, 1 ); //one quick
		} //! debugMode
		break;
		
	case 'd':    // d  = Set debug mode OFF
		Serial.println( char( commandByte ) );
		if ( debugMode ) {
			debugMode = false;
			termSetFor( TERM_DEBUG );
			Serial.println(F("Debug Mode OFF"));
			// blink the LED to indicate we got a valid command
			blinkLED( LED, 2, 9 );
			// Don't change the debug level here, remember it in case debugging is turned back on
		} //debugMode
		break;

	case 'M':    // M  = Toggle bits in the debug channel mask (1 = on, 0 = off)
	case 'm':
		// Debug mode only
		Serial.println( char( commandByte ) );
		if ( debugMode ) {
			readval = 0; // Set the value to be read deliberately out of range.
			do {
				termSetFor( TERM_INPUT );
				// show the current mask, LSB (treble) first
				printChannelList();
				Serial.print(F("DebugEn: "));
				printEnabledStatus( debugChannelMask, maxNumChannels ); // includes a CR
				// Prompt the user for a bell number to toggle on or off
				Serial.print(F(" -> Toggle channel value [1-"));
				Serial.print( maxNumChannels );
				Serial.print(F(", 0=Done]: "));
				termSetFor( TERM_DEFAULT );
				readval = vtSerial.ReadLong(); // read integer ( expecting 1 - 16, 0 to finish)
				Serial.println("");
				// Toggle the channel bit in the debug mask by XOR-ing the current value it with 1
				// (remembering that bell numbers are 1-16, bits/channels are 0-15). Max 16 bits.
				toggleMaskBit( &debugChannelMask, readval - 1, maxNumChannels );
			} while ( readval > 0 && readval <= maxNumChannels );
			// finish by showing the updated bell mask
			termSetFor( TERM_DEBUG );
			printChannelList();
			Serial.print(F("DebugEn: "));
			printEnabledStatus( debugChannelMask, maxNumChannels );	  
			blinkLED( LED, 2, 10 );
		}
		break; 

	case '0' ... '9':    // 0...9  = Debug Markers. Label range wheeze from the Arduino forums
		// Debug mode only
		if ( debugMode ) { 
			Serial.print (F("DEBUG MARKER "));
			Serial.println( char( commandByte ) );
		} else {
			Serial.println( char( commandByte ) );
			// just a quick blink so as not to slow the CLI down too much
			blinkLED( LED, 0, 1 ); //one quick
		}       
		break;

	case 'L':    // L  = Debug Levels Help
	case 'l':
		// Debug mode only
		Serial.println( char( commandByte ) );
		if ( debugMode ) {
			showCLIDebugHelp();
		}
		// just a quick blink so as not to slow the CLI down too much
		blinkLED( LED, 0, 1 ); //one quick
		break;

	case 'Z':    // Z  = Default settings
	case 'z':
		// Debug mode only
		Serial.println( char( commandByte ) );
		if ( debugMode ) {
			defaultSettings();
		}
		// just a quick blink so as not to slow the CLI down too much
		blinkLED( LED, 0, 1 ); //one quick
		break;
		
	case 'H':    // H  = Help
	case 'h':
		Serial.println( char( commandByte ) );
		showCLIHelp();
		// just a quick blink so as not to slow the CLI down too much
		blinkLED( LED, 0, 1 ); //one quick
		break;
		
	case 'T':    // T  = Test Mode
		Serial.println( char( commandByte ) );
		termSetFor( TERM_CONFIRM );
		Serial.print(F("Test Mode in "));
		Serial.print(testStartDelay);
		Serial.print(F(" seconds..."));
		termSetFor( TERM_DEFAULT );
		digitalWrite( LED, HIGH);
		delay(testStartDelay * 1000); //testStartDelay is in seconds.
		// put all the channels into test mode
		for ( i = 0; i < maxNumChannels; i++ ) {
			channelMachineState[i] = TEST_MODE;
		}
		break; 

	case 'R':    // R  = Remap the channel numbers to output signals
	case 'r':
		// prompt the user for a channel number (as 1-to-maxNumChannels)
		// show the current mapped bell character for that channel number
		// prompt for a new mapped char (check validity)
		// update the active map for that channel & warn about duplicates
		
		Serial.println( char( commandByte ) );

		readval = -1; // Set the value to be read deliberately out of range.
		do {
			termSetFor( TERM_INPUT );
			// show the current mapping
			printChannelList();
			showActiveMapping();
			// Prompt the user for a sensor number to change
			Serial.print(F(" -> Remap channel number [1-"));
			Serial.print( maxNumChannels );
			Serial.print(F(", 0=Done]: "));
			readval = vtSerial.ReadLong(); // read integer ( expecting 1 - 16, 0 to finish)
			Serial.println("");
			
			if ( readval > 0 && readval <= maxNumChannels && bitRead( enabledChannelMask, readval - 1 ) ) {
				// it's a valid sensor number 1-16, and the channel (0-15) is enabled,
				// show the current mapping (remember readval is sensors 1-16, array is index 0-15)
				Serial.print(F("Current Mapping: Channel "));
				Serial.print( readval );
				Serial.print(F(" = Bell "));			
				Serial.println( bellStrikeChar[readval - 1] );

				// prompt for a new mapped bell code (check validity)	   
				char readNewMapping[2]; //see vtSerial source
				// set a deliberately invalid mapping
				readNewMapping[0] = '-';
				do {
					Serial.print(F(" -> New Mapping [1-90ETABCDWXYZ]: "));
					vtSerial.ReadText(readNewMapping,1);
					Serial.println("");
				} while ( !checkValidMappingChar( readNewMapping[0] ) );
				
				// We have a valid sensor number, and a valid mapping output character,
				// update the mapping entry in bellStrikeChar[].
				bellStrikeChar[readval - 1] = readNewMapping[0];
				
				//END OF REMAP BLOCK
			}		
			
		} while ( readval != 0 );

		termSetFor( TERM_CONFIRM );
		showActiveMapping();
		// Check for duplicates and warn
		duplicateMapCheck();
		blinkLED( LED, 2, 11 );
		break; 

	default:
		// So this isn't an MBI command, not bell delay data, and it's not a non-MBI CLI command.
		// We don't know what this is, echo it as a char, flash the LED, and throw the data away.
		// Note than any other bytes that were read into the buffer are never looked at either.
		Serial.println( char( commandByte ) );
		blinkLED( LED, 3, 0 );
		
	} //switch commandByte

	// Check for unsaved EEPROM settings
	checkSavedSettings();
	// Show the CLI again
	showCLI();
	
}

/*
*********************************************************************************************
*                                 Function showCLIHelp()                                    *
*********************************************************************************************
*/

// Display the CLI help text.
void showCLIHelp(void) {

	// Position the cursor and clear the screen.
	termClear();

	// Keep it short, this is all static literal data!
	termSetFor( TERM_HEADING );
	Serial.println(F("CLI Commands:"));
	termSetFor( TERM_DEFAULT );

	Serial.println(F(" -> [B] - Set debounce timer (1ms->20ms)"));
	Serial.println(F(" -> [G] - Set guard timer (1cs->50cs)"));
	Serial.println(F(" -> [E] - Enable/Disable a channel"));
	Serial.println(F(" -> [S] - Save settings in EEPROM"));
	Serial.println(F(" -> [P] - Set serial port speed in EEPROM"));
	Serial.println(F(" -> [R] - Remap channel numbers to bell values"));
	Serial.println(F(" -> [D] - Turn debug mode ON or change debug flags"));
	Serial.println(F(" -> [d] - Turn debug mode OFF"));
	Serial.println(F(" -> [M] - Enable/Disable channel debugging (debug mode only)"));
	Serial.println(F(" -> [0-9] - Print debug markers (debug mode only)"));
	Serial.println(F(" -> [Z] - Default settings (debug mode only)"));
	Serial.println(F(" -> [L] - Display debug levels help text (debug mode only)"));
	Serial.println(F(" -> [H] - Display this help text"));
	Serial.println(F(" -> [T] - Enter test mode (reset to exit)"));
	Serial.println(F(" -> [?] - Display current settings"));

}

/*
*********************************************************************************************
*                                Function showCLIDebugHelp()                                *
*********************************************************************************************
*/

// Display the CLI debug levels help text.
void showCLIDebugHelp(void) {

	// Position the cursor and clear the screen.
	termClear();

	// Keep it short, this is all static literal data!
	termSetFor( TERM_HEADING );
	Serial.println(F("Debug Flags:"));
	termSetFor( TERM_DEFAULT );
	int i;
	
	for ( i = 0; i <maxDebugFlags; i++ ) {
		// Dump the debug levels.
		Serial.print(F(" ["));
		Serial.print( i );
		Serial.print(F("] -> "));
		printDebugFlagName( i );
		Serial.println("");
	}

}

/*
*********************************************************************************************
*                                   Function printMask()                                    *
*********************************************************************************************
*/

// Print a mask (e.g. debugMask or enabledSensorMask) as fixed and spaced width binary, LSB
// (usually treble) first, showing a fixed number of bits only.
void printMask( word thisMask, int places )
{
	int i;
	for ( i = 0; i < places; i++ ) {
		// Print the value of the bits.
		Serial.print( bitRead( thisMask, i ) );
		Serial.print(F(" "));
	}
	Serial.println(""); //new line
}

/*
*********************************************************************************************
*                               Function printEnabledStatus()                               *
*********************************************************************************************
*/

// Print the enabledSensorMask) as fixed and spaced "Y" or "-", LSB
// (channel 1) first, showing a fixed number of bits only.
void printEnabledStatus( word thisMask, int places )
{
	int i;
	
	for ( i = 0; i < places; i++ ) {
		// Print the value of the bits.
		if ( bitRead( thisMask, i ) )
		{
			Serial.print(F("Y"));
		}
		else
		{
			Serial.print(F("-"));
		}
		Serial.print(F("  "));
	}

	Serial.println(""); //new line
	
}

/*
*********************************************************************************************
*                                Function toggleMaskBit()                                   *
*********************************************************************************************
*/

// Toggle the value of the specified bit in a mask by XOR-ing it with 1, e.g. to Enable or
// disable a sensor or debug a bell. Bits are passed as 0 to 15, if you need to convert from
// a bell number, do it before passing the bit here. Pass the mask as a pointer so the
// original data gets updated. Don't bother trying to toggle a bit >maxBits.
boolean toggleMaskBit( word *thisMask, int thisBit, int maxBits )
{
	if ( thisBit >= 0 && thisBit < maxBits ) {
		bitWrite( *thisMask, thisBit, ( bitRead ( *thisMask, thisBit ) ^ 1 ));
		return true;
	}
	else
	{
		return false;
	}
}


/*
*********************************************************************************************
*                                 Function enableChannels()                                  *
*********************************************************************************************
*/

// Run through the enabledChannelMask, and set the state machine for each sensor to be
// WAIT_FOR_INPUT or SENSOR_DISABLED based on the bit value (1 = enabled, 0 = disabled).
void enableChannels( word thisMask, int maxBits )
{
	int i;
	for ( i = 0; i < maxBits; i++ ) {
		if ( bitRead( thisMask, i ) == 1 ) {
			// Enable the sensor
			channelMachineState[i] = WAIT_FOR_INPUT;
		}
		else
		{
			// Disable the sensor
			channelMachineState[i] = SENSOR_DISABLED;
		}
	}

}

/*
*********************************************************************************************
*                            Function checkValidMappingChar()                               *
*********************************************************************************************
*/

// Check that the new remapping character supplied by the user is valid, by seeing if it
// exists in the list validMappedChars[].

int checkValidMappingChar( char testchar )
{

	//The mapped char should be listed in the array validMappedChars[]
	int i;
	//scan the validMappedChars[]
	for ( i = 0; i < sizeof( validMappedChars ) - 1; i++ ) {
		if( testchar == validMappedChars[i] ) {
			//found the character, it's valid
			return true;
			//no need to look any further down the array
			break;
		}
	}
	// If we get here the testchar wasn't found, so it's invalid
	return false;
	
}


/*
*********************************************************************************************
*                               Function printChannelList()                                 *
*********************************************************************************************
*/

//Dumps the actual live remapping values.

void printChannelList( void )
{
	//loop counter
	int i;
	
	Serial.print(F("Channel: "));

	//Print the input characters for reference
	for ( i = 0; i < maxNumChannels; i++ ) {
		Serial.print( i + 1 ); // shown as 1-16
		Serial.print(F(" "));
		if ( i < 9 ) { Serial.print(F(" ")); }
	}
	Serial.println(""); //new line

}


/*
*********************************************************************************************
*                               Function showActiveMapping()                                *
*********************************************************************************************
*/

//Dumps the actual live remapping values.

void showActiveMapping( void )
{
	//loop counter
	int i;
	
	//What are the active settings?
	Serial.print(F("Mapping: "));

	//Print the input characters for reference
	for ( i = 0; i < maxNumChannels; i++ ) {
		// Hide mapping for disabled channels
		if ( bitRead( enabledChannelMask, i ) ) {
			Serial.print( bellStrikeChar[i] );
		}
		else
		{
			Serial.print(F("-"));
		}
		Serial.print(F("  "));
	}
	Serial.println(""); //new line
	
}


/*
*********************************************************************************************
*                               Function showSensorInputs()                                 *
*********************************************************************************************
*/

//Dumps the actual live sensor inputs.

void showSensorInputs( void )
{
	//loop counter
	int i;
	
	// What are the most recent sensor values?
	Serial.print(F("Inputs : "));
	
	for ( i = 0; i < maxNumChannels; i++ ) {
		// Hide mapping for disabled channels, the last value makes no sense if the
		// pin is not even being scanned.
		if ( bitRead( enabledChannelMask, i ) ) {
			// Dump the last read values.
			Serial.print( channelLastValue[i] );
		}
		else
		{
			Serial.print(F("-"));
		}
		Serial.print(F("  "));
	}
	Serial.println(""); //new line

}


/*
*********************************************************************************************
*                               Function duplicateMapCheck()                                *
*********************************************************************************************
*/

// Check the bellStrikeChar[] array mapping for duplicates, and warn if any exist.

boolean duplicateMapCheck( void )
{
	// Check for duplicates and warn
	// Don't bother looking at mappings above the maxNumChannels ceiling
	for ( int p = 0; p < maxNumChannels - 1; p++ ) {
		for ( int q = p + 1; q < maxNumChannels; q++ ) {
			// If p and q are enabled channels, and the mapping for p and q are the same,
			// then flag the duplicate and bail out.
			if ( bitRead( enabledChannelMask, p ) && bitRead( enabledChannelMask, q ) && bellStrikeChar[p] == bellStrikeChar[q] ) {
				termSetFor( TERM_DEBUG );
				Serial.println(F("Duplicate mappings defined"));
				termSetFor( TERM_DEFAULT );
				return false;
			}
		}
	}
	// If we get here then no duplicates were found, return true
	return true;
}

/*
*********************************************************************************************
*                                Function defaultSettings()                                 *
*********************************************************************************************
*/

// Load the default settings, but do not save to EEPROM

void defaultSettings( void )
{
	// generic loop counter
	int i;
	
	//simulatorType = 'A';
	numChannels = maxNumChannels;
	debounceDelay = defaultDebounceDelay;
	guardDelay = defaultGuardDelay;
	serialSpeed = defaultSerialSpeed;
	enabledChannelMask = defaultMask;
	debugChannelMask = defaultMask;
	for ( i = 0; i < maxNumChannels; i++ ) { // always 16 bytes
		bellStrikeChar[i] = defaultBellStrikeChar[i];
	}
	
	enableChannels( enabledChannelMask, maxNumChannels );
	termSetFor( TERM_CONFIRM );
	Serial.println(F("Defaults Set (remember to Save)"));
	termSetFor( TERM_DEFAULT );
	
}


/*
*********************************************************************************************
*                                Function getNumChannels()                                  *
*********************************************************************************************
*/

// Calcuate the value of numChannels from the highest enabled sensor in the enabledChannelMask.
// Pass the enabledChannelMask not as a pointer (unlike in toggleMaskBit), because the value in the
// function is going to get bitshifted and therefore lost.
int getNumChannels( word thisMask )
{
	// Starting value to return, decrementing counter
	int i = 16;
	
	// The code below relies on there always being at least one bit set in the
	// enabledChannelMask. This should be enforced elsewhere, but trapped here as well to
	// stop the code looping forever.
	if ( thisMask == 0 ) {
		return i;
	}
	
	// While the MSB of enabledChannelMask is not a 1, left shift the enabledChannelMask
	// and decrement the potential return value. (NB a 0 bit is logical false)
	while( ! bitRead ( thisMask, maxNumChannels - 1 ) ) {
		// This bit wasn't set, try the next one...
		// Shift mask left, decrement counter
		thisMask <<= 1;
		i--;
	}

	if ( debugMode ) {
		Serial.print(F("getNumChannels Returned "));
		Serial.println(i);
	}	
	
	return i;	
}
