#include "wifiControl.h"

#ifdef ENABLE_WIFI_CONTROL
#include <WiFi.h>
#ifdef ENABLE_WIFI_AP
	// Access Point credentials
	const char* ap_ssid = "AniTail-AP";
	const char* ap_password = "12345678";  // Minimum 8 characters for WPA2
#else
	// Station mode - use WiFiMulti for multiple network support
	#include <WiFiMulti.h>
	WiFiMulti wifiMulti;


	#include "wifiNetworks.h"
#endif

WiFiServer server(80);

void setupWiFiControl() {
#ifdef ENABLE_WIFI_AP
	Log.traceln("Setting up WiFi Access Point...");
	
	// Disable WiFi persistence to avoid flash wear and potential issues
	WiFi.persistent(false);
	
	// Configure Access Point
	WiFi.mode(WIFI_AP);
	delay(100); // Brief delay after mode change
	
	// Disable power saving for more stable AP
	WiFi.setSleep(false);
	
	// softAP(ssid, password, channel, hidden, max_connections)
	// channel 1-13, hidden 0=broadcast, max_connections 1-4 (default 4)
	bool success = WiFi.softAP(ap_ssid, ap_password, 6, 0, 4);
	
	if (!success) {
		Log.errorln("Failed to start Access Point!");
		return;
	}
	
	IPAddress IP = WiFi.softAPIP();
	Log.traceln("AP IP address: %p", IP);
	Log.traceln("AP SSID: %s", ap_ssid);
	Log.traceln("AP Password: %s", ap_password);
	Log.traceln("Number of stations: %d", WiFi.softAPgetStationNum());
#else
	Log.traceln("Connecting to WiFi networks...");
	
	// Configure Station mode
	WiFi.mode(WIFI_STA);
	
	// Configure client networks
	configureClientNetworks(wifiMulti);
	
	// Try to connect (with timeout)
	unsigned long startAttempt = millis();
	while (wifiMulti.run() != WL_CONNECTED && millis() - startAttempt < 10000) {
		delay(100);
	}
	
	if (WiFi.status() == WL_CONNECTED) {
		IPAddress IP = WiFi.localIP();
		Log.traceln("Connected! IP address: %p", IP);
		Log.traceln("Connected to: %s", WiFi.SSID().c_str());
	} else {
		Log.errorln("WiFi connection failed!");
		return;
	}
#endif
	
	// Start server (same for both modes)
	server.begin();
	Log.traceln("HTTP server started on port 80");
}

void checkWiFiStatus() {
#ifdef ENABLE_WIFI_AP
	static unsigned long lastCheck = 0;
	if (millis() - lastCheck > 5000) { // Check every 5 seconds
		lastCheck = millis();
		int numStations = WiFi.softAPgetStationNum();
		Log.verboseln("AP Status - Stations: %d, Mode: %d", numStations, WiFi.getMode());
		
		// If WiFi mode changed unexpectedly, log it
		if (WiFi.getMode() != WIFI_AP) {
			Log.errorln("WiFi mode changed! Current mode: %d (should be %d)", WiFi.getMode(), WIFI_AP);
		}
	}
#endif
}

char* readWiFiInput() {
	static WiFiClient activeClient;
	static int bufferIndex = 0;
	static int bufferSize = 0;
	static char* inputBuffer = nullptr;
	static int currentLineLength = 0;
	static unsigned long clientTimeout = 0;
	static int contentLength = 0;
	static bool headersComplete = false;
	static int bodyBytesRead = 0;
	
	// Check for new client if we don't have an active one
	if (!activeClient || !activeClient.connected()) {
		activeClient = server.available();
		if (activeClient) {
			Log.traceln("New client connected");
			// Reset buffer for new connection
			if (inputBuffer) {
				free(inputBuffer);
				inputBuffer = nullptr;
			}
			bufferIndex = 0;
			bufferSize = 0;
			currentLineLength = 0;
			contentLength = 0;
			headersComplete = false;
			bodyBytesRead = 0;
			clientTimeout = millis() + 3000; // 3 second timeout
		}
	}
	
	// Process active client if available
	if (activeClient && activeClient.connected()) {
		// Check for timeout
		if (millis() > clientTimeout) {
			Log.traceln("Client timeout");
			activeClient.stop();
			if (inputBuffer) {
				free(inputBuffer);
				inputBuffer = nullptr;
			}
			bufferIndex = 0;
			bufferSize = 0;
			currentLineLength = 0;
			contentLength = 0;
			headersComplete = false;
			bodyBytesRead = 0;
			return nullptr;
		}
		
		// Read all available data (similar to serial pattern)
		while (activeClient.available()) {
			char incomingByte = activeClient.read();
			
			// Allocate or expand buffer as needed
			if (bufferIndex >= bufferSize - 1) {
				Log.verboseln("Buffer full or near full (index: %d, size: %d), expanding buffer...", bufferIndex, bufferSize);
				int newSize = (bufferSize == 0) ? 128 : bufferSize * 2;
				char* newBuffer = (char*)realloc(inputBuffer, newSize);
				if (newBuffer == nullptr) {
					Log.errorln("Error: Memory allocation failed.");
					free(inputBuffer);
					inputBuffer = nullptr;
					bufferIndex = 0;
					bufferSize = 0;
					currentLineLength = 0;
					contentLength = 0;
					headersComplete = false;
					bodyBytesRead = 0;
					activeClient.stop();
					return nullptr;
				}
				inputBuffer = newBuffer;
				bufferSize = newSize;
			}
			
			// Add character to buffer
			inputBuffer[bufferIndex++] = incomingByte;
			
			if (!headersComplete) {
				// Still reading headers
				if (incomingByte == '\n') {
					if (currentLineLength == 0) {
						// Blank line - headers complete
						headersComplete = true;
						bodyBytesRead = 0;
						
						// Parse Content-Length from headers if present
						inputBuffer[bufferIndex] = '\0';
						char* clHeader = strstr(inputBuffer, "Content-Length: ");
						if (!clHeader) {
							clHeader = strstr(inputBuffer, "content-length: ");
						}
						if (clHeader) {
							contentLength = atoi(clHeader + 16);
							Log.verboseln("Content-Length: %d", contentLength);
						}
					} else {
						currentLineLength = 0;
					}
				} else if (incomingByte != '\r') {
					currentLineLength++;
				}
			} else {
				// Reading body
				bodyBytesRead++;
			}
		}
		
		// Check if we have a complete request
		// For GET requests: headers complete and no content
		// For POST requests: headers complete and all body bytes received
		if (headersComplete && (contentLength == 0 || bodyBytesRead >= contentLength)) {
			// Null terminate the buffer
			inputBuffer[bufferIndex] = '\0';
			
			Log.verboseln("Complete request received. Buffer length: %d, Content-Length: %d, Body bytes: %d", 
			              bufferIndex, contentLength, bodyBytesRead);
			
			// Find body (after \r\n\r\n) BEFORE modifying buffer
			char* bodyStart = strstr(inputBuffer, "\r\n\r\n");
			if (bodyStart) {
				bodyStart += 4; // Skip past \r\n\r\n
				Log.verboseln("Body found, content: '%s'", bodyStart);
			} else {
				Log.verboseln("Body delimiter \\r\\n\\r\\n not found!");
			}
			
			// Parse the HTTP request
			char* method = nullptr;
			char* endpoint = nullptr;
			
			// Find first space (end of method)
			char* firstSpace = strchr(inputBuffer, ' ');
			if (firstSpace) {
				method = inputBuffer;
				*firstSpace = '\0';
				
				// Find second space (end of endpoint)
				char* secondSpace = strchr(firstSpace + 1, ' ');
				if (secondSpace) {
					endpoint = firstSpace + 1;
					*secondSpace = '\0';
				}
			}
			
			Log.traceln("Request: %s %s", method ? method : "?", endpoint ? endpoint : "?");
			
			// Send HTTP response
			activeClient.println("HTTP/1.1 200 OK");
			activeClient.println("Content-Type: application/json");
			activeClient.println("Access-Control-Allow-Origin: *");
			activeClient.println("Connection: close");
			activeClient.println();
			
			char* result = nullptr;
			
			// Handle REST endpoints
			if (method && endpoint && strcmp(method, "GET") == 0 && 
			    (strcmp(endpoint, "/") == 0 || strcmp(endpoint, "/status") == 0)) {
				activeClient.println("{\"status\":\"ok\",\"device\":\"AniTail\"}");
			}
			else if (method && endpoint && strcmp(method, "POST") == 0 && strcmp(endpoint, "/tcode") == 0) {
				if (bodyStart && *bodyStart != '\0') {
					Log.traceln("TCODE command: %s", bodyStart);
					
					// Allocate new buffer for result and copy body
					size_t bodyLen = strlen(bodyStart);
					result = (char*)malloc(bodyLen + 1);
					if (result) {
						strcpy(result, bodyStart);
						// Trim trailing whitespace
						while (bodyLen > 0 && (result[bodyLen-1] == ' ' || result[bodyLen-1] == '\r' || result[bodyLen-1] == '\n')) {
							result[--bodyLen] = '\0';
						}
						Log.verboseln("Complete command received: '%s'", result);
					}
					
					activeClient.println("{\"status\":\"ok\",\"command\":\"tcode\"}");
				} else {
					activeClient.println("{\"error\":\"Empty body\"}");
				}
			}
			else {
				activeClient.println("{\"error\":\"Unknown endpoint or method\"}");
			}
			
			// Close the connection
			activeClient.stop();
			Log.traceln("Client disconnected");
			
			// Clean up input buffer
			free(inputBuffer);
			inputBuffer = nullptr;
			bufferIndex = 0;
			bufferSize = 0;
			currentLineLength = 0;
			contentLength = 0;
			headersComplete = false;
			bodyBytesRead = 0;
			
			return result; // Caller must free()
		}
	}
	
	return nullptr; // No complete command yet
}

#endif // ENABLE_WIFI_CONTROL