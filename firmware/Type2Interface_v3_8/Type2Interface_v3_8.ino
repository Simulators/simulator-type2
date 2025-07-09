/*
Liverpool Ringing Simulator Project
Simulator Interface v3.8

Copyright 2014-2025 Andrew J Instone-Cowie.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Tested against Abel 10.3.2, Beltower 14.05 (2025), Virtual Belfry 3.10.

	3.1 : Dedicated Type 2 (RJ45) Version.
	3.2 : Added remapping code (largely ex SplitterBox).
		  Added CLI option Z to load default settings.
		  CLI detection of unsaved EEPROM settings.
		  Remove quirks mode (Option Q) & associated code.
		  Change default debounce timer to 2ms.
		  Disambiguate channels from bells throughout CLI.
		  Make CLI non case sensitive (apart from Options D/d & T).
		  Improve CLI formatting, really disable disabled channels.
		  Abel 3.10.0, Virtual Belfry 3.5.
		  Measure debounce timer in microseconds to improve consistency.
		  First GitHub release in simulator-type2 repo.
	3.3 : Simplify CLI. Remove requirement to set numChannels and save it to EEPROM,
		  by deriving the value from enabledChannelMask in getNumChannels.
	3.4 : Test mode selection, additional test mode ("firing").
	3.5 : Fix a very old bug in WAIT_FOR_DEBOUNCE.
	3.6 : Test Mode uses the calculated numChannels from enabledChannelMask.
		  Fix Test Mode startup menu crunch.
		  Improve Test Mode CLI.
	3.7 : Add support for MBI protocol 0xFD command for Abel 10.3.2.
	3.8 : Add swing delta timing as an additional debug option.
		  Change debug message format to have identifying letter first.
		  Add time since last pulse to pulse timer (S) debug message.
		  Tested against Beltower 14.05.
		  
*/

/*
ATmega328P fuse settings for 8MHz internal clock:
(Type 2 Interface Board Rev D and earlier)
low_fuses=0xE2
	SUT0
	CKSEL3
	CKSEL2
	CKSEL0
high_fuses=0xDF
	SPIEN
extended_fuses=0xFD
	BODLEVEL1
	
ATmega328P fuse settings for 8MHz external ceramic resonator:
(Type 2 Interface Board Rev E and later)
low_fuses=0xCF
	SUT0
	SUT1
high_fuses=0xDF
	SPIEN
extended_fuses=0xFD
	BODLEVEL1

The appropriate set of fuses is selected in the Arduino IDE by selecting the
correct board type in the Liverpool Ringing Simulator Boards set.

Unlike the previous hardware, we do now clear the EEPROM on load (high fuse 0xdF instead of
0xd7), so we can be confident of getting a workable startup config even if the interface has
not been configured. See loadFromEEPROM() and defaulMask.

*/

// Simulator Interface Hardware Type
//  - Type 2 hardware is the RJ45 interface board.
//  - The IDE sets up a #define ARDUINO_AVR_SIMULATOR_TYPE2
//#define ARDUINO_AVR_SIMULATOR_TYPE2

// Uncomment the following line to use a pin as a CRO timing pin.
// This should be commented out in production code.
//#define USE_CRO

// Include the free memory library, freeMemory() is called by dumpdata().
#include <MemoryFree.h>

// Include the VTSerial library for VT100-style input functions.
#include <VTSerial.h>

// Initialise the VTSerial library.
VTSerial vtSerial;

// Include attributes file for the terminal handling functions.
#include "Terminal_Functions.h"

// Non-volatile configuration data are stored in EEPROM:
// Location  1    : No longer used
// Location  2    : No longer used
// Location  3    : Debounce Timer (in ms)
// Location  4    : Serial port speed (as an index into serialSpeeds[])
// Location  5    : enabledChannelMask (lo byte)
// Location  6    : enabledChannelMask (hi byte)
// Location  7    : debugChannelMask (lo byte)
// Location  8    : debugChannelMask (hi byte) 
// Location  9    : Guard Timer (in cs)

// Define the EEPROM locations to make the code more readable.
// #define EEPROM_SIMULATOR_TYPE 1
// #define EEPROM_NUMCHANNELS 2
#define EEPROM_DEBOUNCE_DELAY 3
#define EEPROM_SERIAL_SPEED 4
#define EEPROM_ENABLE_MASK_LO 5
#define EEPROM_ENABLE_MASK_HI 6
#define EEPROM_DEBUG_MASK_LO 7
#define EEPROM_DEBUG_MASK_HI 8
#define EEPROM_GUARD_DELAY 9
#define EEPROM_MAPPING_BASE 10 //10-25 are used for remapped output signals

#include <EEPROM.h>

// Software version
const int majorVersion = 3;
const int minorVersion = 8;

// -------------------------------------------------------------------------------------------
//                                    Core Simulator
// -------------------------------------------------------------------------------------------

// Define the maximum number of channels that can be set on the hardware. For the Type 2 
// interface this is 16.
const int maxNumChannels = 16;

// Define the number of the highest input channel to scan (always starting at 0, held in EEPROM).
// Not scanning unused channels avoids issues with spurious signals being detected on
// floating inputs (even with INPUT_PULLPUP). Most data structures, etc are set up for this
// number of channels.
int numChannels; // initialised from enabledChannelMask after EEPROM load in setup

#ifdef ARDUINO_AVR_SIMULATOR_TYPE2
const int channelSensorPin[] = { 6, 7, 8 ,9, 10, 11, 12, 13, 14, 3, 15, 2, 16, 17, 18, 19 };
//14-19 aka A0-A5
#endif

// Set up an array of the characters sent on the serial port to the simulator software, as
// defined in the MBI interface specification. These are ASCII characters. 
// These may be overwritten from EEPROM, see loadFromEEPROM();
char bellStrikeChar[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'E', 'T', 'A', 'B', 'C', 'D' };

// Define the default serial characters in case the EEPROM is not yet set sensibly.
const char defaultBellStrikeChar[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'E', 'T', 'A', 'B', 'C', 'D' };

// Set up an array to store the last observed sensor status, so we can detect transitions
// in input state. This is important to avoid duplicated sensor signals. 
int channelLastValue[maxNumChannels]; //initialise these in setup, and set them LOW (see below)

// Set up an array of the permitted serial port speeds. Note that Abel and Beltower
// (and the MBI protocol specification) REQUIRE 2400 bps. Other speeds may be useful only
// for debugging where the text output can overrun the serial transmit buffer and cause
// timing issues. Also declare an int to hold the indexed value, and a sane default.
unsigned long serialSpeeds[] = { 2400, 4800, 9600 };
int serialSpeed = 0; // index into serialSpeeds[], not the actual bps, loaded from EEPROM 
const long defaultSerialSpeed = 0; // 2400 - required by the simulators

// Set up an Enabled Sensor Mask variable as a 16 bit unsigned word. The bits
// (0-15) correspond to the sensor being enabled on that channel (1=true=on, 0=false=off).
// LSB = Channel 0 (usually treble, or sensor 1).
word enabledChannelMask; // initialised from EEPROM in setup. See option E.

// Define the default mask to use if the EEPROM is not sane. This is used for both the
// enabledSensorMask and debugBellMask.
const word defaultMask = 65535;

// -------------------------------------------------------------------------------------------
//                                   Sensor Debounce
// -------------------------------------------------------------------------------------------

// Set up an array to hold the time that the debounce timer will expire, one per channel. The
// data type is unsigned long, because these values will be the sum of debounceDelay and
// the output of micros().
unsigned long channelDebounceEndTime[maxNumChannels]; // initialise these in setup()

// Set up a variable to hold the debounce delay time, in milliseconds.
// Note that the debounce timer is stored and presented in milliseconds, but observed
// in microseconds. This improves accuracy where the number of milliseconds is very small.
unsigned long debounceDelay ; // initialised from EEPROM in setup. See option B.

// Define the maximum permitted debounce timer value. Current value is stored in EEPROM, so
// must be <=255 ms. This value is used when setting the timer via the B CLI option.
// The minDebounceTimer will always be 1ms, so no definition for it.
const unsigned long maxDebounceDelay = 20; //ms

// Define the default debounce timer value in case the EEPROM is not yet set sensibly.
const unsigned long defaultDebounceDelay = 2; //ms

// -------------------------------------------------------------------------------------------
//                                   Sensor Guard Time
// -------------------------------------------------------------------------------------------

// Set up an array to hold the actual time that the guard timer will expire for each channel,
// one per channel. The data type is unsigned long, because these values will be the sum of
// guardDelay (x 10) and the output of millis().
unsigned long channelGuardEndTime[maxNumChannels]; // initialise these in setup()

// Set up a variable to hold the debounce delay time, in milliseconds.
unsigned long guardDelay ; // initialised from EEPROM in setup. See option G.

// Define the maximum permitted guard timer value. Current value is stored in EEPROM, so
// must be <=255 cs. This value is used when setting the timer via the G CLI option.
// The minDebounceTimer will always be 1cs, so no definition for it.
const unsigned long maxGuardDelay = 50; //cs

// Define the default guard timer value in case the EEPROM is not yet set sensibly.
const unsigned long defaultGuardDelay = 10; //cs

// -------------------------------------------------------------------------------------------
//                                    State Machines
// -------------------------------------------------------------------------------------------

// Set up an array of integers to indicate whether each per-channel state machine is in the
// program state "WAIT_FOR_INPUT" (0), "WAIT_FOR_DEBOUNCE" (1), "WAIT_FOR_GUARD"(2), 
// "TEST_MODE" (3) or "SENSOR_DISABLED" (4)
int channelMachineState[maxNumChannels]; // initialise these in setup()

// Define the possible machine states as text to make the code more readable.
#define WAIT_FOR_INPUT 0
#define WAIT_FOR_DEBOUNCE 1
#define WAIT_FOR_GUARD 2
#define TEST_MODE 3
#define SENSOR_DISABLED 4

// -------------------------------------------------------------------------------------------
//                                      Debugging
// -------------------------------------------------------------------------------------------
// Set up a Debug Mode flag - this always cleared on reset, so not held in EEPROM.
boolean debugMode = false;

// Define the debug flags as text to make the code more readable
#define DEBUG_PULSE_TIMER 1		//bit 0
#define DEBUG_SHOW_MISFIRES 2	//bit 1
#define DEBUG_SHOW_DEBOUNCE 4	//bit 2
#define DEBUG_SHOW_LED 8		//bit 3
#define DEBUG_SWING_TIMER 16	//bit 4
// Remaining bits not used.
// (See also the function printDebugFlagName(), seeing as F() is not available globally)

// Set up a Debug Mask variable as a 16 bit unsigned word. The bits (0-16)
// correspond to debugging being enabled on that channel (1=true=on, 0=false=off). This
// allows per-channel debugging to be configured. LSB = Channel 0 (which is sensor 1).
word debugChannelMask; // initialised from EEPROM in setup. See option M.
// See also defaulMask above.

// Set up a Debug Flags Mask variable. The first 5 bits (0-4) correspond to debugging
// facilities in the code (1=on, 0=off). This allows each facility to turned on and off
// independently. These can be changed on the fly with the D option.
word debugFlagsMask = DEBUG_PULSE_TIMER; //debug starts with DEBUG_PULSE_TIMER flag set by default

// Define the maximum configured number of flags. This value will depend on the flags
// actually implemented in the code. Currently there are 5 flags, as #defined below.
// If you add a debug level, increment this constant so that the CLI works.
const int maxDebugFlags = 5;


// -------------------------------------------------------------------------------------------
//                                   Debug Pulse Timing
// -------------------------------------------------------------------------------------------

// These variables are used for debug mode pulse length timing and counting only. There is
// no need to do pulse timing during normal operation.

// Set up two arrays to hold the actual start and end times of the sensor pulse, one per bell.
// The data type is unsigned long, because these values will be the output of micros(). These
// are used in debug mode only to calculate the length of the (first) sensor pulse.
unsigned long pulseStartTime[maxNumChannels]; // initialise these in setup()
unsigned long pulseEndTime[maxNumChannels];   // initialise these in setup()

// Set up an array to store the last observed sensor status, so we can detect transitions.
// This array is used for debug mode pulse timing only, because updating the real
// channelLastValue[] would lose the debounce effect for real simulator operation (which
// has to carry on regardless).
int pulseTimeLastValue[maxNumChannels]; //initialise these in setup, and set them LOW (see below)

// Set up an array of integers to count the pulses (to show how noisy a sensor is). We do this
// by counting spurious pulse *ends* (LOW to HIGH) when we aren't expecting them, and to make
// sure we timed only the first pulse.
int pulseTimeCount[maxNumChannels]; //initialise in setup, and set to zero


// -------------------------------------------------------------------------------------------
//                                   Debug Swing Timing
// -------------------------------------------------------------------------------------------

// These variables are used for debug mode swing timing only. There is no need to do swing
// timing during normal operation.

// Set up an array to hold the saved start time of the /previous/ pulse, so that the swing
// time (i.e. the delta between one pulse and the next) can be calculated, one per bell.
// The previous value of pulseStartTime[n] is pushed into this array before pulseStartTime[n]
// is updated in WAIT_FOR_INPUT.
unsigned long lastPulseStartTime[maxNumChannels]; // microseconds, initialise these in setup()

// Set up an array to hold the duration of the previous swing, so that the delta between this
// swing time and the previous one can be calculated (i.e. the difference between handstroke
// and backstroke). The swingTime value is pushed into lastSwingTime[n] for re-use after the
// current swing time has been calculated and displayed in WAIT_FOR_DEBOUNCE. Data type is long,
// because this will be difference between two long values, and may be negative.
long lastSwingTime[maxNumChannels]; // milliseconds, initialise these in setup()


// -------------------------------------------------------------------------------------------
//                                     CLI Terminal
// -------------------------------------------------------------------------------------------

// Define the terminal display configurations as text to make the code more readable.
// See termSetFor().
#define TERM_DEFAULT 0
#define TERM_HEADING 1
#define TERM_CLI 2
#define TERM_INPUT 3
#define TERM_CONFIRM 4
#define TERM_DEBUG 5

// -------------------------------------------------------------------------------------------
//                                     Test Mode
// -------------------------------------------------------------------------------------------

// These variables are used for test mode. None of these things are configurable on the fly.

// Define the delay before test mode kicks in after selecting the "T" CLI option
const int testStartDelay = 20; //seconds

// The number of bells used in test mode (and elsewhere) is calculated as numChannels from the
// highest channel enabled in the enabledChannelMask, so by setting this (option E in the CLI),
// the number of test mode bells can be set.

// Define the inter-bell interval used in test mode. 200ms equates to a 12-bell peal speed
// of 3h30m with an open handstroke lead.
const int testInterval = 200; //msec

// Define the test mode:
// Mode 0 = Bail out of the T menu item in the CLI
// Mode 1 = Rounds on testBells @testInterval ms
// Mode 2 = "Firing" at approx the same speed, all characters sent as quickly as possible
int testMode = 0;

// -------------------------------------------------------------------------------------------
//                                     LEDs & Timing
// -------------------------------------------------------------------------------------------

// Miscellaneous LEDs and optionally a CRO timing pin

#ifdef ARDUINO_AVR_SIMULATOR_TYPE2
// Only 1 signal LED, the right hand yellow LED in the power/data RJ45 connector
const int LED = 4;
#endif

// If USE_CRO was defined above, define a pin for CRO timing measurements.
#if defined USE_CRO && defined ARDUINO_AVR_SIMULATOR_TYPE2
const int CROpin = 5; 
#endif

// Overall this leaves as spares the following pins:
// - Type 2: 5

// ------------------------------------------------------------------------------------------
//                                   Target Remapping
// ------------------------------------------------------------------------------------------

// The remapper allows the character sent by each sensor/channel to be mapped to a different bell.
// The array bellStrikeChar[] above is no longer a const, but can be amended on the fly.
// See also defaultBellStrikeChar[] above, in case the EEPROM is not set sensibly.

// Note that many simulator interfaces do not currently send A-D, or W-Z (switch codes), but
// the remapper supports these (and thus allows a bell to be configured as a switch!).

// Define an array of valid mapped characters for user input checking.
// ABCD is standard nomenclature for bells 13-16, WXYZ for Abel-style switches.
const char validMappedChars[] = "1234567890ETABCDWXYZ"; 


/*
*********************************************************************************************
*                                   Function setup()                                        *
*********************************************************************************************
*/

// the setup routine runs once on powerup or reset:
void setup() {  

	// initialize the LED pins as outputs.
	pinMode( LED, OUTPUT );

	// set LED pins appropriately (turn the LEDs off).
	digitalWrite( LED, LOW );

	// initialize the timing pin, if used.
#ifdef USE_CRO
	pinMode( CROpin, OUTPUT );
	digitalWrite( CROpin, LOW );
#endif

	// Load default values from EEPROM
	loadFromEEPROM();
	
	// Calculate the highest enabled channel number and set numChannels
	numChannels = getNumChannels( enabledChannelMask );

	// generic setup loop counter
	int i;

	// setup data structures for as many entries supported by the hardware, maxNumChannels,
	// even though we are scanning up to numChannels in operation.
	for ( i = 0; i < maxNumChannels; i++ ) {

		// initialize the channel digital input pins with the pullup resistors active.
		// The photoheads pull the pin LOW when there is a signal.
		pinMode( channelSensorPin[i], INPUT_PULLUP );

		// initialize the array holding the last sensor status for each channel. Set these initially
		// to be LOW, so that the very first pass of loop() doesn't find a spurious transition
		// if the bells are down (and thus signalling continuously) and result in a fire-up
		// on startup.
		channelLastValue[i] = LOW;
		
		// initialize the guard timer expiry time values. These values should never get used, because
		// they will be overwritten when the actual time is calculated as the state machine enters
		// "WAIT_FOR_GUARD", but there is no harm in initialising it here just in case.
		channelGuardEndTime[i] = millis();

		// initialize the debounce timer end time values. These values should never get used,
		// because they will be overwritten when the actual time is calculated as the state
		// machine leaves "WAIT_FOR_INPUT", but there is no harm in initialising it here just in case.
		channelDebounceEndTime[i] = micros();

		// initialise the variables used for debug mode pulse length and swing time measurements    
		pulseStartTime[i] = 0;
		pulseEndTime[i] = 0;
		pulseTimeLastValue[i] = HIGH;
		pulseTimeCount[i] = 0;
		lastPulseStartTime[i] = 0; //swing timing
		lastSwingTime[i] = 0; //swing timing
		
	}
	
	// Set the enable state for ALL channels. This initializes the state machine flag for each
	// channel. All enabled channels start by waiting for input. Use maxNumChannels here, as
	// numChannels is now derived from the enabledChannelMask.
	enableChannels( enabledChannelMask, maxNumChannels );
	
	// Set up the serial port. The MBI interface spec uses 2400 8N1, another speed may have
	// been set for debugging purposes.
	Serial.begin( serialSpeeds[serialSpeed], SERIAL_8N1 );
	termInit();

	// Announce the software version on startup or reset
	blinkLED( LED, majorVersion, minorVersion ); 

	// If the serial port speed is not explicitly set to 2400 in the EEPROM, turn the yellow
	// LED on and leave it on as a warning (until the next command changes the LED state)
	if ( serialSpeed != 0 ) {
		delay(100);
		digitalWrite( LED, HIGH );
	}

}

/*
*********************************************************************************************
*                                     Function loop()                                       *
*********************************************************************************************
*/

// once setup has completed, the loop routine runs over and over again forever:
void loop() {

	// main loop start, waggle the CRO timing pin up if used.
#ifdef USE_CRO
	digitalWrite( CROpin, HIGH );
#endif

	// main loop counter
	int i;
	// test mode counters
	int j, k;

	// loop around all the active channels and look at the state of the state machine for each
	for ( i = 0; i < numChannels; i++ ) {

		// Variable to store the value read from the pin
		int channelSensorValue;
		
		switch ( channelMachineState[i] ) {

			// ---------------------------------------------------------------------------------------
			// -                                 WAIT_FOR_INPUT                                      -
			// ---------------------------------------------------------------------------------------
		case WAIT_FOR_INPUT:

			// This channel is in "WAIT_FOR_INPUT" state
			// Read the value from the sensor pin
			channelSensorValue = digitalRead( channelSensorPin[i] );
			
			if ( channelSensorValue == LOW && channelLastValue[i] == HIGH ) {
				
				// This is a HIGH to LOW transition since we last looked, so we have the start of a
				// sensor pulse. Note the current time in a scratch variable (to avoid calling
				// time functions twice)
				unsigned long timeNowMicros;
				timeNowMicros = micros();
				unsigned long timeNowMillis;
				timeNowMillis = millis();
				
				// Set the time that the guard timer will expire to now + the guard timer,
				// remembering that the guard timer is in cs, not ms. (Do this calculation
				// here so we only have to do the comparison in following loops, not the addition)
				channelGuardEndTime[i] = timeNowMillis + ( guardDelay * 10 );

				// Set the time that the debounce timer will expire to to now + the debounce timer,
				// remembering that the stored value is in milliseconds but measured in microseconds.
				// (Do this calculation here so we only have to do the comparison in following loops,
				// not the addition)
				channelDebounceEndTime[i] = timeNowMicros + ( debounceDelay * 1000 );
				
				// Set the last observed value to the current state of the sensor, which is LOW.
				// For an explanation of why we do this, see below.
				channelLastValue[i] = channelSensorValue;

				// Use the LED to signal strikes on channel 1 (MBI always starts at channel 1, even for
				// <12 bells). Turn on when the pulse is received, off when guard timer expires. Array
				// is zero indexed. (The LED may be used in other debug options, so hand this off to
				// a function.)
				strikeLED( LED, i, true );
				
				// Move the machine state on to "WAIT_FOR_DEBOUNCE"
				channelMachineState[i] = WAIT_FOR_DEBOUNCE;
				
				// If we are in debug mode, grab the pulse start time (saving the previous value) and reset the counter.
				// Both the pulse timer and the misfire detection require pulse timing to be done.
				// Swing timing requires the start time of this and the previous pulse.
				if ( debugThisChannel( i ) && isDebugFlagSet( DEBUG_PULSE_TIMER | DEBUG_SHOW_MISFIRES | DEBUG_SWING_TIMER ) ) {
					
					lastPulseStartTime[i] = pulseStartTime[i]; //save the previous value for swing timer debugging
					pulseStartTime[i] = timeNowMicros;
										
					// Avoid overflows giving a silly number if we get to a debug print before the end
					// of the first pulse! A pulse length of 0 means the pulse has not ended yet.
					pulseEndTime[i] = pulseStartTime[i];
					pulseTimeLastValue[i] = channelSensorValue;
					pulseTimeCount[i] = 0; //incremented at the END of the pulse
					
					// We only run this code at the start of a valid pulse. The counter is used
					// to ensure that we only measure the duration of the first pulse in a potential
					// string of noisy input pulses (which are ignored by the main interface code).
					
				} //debugThisBell
				
			}
			else // this is not the start of a pulse
			{

				// Set the last observed value to the current state of the sensor, which may
				// (still) be LOW or (by now) HIGH.
				channelLastValue[i] = channelSensorValue;
				
				// We stay in the "WAIT_FOR_INPUT" state. We may have come back here and found the
				// sensor still LOW after sending the character to the simulator if the delay is
				// very short and the pulse very long, hence the transition detection.
				
			} //channelSensorValue
			
			break; //channelMachineState == WAIT_FOR_INPUT

			// ---------------------------------------------------------------------------------------
			// -                                WAIT_FOR_DEBOUNCE                                    -
			// ---------------------------------------------------------------------------------------      
		case WAIT_FOR_DEBOUNCE:
			
			// This channel is in "WAIT_FOR_DEBOUNCE" state.
			// Check whether the signal has ended too soon (and so is considered noise).
			
			// Note the current time in a scratch variable (to avoid calling time functions twice)
			unsigned long timeNowMicros;
			timeNowMicros = micros();
						
			// Read the value from the sensor pin
			channelSensorValue = digitalRead( channelSensorPin[i] );

				
			if ( channelSensorValue == HIGH ) {
				
				// Input has gone high again, pulse was too short, so this is a misfire.
				
				// Turn the LED off again if it was on
				strikeLED( LED, i, false );
				
				// Set the machine state back to "WAIT_FOR_INPUT"
				channelMachineState[i] = WAIT_FOR_INPUT;
				
				// Bug fixed in 3.5: Set the last observed value to the current state of the
				// sensor, which is now HIGH again. If we don't do this, and the noise pulse
				// is so close to the stable pulse that the stable pulse begins on the very
				// next iteration of loop(), then the real pulse would be missed (because
				// WAIT_FOR_INPUT is looking for a high-to-low transition). This has only
				// been seen with a VERY noisy mechanical switch sensor. 
				channelLastValue[i] = channelSensorValue;
				
				// If we are in a debug mode, report the misfire and pulse data.
				if ( debugThisChannel( i ) && isDebugFlagSet( DEBUG_SHOW_MISFIRES ) ) {
					
					// Grab the time of the end of the pulse (approximately) 
					pulseEndTime[i] = timeNowMicros;
					
					Serial.print(F("M " ));
					Serial.print( i + 1 ); // The channel number, as 1-16
					Serial.print(F(" "));
					Serial.print( bellStrikeChar[i] );
					Serial.print(F(" "));
					Serial.print( channelDebounceEndTime[i] ); //microseconds
					Serial.print(F(" "));
					Serial.println( pulseEndTime[i] - pulseStartTime[i] ); //microseconds
					
					// There is no need to set the pulse counter here - misfires are by
					// definition pulse #1 of 1. There may however be noise pulses *after*
					// the first one which is longer than the debounce timer, so we will
					// update the pulse counter when in WAIT_FOR_GUARD
					
				} //debugThisBell
				
			}
			else if ( timeNowMicros >= channelDebounceEndTime[i] )
			{
				
				// Input is still low (otherwise we would have bailed out in the clause above)
				// and the debounce timer has now expired, so we have a good signal at least 
				// debounceDelay ms long. Send the appropriate character to the interface.
				// This uses Serial.print because the MBI protocol specifies ASCII characters
				// for bell strike signals. (The serial command code uses Serial.write because
				// it sends raw bytes.)
				
				// If any debug mode is set, suppress the normal output altogether. Debug code
				// supplies its own output as needed
				if ( ! debugMode ) {
					Serial.print( bellStrikeChar[i] );
				}				
				
				//Move the machine on to WAIT_FOR_GUARD state.
				channelMachineState[i] = WAIT_FOR_GUARD;
				
				if ( debugThisChannel( i ) ) {
					
					if ( isDebugFlagSet( DEBUG_SHOW_DEBOUNCE ) ) {
						
						Serial.print(F("D " ));
						Serial.print( i + 1 ); // The channel number, as 1-16
						Serial.print(F(" "));
						Serial.print( bellStrikeChar[i]);
						Serial.print(F(" "));
						Serial.println( channelDebounceEndTime[i] ); //microseconds
						
					} //debugFlagSet
				
					if ( isDebugFlagSet( DEBUG_SWING_TIMER ) ) {
						
						// Variables to hold the swing time and delta, in milliseconds
						long swingTime;
						long swingTimeDelta;
						
						// Calculate the length of this swing, converting to milliseconds
						swingTime = ( ( pulseStartTime[i] - lastPulseStartTime[i] ) / 1000 ) ;
						// Calculate the difference between this swing and the last one, in milliseconds
						swingTimeDelta = ( swingTime - lastSwingTime[i] ) ; // NB may be negative
						
						
						Serial.print(F("T " ));
						Serial.print( i + 1 ); // The channel number, as 1-16
						Serial.print(F(" "));
						Serial.print( bellStrikeChar[i]);
						Serial.print(F(" "));
						Serial.print( swingTime ); //milliseconds, always positive
						Serial.print(F(" "));
						
						// Try to align negatives and zeroes reasonably nicely
						if ( swingTimeDelta > 0 ) {
							Serial.print( "+" ); // Print will handle the negative signs
						}
						else if ( swingTimeDelta == 0 ) {
							Serial.print( " " ); // no sign for zero
						}
						
						Serial.println( swingTimeDelta ); //milliseconds, may be negative or zero
												
						// Save the swing time for this bell for re-use next time around.
						lastSwingTime[i] = swingTime;
					
					} //debugFlagSet
			
				} //debugThisChannel
				
			}
			else if ( ( channelDebounceEndTime[i] - timeNowMicros ) > ( maxDebounceDelay * 1000 ) )
			{
					
				// Input is still low (otherwise we would have bailed out above) but
				// channelDebounceEndTime[] seems a very long way into the future, so the
				// microsecond timer may have looped (2^32us is about 70 minutes). Under
				// normal conditions channelDebounceEndTime[] can never be more than
				// maxDebounceDelay into the future (because debounceDelay cannot be
				// >maxDebounceDelay. If it is, assume the timer has wrapped and reset
				// channelDebounceEndTime[] to timeNowMicros + the debounce delay. This
				// prevents the signal getting lost, but may extend the debounce time for
				// this signal (not more than doubling it), depending on exactly when the
				// wrap happened.
				// This will happen only if the microsecond timer wraps while a channel
				// is in WAIT_FOR_DEBOUNCE.
				
				channelDebounceEndTime[i] = timeNowMicros + ( debounceDelay * 1000 );
				
			}
			
			// There is no other else here - if the input  is still low and the timer
			// has not expired, we are still testing the signal quality so we just loop round.
			
			break; // channelMachineState == WAIT_FOR_DEBOUNCE

			// ---------------------------------------------------------------------------------------
			// -                                    WAIT_FOR_GUARD                                     -
			// ---------------------------------------------------------------------------------------      
		case WAIT_FOR_GUARD:
			
			// This channel is in "WAIT_FOR_GUARD" state.

			// We don't read the sensor pin at all when in this state. This debounces the input
			// and ignores spurious input. However there is a weakness here: If the delay is
			// insanely long then there is the possibility that signals may be lost. This has not
			// been addressed here, as it means having more than one signal outstanding for a
			// single channel, and a queue of timers, and this behaviour is not representative of
			// the behaviour of real bells...
			
			// Check the timer - has it expired?
			if ( millis() >= channelGuardEndTime[i] ) {
				
				// Turn the LED off again if appropriate
				strikeLED( LED, i, false );
				
				// Set the machine state back to "WAIT_FOR_INPUT"
				channelMachineState[i] = WAIT_FOR_INPUT;       
				
				// If we are in debug, send the timer values. This could be logged to a file for
				// later analysis.
				if ( debugThisChannel( i ) ) {
					
					if ( isDebugFlagSet( DEBUG_PULSE_TIMER ) ) {
						
						Serial.print(F("S "));
						Serial.print( i + 1 ); // The channel number, as 1-16
						Serial.print(F(" "));
						Serial.print( bellStrikeChar[i] );
						Serial.print(F(" "));
						Serial.print( channelGuardEndTime[i] ); //milliseconds
						Serial.print(F(" "));
						Serial.print( pulseEndTime[i] - pulseStartTime[i] ); //microseconds
						Serial.print(F(" "));
						Serial.print( pulseTimeCount[i] );
						Serial.print(F(" "));
						Serial.println( ( pulseStartTime[i] - lastPulseStartTime[i] ) / 1000 ); //milliseconds
						
						// Reset the pulse counter, as we are going back to WAIT_FOR_INPUT
						pulseTimeCount[i] = 0;
					}

				} //debugThisChannel
				
			} //timer expired

			// There is no ELSE here - there is nothing to do if we are waiting for the 
			// timer to expire, other than do the debugging below, we just loop and check
			// again on the next loop.
			
			// If we are in debug mode then we DO read the pin, but only to check for the end of
			// sensor pulses during the "wait for guard" period, and grab the time for debug output.
			
			if ( debugThisChannel( i ) && isDebugFlagSet( DEBUG_PULSE_TIMER ) ) {
				
				int channelSensorValue;
				channelSensorValue = digitalRead(channelSensorPin[i]);
				
				if ( channelSensorValue == HIGH && pulseTimeLastValue[i] == LOW ) {
					// LOW to HIGH transition, a pulse just ended
					
					if ( pulseTimeCount[i] == 0 ) {
						
						// This is the end of the first pulse (after a successful debounce), so note the
						// time.
						pulseEndTime[i] = micros();
						pulseTimeCount[i]++; // so we don't time any subsequent noisy pulses
						
					}
					else
					{
						
						// End of a spurious noise pulse after the first timed one, increment the
						// pulse counter.
						pulseTimeCount[i]++;
						
					}
					
				} //channelSensorValue == HIGH && pulseTimeLastValue[i] == LOW
				
				// Update the last seen sensor value.
				pulseTimeLastValue[i] = channelSensorValue;
				
			} //debugThisBell  
			
			break; //channelMachineState == WAIT_FOR_GUARD

			// ---------------------------------------------------------------------------------------
			// -                                      TEST_MODE                                      -
			// ---------------------------------------------------------------------------------------      
		case TEST_MODE:
			
			// This channel is in "TEST_MODE" state.
			
			// Test mode works like this. The "T" CLI command puts all the channels into TEST_MODE
			// and waits for testStartDelay seconds. For all channels other than 0, test mode does
			// nothing and the code skips past. For channel 0, the first time we fall into the
			// function below, the interface start to the test pattern at the defined pace.
			// The function never returns, and there is no escape other than an interface reset.
			
			if( i == 0 ) {
				runTestMode( testMode );
			} // no else, only do this for channel 0
			
			break; //channelMachineState == TEST_MODE

			// ---------------------------------------------------------------------------------------
			// -                                      SENSOR_DISABLED                                -
			// ---------------------------------------------------------------------------------------      
		case SENSOR_DISABLED:
			
			// This channel is in "SENSOR_DISABLED" state.
			
			// If the sensor is disabled, simply ignore it. Do nothing other than keep the
			// yellow LED on. SENSOR_DISABLED mode toggled with WAIT_FOR_INPUT in CLI
			// option E (for Enable).
			if ( debugThisChannel( i ) && !isDebugFlagSet( DEBUG_SHOW_LED ) ) {
				digitalWrite( LED, HIGH );
			}
			
			break; //channelMachineState == SENSOR_DISABLED

		} //case
		
	} //for numChannels

	// main loop complete, waggle the timing pin down.
#ifdef USE_CRO
	digitalWrite( CROpin, LOW );
#endif

} //loop()

/*
*********************************************************************************************
*                                  Function serialEvent()                                   *
*********************************************************************************************
*/

void serialEvent() {


/*  

We have to handle the CLI for the simulator interface.

This simulator interface does not process or store delay values sent by simulator software, 
delays are applied in software for maximum control over striking. However, we do have to
cater for simulator software sending unexpected delay data, in case that is misinterpreted 
as CLI commands and corrupts the settings, or sending MBI command input.

Therefore this code derived from the Type 1 simulator remains, to detect and discard MBI
commands or delay data.

The MBI protocol defines two distinct commands, plus the timers:
0xFD = Are you there?, correct response is 0xFD
0xFE = Send timer values, correct response is to send 12 bytes of timer values terminated 
		with 0xFF
0xNN = Anything else is the first of 12 bytes of hex timer values, terminated with 0xFF

Note that Abel 10.3.2 introduced support for the MBI protocol 0xFD command to detect a
simulator interface and do basic population of the External Bells dialogue box. The 0xFD
command is therefore supported.

The serialEvent() built-in function is not really event driven: It relies on loop(), so
does not get called when code is doing other things (like running the TEST_MODE loop).

*/

	// We got here because there is stuff in the serial buffer waiting for us.
	// We'll stay here until we have processed everything there is before we revert to loop().  
	while ( Serial.available() ) {

		// peek at the first byte in the buffer, without debuffering it, to see what's coming:
		byte peekChar = Serial.peek(); // what is waiting?
		byte inChar; // to hold the bytes when they are read().

		// What have we got?
		switch ( peekChar ) {

		case 0xFD:    // FD = Are You There command?
			inChar = (byte)Serial.read(); //debuffer the character
			Serial.write(0xFD);
			// Disable debugging
			debugMode = false;
			// blink the LED to indicate we got a valid command
			blinkLED( LED, 1, 1 );   
			break;
						
		case 0xFE:    // FE = Send Delay Values
			inChar = (byte)Serial.read(); //debuffer the character
			// No response is sent: we do not store delay values in the interface.
			// blink the LED to indicate we got a command
			blinkLED( LED, 1, 2 );   
			break;

		default:
	/*
	Even though we do not respond to MBI delay commands above, we may still be sent unsolicited
	delay data for 12 bells. Abel in particular will send delay data blindly on startup if
	interface-managed delay timers are configured. So, any non-command data must first be
	assumed to be the delay timers, because the protocol does not include any leading command
	character to announce that delay data is a-coming. So we set Serial.setTimeout to 1 sec
	and Serial.readBytesUntil to read 13 bytes, or timeout, and then look, check whether got
	exactly 13 bytes and that the last one is 0xFF. If it is, it's likely to be delay data
	and we throw it	away.
	*/

			// We are expecting to read 13 bytes in total (bells 1-12 & 0xFF).
			byte tempDelayBuffer[13]; // Bells 1-12 & 0xFF. Zero indexed.
			byte bytesRead; // how many bytes did we read?
			Serial.setTimeout(1000); // 1 sec
			
			// Serial.readBytesUntil will only write into a char[], so cast it to get it
			// to write into a byte[] array.
			bytesRead = Serial.readBytesUntil( 0xFF, (char *)tempDelayBuffer, 13 ); 

			// Did we get enough bytes, and is the 13th byte 0xFF?
			if (bytesRead == 13 && tempDelayBuffer[12] == 0xFF) {
				// This looks like delay timer data, ignore it.
				// blink the LED to indicate we got a valid command
				blinkLED( LED, 1, 3 );
				
			}
			else
			{
				/*
		Either we didn't read enough data for it to be delay values, or the last byte was
		not 0xFF. It might be a CLI command, but we have read the data now so we
		can't peek any more.

		If we got here then we know we read at least one byte (because there was a byte to
		peek at, and we called readBytesUntil(). We will look only at the first byte in the
		array, if someone managed to get two commands in before the 1000ms timeout, hard luck.
		Moral: Wait for the CLI to respond - there is no typeahead!
		*/

				// Hand the first byte over to handleCLI() to see if that can make sense of it...
				handleCLI( tempDelayBuffer[0] );
				
			} //else 

		} //switch peekchar
		
	}//serial available

}//serial event


