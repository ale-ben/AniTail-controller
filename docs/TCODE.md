# TCODE

## Introduction
TCODE (Tail Code) is a simple command protocol designed for AniTail to control the tail movements efficiently. It is inspired by GCODE used in CNC machines and 3D printers, but adapted to the specific needs of AniTail.

## Commands

### G0 A{angle}  B{angle} ...
This command moves the specified motors to the given angles at maximum speed. For example, "G0 A90 B45" would move the first motor to 90 degrees and the second motor to 45 degrees as fast as possible.

### G1 A{angle}  B{angle} ... F{speed in degrees per second}
This command moves the specified motors to the given angles at a specified speed. For example, "G1 A90 B45 F60" would move the first motor to 90 degrees and the second motor to 45 degrees at a speed of 60 degrees per second.

### G4 P{duration} (or S{duration})
This command causes a delay for the specified duration in milliseconds. For example, "G4 P500" would cause a delay of 500 milliseconds before executing the next command. The "S" prefix can also be used instead of "P" for the duration parameter in order to specify the duration in seconds.

### G28 (home all motors)
Moves all motors to their home position (hardcoded in cfg).

### M114 (get current angles)
Returns the current angles of all motors in the format "A{angle} B{angle} ...". For example, "A90 B45" would indicate that the first motor is at 90 degrees and the second motor is at 45 degrees.