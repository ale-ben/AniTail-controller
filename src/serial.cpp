#include "Arduino.h"

char* readSerialInput() {
	static int bufferIndex = 0;
	static int bufferSize = 0;
	static char* inputBuffer = nullptr;
	
	while (Serial.available() > 0) {
		char incomingByte = Serial.read();
		
		// Allocate or expand buffer as needed
		if (bufferIndex >= bufferSize - 2 && (incomingByte != '\n' && incomingByte != '\r')) {
			int newSize = (bufferSize == 0) ? 20 : bufferSize * 2;
			char* newBuffer = (char*)realloc(inputBuffer, newSize);
			if (newBuffer == nullptr) {
				Serial.println("Error: Memory allocation failed.");
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
			
			return result; // Caller must free()
		}
		
		// Add character to buffer
		inputBuffer[bufferIndex++] = incomingByte;
	}
	
	return nullptr; // No complete command yet
}