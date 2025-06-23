/*
Liverpool Ringing Simulator Project
Simulator Interface v3.8
Terminal Functions

Copyright 2014-2025 Andrew J Instone-Cowie.

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

Adapted from the BasicTerm library Copyright 2011 Trannie Carter <borys@nottwo.org>
Licensed for use under the terms of the GNU Lesser General Public License v3

termSetFor() is nor part of the original BasicTerm.
*/

void termInit( void ) {
	Serial.print(F("\x1b\x63")); 
}

void termClear( void ) {
	Serial.print(F("\x1b[2J")); // Clear the screen
	Serial.print(F("\x1b[H"));  // Move to Home position
}

void termPos( int row, int col) {
	Serial.print(F("\x1b["));
	Serial.print(row + 1);
	Serial.print(F(";"));
	Serial.print(col + 1);
	Serial.print(F("H"));
}

void termShowCursor( boolean show ) {
	if( show ) {
		Serial.print(F("\x1b[?25h"));
	} else {
		Serial.print(F("\x1b[?25l"));
	}
}

void termSetAttribute( int attr ) {
	if(attr & BT_REVERSE) {
		Serial.print(F("\x1b[7m"));
	}
	if(attr & BT_UNDERLINE) {
		Serial.print(F("\x1b[4m"));
	}
	if(attr & BT_BOLD) {
		Serial.print(F("\x1b[1m"));
	}
	if(attr & BT_BLINK) {
		Serial.print(F("\x1b[5m"));
	}
	if(attr == BT_NORMAL) {
		Serial.print(F("\x1b[0m"));
	}
}

void termSetColour( int fg, int bg ) {
	Serial.print(F("\x1b["));
	Serial.print(30 + fg);
	Serial.print(";");
	Serial.print(40 + bg);
	Serial.print(F("m"));
}

void termSetFor( int type ) {

	// Set up the CLI terminal for different classes of message

	switch ( type ) {
	case 0: // TERM_DEFAULT
		termSetAttribute( BT_NORMAL );
		break;
	case 1: // TERM_HEADING
		termSetAttribute( BT_BOLD );
		termSetColour( BT_MAGENTA, BT_BLACK );
		break;
	case 2: // TERM_CLI
		termSetAttribute( BT_BOLD );
		termSetColour( BT_CYAN, BT_BLACK );
		break;
	case 3: // TERM_INPUT
		termSetAttribute( BT_BOLD );
		termSetColour( BT_YELLOW, BT_BLACK );
		break;
	case 4: // TERM_CONFIRM
		termSetAttribute( BT_BOLD );
		termSetColour( BT_GREEN, BT_BLACK );
		break;
	case 5: // TERM_DEBUG
		termSetAttribute( BT_BOLD );
		termSetColour( BT_RED, BT_BLACK );
		break;
	default: // Some other EEPROM value
		termSetAttribute( BT_NORMAL );
		break;
	}

}



