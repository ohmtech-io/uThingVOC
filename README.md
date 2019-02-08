# uThing::VOC

### Air Quality USB dongle with configurable output format 

## Toolchain

1. Get the ARM-GCC compiler for your platform.

    Altought different versions of the ARM-GCC compiler may work, the prototype was developed and tested with the *gcc-arm-none-eabi-8-2018-q4-mayor* version, available in the ARM website: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads.

2. Change the GCC_PATH path on the Makefile with your toolchain's location.
    Alternatively, compile adding the **GCC_PATH** to the *make* command (*make GCC_PATH="/gcc-arm-path"*) or add it as environment variable.

## Building

```
make
```

There is a macro which enable/disable printing some debug information (using it as UartLog()) often useful for development while monitoring the USART1 TX pad on the board bottom. To enable it, compile with:

```
make DEBUG=1
```

## Flashing

### Using JLink connected to the SWD port

In this case, it's necesary to solder the correspondent signals to the SWD port on the board bottom (do not forget to connect the 3V3 power signal or JLink won't detect the target).

```
make flash
```
### Using USB-DFU

If the intention is to simply update the firmware, or just program a slightly modified version without the need for extensive debugging / testing, the MCU can be re-flashed via the USB port by using the on-board USB-DFU capability.

The programming via USB requires two steps:

1. **Booting the dongle into USB-DFU mode**:

    The MCU has a special Bootloader in ROM memory, which provides the functionality of DFU. In order to start this process, the MCU has to be reset into this Bootloader mode. This is accomplished by powering-up the device while holding the BOOT0 line (pin #44) into logic-high level (3.3V / VCC).

    To do this, unplug the dongle, hold a jumper (conductive material, like a piece of wire, screwdriver, paper-clip, etc.) between the VCC and BOOT0 pins with the precaution of not short-circuit any other pin, and plug the device into the USB while shorting these 2 pins. In this case, both status LEDs should stay OFF. The jumper can be then released.

    To verify if the Bootloader enumerated the MCU this time as a USB-DFU capable device, use the command lsusb This is the displayed information in MacOS:

    ```
    Bus 020 Device 008: ID 0483:df11 STMicroelectronics STM32  BOOTLOADER  Serial: FFFFFFFEFFFF
    ```

    In a Debian distribution:

    ```
    Bus 001 Device 004: ID 0483:df11 STMicroelectronics STM Device in DFU Mode
    ```

    In Windows, the Device Manager show a “STM Device in DFU Mode” under USB devices.

2. **The DFU-UTIL command**:

    DFU-UTIL is a CLI application available for MacOS, Linux and Windows. In the Unix-based systems it’s available in the usual packet managers (Brew, apt-get, etc.). The version v0.9 is the latest one at the moment of writing and it’s been tested successfully on Debian16 and MacOS.

    To perform the firmware programming, issue the following command, where “USBthingVOC.bin” is the application to flash in .bin format (no HEX or ELF supported):

    ```    
    dfu-util -a 0 -D USBthingVOC.bin --dfuse-address 0x0800C000 -d 0483:df11
    ```
     *Note:* In Linux, the dfu-util needs to be run as root, or a udev rule and permissions should be added.
