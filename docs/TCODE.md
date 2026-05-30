# TCODE

## Introduction
TCODE (Tail Code) is a simple command protocol designed for AniTail to control the tail movements efficiently. It is inspired by GCODE used in CNC machines and 3D printers, but adapted to the specific needs of AniTail.

## Commands

### G0 A{angle}  B{angle} ...
This command moves the specified motors to the given angles at maximum speed. For example, "G0 A90 B45" would move the first motor to 90 degrees and the second motor to 45 degrees as fast as possible.

### G1 A{angle}  B{angle} ... T{duration in milliseconds}
This command moves the specified motors to the given angles over a specified duration. For example, "G1 A90 B45 T1000" would move the first motor to 90 degrees and the second motor to 45 degrees over 1000 milliseconds (1 second).

### G4 P{duration} (or S{duration})
This command causes a delay for the specified duration in milliseconds. For example, "G4 P500" would cause a delay of 500 milliseconds before executing the next command. The "S" prefix can also be used instead of "P" for the duration parameter in order to specify the duration in seconds.

### G28 (home all motors)
Moves all motors to their home position (hardcoded in cfg).

### M114 (get current angles)
Returns the current angles of all motors in the format "A{angle} B{angle} ...". For example, "A90 B45" would indicate that the first motor is at 90 degrees and the second motor is at 45 degrees.



## File management commands

### S0 {filename}
This command executes a TCODE file stored on the SD card. For example, "S0 test/dance" would execute the commands in the "dance.tcode" file.
#### As of now, AniTail only support one open file (on top of autostart)

### S1
Restarts the currently running TCODE file from the beginning. This can be useful for looping animations or resetting the tail position.
#### NOTE: This command is only available in tcode files. Sending it from the serial console will have no effect.

### S2
Pauses the currently running TCODE file. The tail will hold its current position until the file is resumed or a new command is issued.

### S3
Resumes a paused TCODE file from where it was paused. This allows for seamless continuation of animations or movements after a pause.