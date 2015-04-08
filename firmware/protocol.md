# Command protocol

## Command format

Each command is composed of a single character command code, with 1 or more characters of command data.

[1 byte: command code][n bytes: command data]

## Response format

The success response indicates that a command was successful, and returns 1 or more bytes of response data. The minimum response length is 3 bytes, and the maximum is 258 bytes.

[1 byte: command status ('P' or 'F')][1 byte: response length - 1][response length bytes: return data]

The command status is 'P' for pass, or 'F' for failure.

The return data can be from 1 to 256 bytes in length, and is command-specific

## LED Controller commands

### 0x01: Program address

For WS2822s pixels: Program the connected pixel with the specificed address.

[0x01][Address high byte][Address low byte]

### 0x02: Reload animations

Re-start the animation playback state machine.

## Filesystem Commands

### 0x10: Get free space

Get the total amount of free space on the flash, in bytes. Note that the largest available file may be smaller than this.

Return data:

[4 bytes: Free space in the flash]

### 0x11 Get largest file

Gets the size of the largest available file, in bytes.

[4 bytes: Largest available file]

### 0x12 File count

Get the total number of files stored in the flash

### 0x13 First free sector

Find the first free sector that a file can be written to

[

