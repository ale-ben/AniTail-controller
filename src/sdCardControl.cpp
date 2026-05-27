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

static char* duplicateCommandBuffer(const char* source, size_t length) {
	char* result = static_cast<char*>(malloc(length + 1));
	if (result == nullptr) {
		return nullptr;
	}

	memcpy(result, source, length);
	result[length] = '\0';
	return result;
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
 * @brief If curr line pointer is null, read the first line of autostart.tcode, otherwise read the next line. Return null if no more lines to read.
 *
 * @return char* Pointer to the line read, or null if no more lines are available.
 */
static char* readSDCardInputInternal(bool canRewind) {
	if (!autostartFile.isOpen()) {
		if (!autostartFile.open(sd.vol(), "autostart.tcode", O_RDONLY)) {
			Log.errorln("Failed to open autostart.tcode");
			return nullptr;
		}
	}

	static int bufferSize = 0;
	static char* lineBuffer = nullptr;
	int bufferIndex = 0;

	while (true) {
		int nextByte = autostartFile.read();
		if (nextByte < 0) {
			if (bufferIndex > 0) {
				char* result = duplicateCommandBuffer(lineBuffer, bufferIndex);
				if (result == nullptr) {
					Log.errorln("Error: SD input result allocation failed.");
				}
				return result;
			}

			if (!canRewind) {
				return nullptr; // File is empty or unreadable.
			}

			autostartFile.close();
			if (!autostartFile.open(sd.vol(), "autostart.tcode", O_RDONLY)) {
				Log.errorln("Failed to reopen autostart.tcode");
				return nullptr;
			}
			return readSDCardInputInternal(false);
		}

		char incomingByte = static_cast<char>(nextByte);
		if (incomingByte == '\n' || incomingByte == '\r') {
			if (bufferIndex == 0) {
				continue;
			}
			char* result = duplicateCommandBuffer(lineBuffer, bufferIndex);
			if (result == nullptr) {
				Log.errorln("Error: SD input result allocation failed.");
				return nullptr;
			}
			return result;
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

char* readSDCardInput() {
	if (autostartIsPresent == 0) {
		return nullptr; // autostart.tcode is not present, so no input to read
	}
	return readSDCardInputInternal(true);
};
#endif // ENABLE_SDCARD_CONTROL