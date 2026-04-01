#include "Arduino.h"
#include "generalConfig.h"

char* readSerialInput() {
	static int bufferIndex = 0;
	static int bufferSize = 0;
	static char* inputBuffer = nullptr;
	
	while (Serial.available() > 0) {
		char incomingByte = Serial.read();
		
		// Allocate or expand buffer as needed
		if (bufferIndex >= bufferSize - 2 && (incomingByte != '\n' && incomingByte != '\r')) {
			Log.verboseln("Buffer full or near full (index: %d, size: %d), expanding buffer...", bufferIndex, bufferSize);
			int newSize = (bufferSize == 0) ? 20 : bufferSize * 2;
			char* newBuffer = (char*)realloc(inputBuffer, newSize);
			if (newBuffer == nullptr) {
				Log.errorln("Error: Memory allocation failed.");
				free(inputBuffer);
				inputBuffer = nullptr;
				bufferIndex = 0;
				bufferSize = 0;
				return nullptr;
			}
			inputBuffer = newBuffer;
			bufferSize = newSize;
		}
		
		if (incomingByte == '\n' || incomingByte == '\r') {
			// Complete command - null terminate and return
			inputBuffer[bufferIndex] = '\0';
			
			// Transfer ownership to caller
			char* result = inputBuffer;
			inputBuffer = nullptr;
			bufferIndex = 0;
			bufferSize = 0;

			Log.verboseln("Complete command received: '%s'", result);
			
			return result; // Caller must free()
		}
		
		// Add character to buffer
		inputBuffer[bufferIndex++] = incomingByte;
	}
	
	return nullptr; // No complete command yet
}