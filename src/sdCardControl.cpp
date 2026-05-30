#include "sdCardControl.h"

#ifdef ENABLE_SDCARD_CONTROL
#ifndef DISABLE_FS_H_WARNING
#define DISABLE_FS_H_WARNING  // Disable warning for type File not defined.
#endif  // DISABLE_FS_H_WARNING
#include "SdFat.h"
#include "sdios.h"



#define SPI_SPEED SD_SCK_MHZ(4)
//------------------------------------------------------------------------------
SdExFat sd;
ExFile autostartFile;
ExFile sdCommandFile;
bool sdCommandFileActive = false;
bool sdPlaybackPaused = false;

static char* duplicateCommandBuffer(const char* source, size_t length) {
	char* result = static_cast<char*>(malloc(length + 1));
	if (result == nullptr) {
		return nullptr;
	}

	memcpy(result, source, length);
	result[length] = '\0';
	return result;
}

static bool hasTCodeExtension(const char* path) {
	const char* lastDot = strrchr(path, '.');
	if (lastDot == nullptr) {
		return false;
	}

	return strcmp(lastDot, ".tcode") == 0;
}

static bool commandTailIsOnlyWhitespace(const char* command, size_t length, size_t startIndex) {
	for (size_t i = startIndex; i < length; i++) {
		if (command[i] != ' ') {
			return false;
		}
	}
	return true;
}

static void restartActiveSDInputFile() {
	ExFile* activeInputFile = sdCommandFileActive ? &sdCommandFile : &autostartFile;

	if (!activeInputFile->isOpen()) {
		Log.errorln("S1 failed: no active SD input file is open.");
		return;
	}

	if (!activeInputFile->seekSet(0)) {
		Log.errorln("S1 failed: could not rewind active SD input file.");
		return;
	}

	if (sdCommandFileActive) {
		Log.infoln("S1: restarted current S0 file from beginning.");
	} else {
		Log.infoln("S1: restarted autostart.tcode from beginning.");
	}
}

bool setActiveSDCommandFile(const char* filename) {
	if (filename == nullptr || filename[0] == '\0') {
		Log.warningln("S0 command requires a filename.");
		return false;
	}

	size_t rawFilenameLength = strlen(filename);
	bool appendExtension = true;
	if (rawFilenameLength < 250) {
		char rawFilename[250];
		memcpy(rawFilename, filename, rawFilenameLength);
		rawFilename[rawFilenameLength] = '\0';
		appendExtension = !hasTCodeExtension(rawFilename);
	}

	size_t fullPathLength = rawFilenameLength + (appendExtension ? 6 : 0);
	char* filePath = static_cast<char*>(malloc(fullPathLength + 1));
	if (filePath == nullptr) {
		Log.errorln("Failed to allocate memory for S0 file path.");
		return false;
	}

	memcpy(filePath, filename, rawFilenameLength);
	if (appendExtension) {
		memcpy(filePath + rawFilenameLength, ".tcode", 6);
	} else {
		filePath[rawFilenameLength] = '\0';
	}

	if (sdCommandFile.isOpen()) {
		sdCommandFile.close();
	}
	sdCommandFileActive = false;

	if (!sdCommandFile.open(sd.vol(), filePath, O_RDONLY)) {
		Log.errorln("S0 file not found or failed to open: %s", filePath);
		free(filePath);
		return false;
	}

	if (sdCommandFile.isDirectory()) {
		Log.errorln("S0 path is a directory, expected file: %s", filePath);
		sdCommandFile.close();
		free(filePath);
		return false;
	}

	sdCommandFileActive = true;
	sdPlaybackPaused = false;
	Log.infoln("S0 command opened file: %s", filePath);
	free(filePath);
	return true;
}

void pauseSDCardPlayback() {
	if (!sdCommandFileActive && autostartIsPresent == 0) {
		Log.warningln("S2 failed: no SD input file is available.");
		return;
	}

	if (sdPlaybackPaused) {
		Log.infoln("S2: SD playback is already paused.");
		return;
	}

	sdPlaybackPaused = true;
	Log.infoln("S2: paused SD playback.");
}

void resumeSDCardPlayback() {
	if (!sdCommandFileActive && !autostartFile.isOpen() && autostartIsPresent == 0) {
		Log.warningln("S3 failed: no SD input file is available.");
		return;
	}

	if (!sdPlaybackPaused) {
		Log.infoln("S3: SD playback is already running.");
		return;
	}

	sdPlaybackPaused = false;
	Log.infoln("S3: resumed SD playback.");
}

static bool parseSDCardCommand(const char* command, size_t length) {
	if (length < 2) {
		Log.warningln("Invalid SD command: '%.*s'", static_cast<int>(length), command);
		return true;
	}

	if (command[1] == '1' && commandTailIsOnlyWhitespace(command, length, 2)) {
		restartActiveSDInputFile();
		return true;
	}

	return false;
}

int autostartIsPresent = 0; // Global variable to track if autostart.tcode is present on SD card

int setupSDCard() {
	Log.traceln("SD Card SPI pins: MISO=%d, MOSI=%d, SCK=%d, SS=%d", MISO, MOSI, SCK, SS);

	if (!sd.begin(CHIP_SELECT, SPI_SPEED)) {

		if (sd.card()->errorCode()) {
			Log.errorln("SD initialization failed. errorCode: 0x%X, errorData: 0x%X",                  sd.card()->errorCode(),                  sd.card()->errorData());
			return -1;
		}
		Log.infoln("Card successfully initialized.");

		if (sd.vol()->fatType() == 0) {
			Log.errorln("Can't find a valid exFAT partition.");
			return -1;
		}
		Log.errorln("Can't determine error type");
		return -1;
	}
	Log.infoln("Card successfully initialized.");


	uint32_t size = sd.card()->sectorCount();
	if (size == 0) {
		Log.errorln("Can't determine the card size.");
		return -1;
	}

	uint32_t sizeMB = 0.000512 * size + 0.5;
	Log.infoln("Card size: %lu MB (MB = 1,000,000 bytes)", static_cast<unsigned long>(sizeMB));
	if (sd.fatType() <= 32) {
		Log.infoln("Volume is FAT%d, Cluster size (bytes): %lu",
		           int(sd.fatType()),
		           static_cast<unsigned long>(sd.vol()->bytesPerCluster()));
	} else {
		Log.infoln("Volume is exFAT, Cluster size (bytes): %lu",
		           static_cast<unsigned long>(sd.vol()->bytesPerCluster()));
	}
	Log.infoln("Files found (date time size name):");
	sd.ls(LS_R | LS_DATE | LS_SIZE);

	if ((sizeMB > 1100 && sd.vol()->sectorsPerCluster() < 64) ||
	    (sizeMB < 2200 && sd.vol()->fatType() == 32)) {
		Log.errorln("This card should be reformatted for best performance. Use a cluster size of 32 KB for cards larger than 1 GB. Only cards larger than 2 GB should be formatted FAT32.");
		return -1;
	}

	// Check if autostart.tcode exists and is a file
	if (!sd.vol()->exists("autostart.tcode")) {
		Log.warningln("autostart.tcode not found on SD card.");
		autostartIsPresent = 0;
	} else {
		if (!autostartFile.open(sd.vol(), "autostart.tcode", O_RDONLY)) {
			Log.warningln("Failed to open autostart.tcode");
			return -1;
		} else if (autostartFile.isDirectory()) {
			Log.warningln("autostart.tcode is a directory, expected a file.");
			autostartFile.close();
			return -1;
		} else {
			autostartIsPresent = 1;
			Log.infoln("autostart.tcode found and opened successfully.");
		}
	}
	return 0;
}

/**
 * @brief Read the next command line from SD input sources.
 *
 * @return char* Pointer to the line read, or null if no more lines are available.
 */
char* readSDCardInput() {
	if (sdPlaybackPaused) {
		return nullptr;
	}

	if (autostartIsPresent == 0 && !sdCommandFileActive) {
		return nullptr; // autostart.tcode is not present, so no input to read
	}

	if (!sdCommandFileActive && !autostartFile.isOpen()) {
		if (!autostartFile.open(sd.vol(), "autostart.tcode", O_RDONLY)) {
			Log.errorln("Failed to open autostart.tcode");
			return nullptr;
		}
	}

	static int bufferSize = 0;
	static char* lineBuffer = nullptr;
	int bufferIndex = 0;
	bool skipCommentLine = false;

	while (true) {
		ExFile* activeInputFile = sdCommandFileActive ? &sdCommandFile : &autostartFile;
		int nextByte = activeInputFile->read();
		if (nextByte < 0) {
			if (skipCommentLine) {
				skipCommentLine = false;
				bufferIndex = 0;
			}
			if (bufferIndex > 0) {
				if (lineBuffer[0] == 'S') {
					if (parseSDCardCommand(lineBuffer, static_cast<size_t>(bufferIndex))) {
						bufferIndex = 0;
						continue;
					}
				}
				char* result = duplicateCommandBuffer(lineBuffer, bufferIndex);
				if (result == nullptr) {
					Log.errorln("Error: SD input result allocation failed.");
				}
				return result;
			}

			if (sdCommandFileActive) {
				sdCommandFile.close();
				sdCommandFileActive = false;

				if (autostartIsPresent == 0) {
					Log.infoln("S0 file complete. No autostart.tcode available.");
					return nullptr;
				}

				if (!autostartFile.isOpen()) {
					if (!autostartFile.open(sd.vol(), "autostart.tcode", O_RDONLY)) {
						Log.errorln("Failed to reopen autostart.tcode after S0 completion.");
						return nullptr;
					}
				}

				Log.infoln("S0 file complete. Resuming autostart.tcode.");
				continue;
			}
			return nullptr; // File complete or unreadable.
		}

		char incomingByte = static_cast<char>(nextByte);
		if (incomingByte == '\n' || incomingByte == '\r') {
			if (skipCommentLine) {
				skipCommentLine = false;
				bufferIndex = 0;
				continue;
			}
			if (bufferIndex == 0) {
				continue;
			}
			if (lineBuffer[0] == 'S') {
				if (parseSDCardCommand(lineBuffer, static_cast<size_t>(bufferIndex))) {
					bufferIndex = 0;
					continue;
				}
			}
			char* result = duplicateCommandBuffer(lineBuffer, bufferIndex);
			if (result == nullptr) {
				Log.errorln("Error: SD input result allocation failed.");
				return nullptr;
			}
			return result;
		}

		if (bufferIndex == 0 && incomingByte == '#') {
			skipCommentLine = true;
			continue;
		}

		if (skipCommentLine) {
			continue;
		}

		if (bufferIndex >= bufferSize - 1) {
			int newSize = (bufferSize == 0) ? 20 : bufferSize * 2;
			char* newBuffer = static_cast<char*>(realloc(lineBuffer, newSize));
			if (newBuffer == nullptr) {
				Log.errorln("Error: SD input buffer allocation failed.");
				free(lineBuffer);
				lineBuffer = nullptr;
				bufferSize = 0;
				return nullptr;
			}
			lineBuffer = newBuffer;
			bufferSize = newSize;
		}

		lineBuffer[bufferIndex++] = incomingByte;
	}
}

#endif // ENABLE_SDCARD_CONTROL