# Blinky Pendant Firmware

## Development

The ARM toolchain 'arm-none-eabi-gcc' is used to compile this project. Specifically, the gcc-arm-none-eabi-5_4-2016q3 version. Get it here:

	https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update


You'll also need dfu-util. Homebrew is possibly the easiest way to get this.

You'll also need GNU Make, which probably requires Xcode on OS/X, or similar developer tools on Linux. The version is probably not as important.

Set the path to include the dev tools:

    export PATH=$PATH:~/gcc-arm-none-eabi-5_4-2016q3/bin

Once you have the toolchain installed, change to the firmware directory:

    cd firmware

and run make to compile:

    make

Use dfu-util (TODO: instructions for installing dfu-util!) to install the new firmware:

    make install

