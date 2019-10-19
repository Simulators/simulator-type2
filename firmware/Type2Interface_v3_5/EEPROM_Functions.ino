/*
Liverpool Ringing Simulator Project
Simulator Interface v3.5 Beta
EEPROM Functions

Copyright 2014-2019 Andrew J Instone-Cowie.

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
*                                 Function saveToEEPROM()                                   *
*********************************************************************************************
*/

// Save the following EEPROM locations:
// 	Location  EEPROM_DEBOUNCE_DELAY	: Debounce Timer (in ms)
// 	Location  EEPROM_ENABLE_MASK_LO	: enabledChannelMask (lo byte)
// 	Location  EEPROM_ENABLE_MASK_HI	: enabledChannelMask (hi byte)
// 	Location  EEPROM_DEBUG_MASK_LO	: debugChannelMask (lo byte)
// 	Location  EEPROM_DEBUG_MASK_HI	: debugChannelMask (hi byte)
//  Location  EEPROM_GUARD_DELAY	: Guard Timer (in cs)
//  Location  EEPROM_MAPPING_BASE	: Output mappings (16 bytes)

// Don't save the following:
// 	Location  EEPROM_SERIAL_SPEED	: Serial port speed - Saved by the P option and requires a reset
// 	Location  EEPROM_SIMULATOR_TYPE	: No longer used
// 	Location  EEPROM_NUMCHANNELS	: No longer used

void saveToEEPROM( void )
{
	termSetFor( TERM_CONFIRM );
	
	// generic loop counter
	int i;

	// Only save the byte if different from the current value, to reduce EEPROM write wear	

	if ( EEPROM.read( EEPROM_DEBOUNCE_DELAY ) != byte( debounceDelay ) ) {
		EEPROM.write( EEPROM_DEBOUNCE_DELAY, byte( debounceDelay ) ); 
		// Save debounceDelay to location EEPROM_DEBOUNCE_DELAY
	}
	Serial.println(F("Debounce timer saved to EEPROM"));

	if ( EEPROM.read( EEPROM_GUARD_DELAY ) != byte( guardDelay ) ) {
		EEPROM.write( EEPROM_GUARD_DELAY, byte( guardDelay ) ); 
		// Save guardDelay to location EEPROM_GUARD_DELAY
	}
	Serial.println(F("Guard timer saved to EEPROM"));
	
	if ( EEPROM.read( EEPROM_ENABLE_MASK_LO)  != lowByte( enabledChannelMask ) ) {
		EEPROM.write( EEPROM_ENABLE_MASK_LO, lowByte( enabledChannelMask ) );
	}
	if ( EEPROM.read( EEPROM_ENABLE_MASK_HI ) != highByte( enabledChannelMask ) ) {
		EEPROM.write( EEPROM_ENABLE_MASK_HI, highByte( enabledChannelMask ) );
	}
	Serial.println(F("Enabled channels saved to EEPROM"));
	
	for ( i = 0; i < maxNumChannels; i++ ) { // always 16 bytes
		if ( EEPROM.read( EEPROM_MAPPING_BASE + i ) != bellStrikeChar[i] ) {
			EEPROM.write( ( EEPROM_MAPPING_BASE + i ), byte( bellStrikeChar[i] ) );
			//Save the 16 bellStrikeChar[] starting at  EEPROM_MAPPING_BASE
		}
	}
	Serial.println(F("Channel/Bell mappings saved to EEPROM"));
	duplicateMapCheck();
	
	if ( debugMode ) {
		termSetFor( TERM_CONFIRM );
		if ( EEPROM.read( EEPROM_DEBUG_MASK_LO ) != lowByte( debugChannelMask ) ) {
			EEPROM.write( EEPROM_DEBUG_MASK_LO, lowByte( debugChannelMask ) );
		}
		if ( EEPROM.read( EEPROM_DEBUG_MASK_HI ) != highByte( debugChannelMask ) ) {
			EEPROM.write( EEPROM_DEBUG_MASK_HI, highByte( debugChannelMask ) );
		}
		Serial.println(F("Debug mask saved to EEPROM"));
	}
	termSetFor( TERM_DEFAULT );

}

/*
*********************************************************************************************
*                                Function loadFromEEPROM()                                  *
*********************************************************************************************
*/

// Load the following EEPROM locations:
// 	Location  EEPROM_DEBOUNCE_DELAY	: Debounce Timer (in ms)
// 	Location  EEPROM_SERIAL_SPEED	: Serial port speed (as an index into serialSpeeds[])
// 	Location  EEPROM_ENABLE_MASK_LO	: enabledChannelMask (lo byte)
// 	Location  EEPROM_ENABLE_MASK_HI	: enabledChannelMask (hi byte)
//	Location  EEPROM_DEBUG_MASK_LO	: debugBellMask (lo byte)
// 	Location  EEPROM_DEBUG_MASK_HI	: debugBellMask (hi byte)
//  Location  EEPROM_GUARD_DELAY	: Guard Timer (in cs)
//  Location  EEPROM_MAPPING_BASE	: Output mappings (16 bytes)

// Don't load the following:
// 	Location  EEPROM_SIMULATOR_TYPE	: No longer used
// 	Location  EEPROM_NUMCHANNELS	: No longer used

void loadFromEEPROM( void )
{
	// No point printing messages, the serial port isn't started yet...

	// generic loop counter
	int i;
	
	// flag that the EEPROM contains some out of range values, and therefore probably has
	// not been set and saved.
	boolean dirtyEEPROM = false;

	// initialise the debounce timer from EEPROM. If the value is <1 or >20, set it to default.
	// Cast the byte value read into a long so we can add it to millis().
	debounceDelay = long( EEPROM.read( EEPROM_DEBOUNCE_DELAY ) );
	// but the EEPROM value may not be set to a sensible value! Seed a sane value.
	if ( debounceDelay < 1 || debounceDelay > maxDebounceDelay ) {
		debounceDelay = defaultDebounceDelay;
		dirtyEEPROM = true;
	}

	// initialise the guard timer from EEPROM. If the value is <1 or >50, set it to default.
	// This is in cs, not ms. Cast the byte value read into a long so we can add it to millis().
	guardDelay = long( EEPROM.read( EEPROM_GUARD_DELAY ) );
	// but the EEPROM value may not be set to a sensible value! Seed a sane value.
	if ( guardDelay < 1 || guardDelay > maxGuardDelay ) {
		guardDelay = defaultGuardDelay;
		dirtyEEPROM = true;
	}
	
	// initialise the serial port speed from EEPROM. If the value is <1 or >2, set it to
	// default. Cast the byte value read into a long for Serial.begin.
	serialSpeed = int( EEPROM.read( EEPROM_SERIAL_SPEED ) );
	// but the EEPROM value may not be set to a sensible value! Seed a sane value.
	if ( serialSpeed < 0 || serialSpeed > 2 ) {
		serialSpeed = defaultSerialSpeed;
		// Silently update the EEPROM with the default, to avoid issues with checkSavedSettings()
		EEPROM.write( EEPROM_SERIAL_SPEED, byte( defaultSerialSpeed ) ); // Save to EEPROM
	}

	// Try loading the output mapped character array. Test each character for validity.
	for ( i = 0; i < maxNumChannels; i++ ) { // always 16 bytes
		//The mapped char should be listed in the array validMappedChars[]
		char tempChar;
		tempChar = (char) EEPROM.read( EEPROM_MAPPING_BASE + i ); // Cast byte to char
		//scan the validMappedChars[]
		if( checkValidMappingChar( tempChar )) {
			bellStrikeChar[i] = tempChar;
		}
		else
		{
			dirtyEEPROM = true;
		}			
	}

	// There is no point trying to sanity check these values, any 16-bit quantity is potentially valid,
	// apart from zero (disabling all sensors is not allowed)
	// If at least one other EEPROM value is out of range do we set a default.	
	if ( dirtyEEPROM ) {
		enabledChannelMask = defaultMask;
		debugChannelMask = defaultMask;
	} 
	else
	{
		// Initialise the enabled sensor mask and debug channel mask from EEPROM, assembing the word
		// from the high and low bytes.
		enabledChannelMask = word( EEPROM.read( EEPROM_ENABLE_MASK_HI ), EEPROM.read( EEPROM_ENABLE_MASK_LO ) );
		debugChannelMask = word( EEPROM.read( EEPROM_DEBUG_MASK_HI ), EEPROM.read( EEPROM_DEBUG_MASK_LO ) ); //hi, lo
		// if all the sensors are disabled, set it to the default.
		if( enabledChannelMask == 0 ) {
			enabledChannelMask = defaultMask;
		}
	}

	// Set the default output character mappings.	
	if ( dirtyEEPROM ) {
		// Set the default output signals at startup if EEPROM settings are not good
		for ( i = 0; i < maxNumChannels; i++ ) { // always 16 bytes
			bellStrikeChar[i] = defaultBellStrikeChar[i];
		}
	}
	
}

/*
*********************************************************************************************
*                              Function checkSavedSettings()                                *
*********************************************************************************************
*/

// Compare live (RAM) and saved (EEPROM) settings.

boolean checkSavedSettings( void )
{
	// Returning TRUE means the EEPROM matches live, FALSE it doesn't match.
	// This function gets called in handleCLI();

	// generic loop counter
	int i;
	
	// flag that the EEPROM contains some out of range values, and therefore probably has
	// not been set and saved.
	boolean unsavedSettings = false;

	// Check the debounce timer.
	if ( debounceDelay != long( EEPROM.read( EEPROM_DEBOUNCE_DELAY ) ) ) {
		unsavedSettings = true;
	}
	
	// Check the guard timer
	if ( guardDelay != long( EEPROM.read( EEPROM_GUARD_DELAY ) ) ) {
		unsavedSettings = true;
	}
		
	// Check the serial speed.
	if ( serialSpeed != int( EEPROM.read( EEPROM_SERIAL_SPEED ) ) ) {
		unsavedSettings = true;
	}
	
	// Check the output mapping array
	for ( i = 0; i < maxNumChannels; i++ ) { // always 16 bytes
		//The mapped char should be listed in the array validMappedChars[]
		if ( bellStrikeChar[i] != (char) EEPROM.read( EEPROM_MAPPING_BASE + i ) ) {
			unsavedSettings = true;
		}
	}
	
	// Check the enabledChannelMask
	if ( enabledChannelMask != word( EEPROM.read( EEPROM_ENABLE_MASK_HI ), EEPROM.read( EEPROM_ENABLE_MASK_LO ) ) ) {
		unsavedSettings = true;
	}
	
	// Check the debugBellMask
	// Only flag this as unsaved if debug mode is active, as the debugChannelMask only gets
	// saved if debugMode is active.
	if ( debugMode && ( debugChannelMask != word( EEPROM.read( EEPROM_DEBUG_MASK_HI ), EEPROM.read( EEPROM_DEBUG_MASK_LO ) ) ) ) {
		unsavedSettings = true;
	}

	// Warn about unsaved EEPROM settings
	if ( unsavedSettings ) {
		termSetFor( TERM_DEBUG );
		Serial.println(F("Settings not saved!"));
		termSetFor( TERM_DEFAULT );
	}
	
	return unsavedSettings;
	
}

/*
*********************************************************************************************
*                              Function dumpEEPROMMappings()                                *
*********************************************************************************************
*/

//Dump the EEPROM mapped characters from locations 10-25.

void dumpEEPROMMappings( void )
{
	//loop counter
	int i;
	
	Serial.print(F("EEPROM Mapping: "));
	for ( i = 0; i < maxNumChannels; i++ ) {
		Serial.print( (char) EEPROM.read( EEPROM_MAPPING_BASE + i ));
		Serial.print(F(" "));
	}
	Serial.println(""); //new line
}

