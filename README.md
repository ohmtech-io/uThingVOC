# uThing::VOC

![uThingVOC](/img/uThingVOC-block-diagram.png)

### Air Quality USB dongle with configurable output format 

## Toolchain

1. Get the ARM-GCC compiler for your platform.

    Although different versions of the ARM-GCC compiler may work correctly, the prototype was developed and tested using the *gcc-arm-none-eabi-8-2018-q4-mayor* version, available in the ARM website: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads.

2. Change the GCC_PATH path on the Makefile with your toolchain's location.
    Alternatively, compile adding the **GCC_PATH** to the *make* command (`make GCC_PATH="/gcc-arm-path"`) or add it as environment variable.

## Bosch BSEC library

The Bosch BSEC library is provided as a closed source binary file upon acceptance of a license agreement.

Therefore the user needs to read the license and download the library here:
https://www.bosch-sensortec.com/bst/products/all_products/bsec

The initial revision of uThing::VOC uses the BSEC version v1.4.7.1 | Sept. 18th, 2018.

After download, locate the appropriate version of the binary file for the **Lite** version.
The file can be found under the following path:
`DownloadedPackage/algo/bin/Lite_version/Cortex_M0/libalgobsec.a`

Then copy the library into the project source as:
`uThingVOC/Middlewares/Bosch/libalgobsec.a`

## Building

```
make
```

*Note:* There is a macro which enable/disable printing some debug information (using it as UartLog()) often useful for development while monitoring the USART1 TX pad on the board bottom. To enable it, compile with:

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

    The MCU has a special Bootloader in ROM memory, which provides the functionality of DFU. In order to start this process, the MCU has to be reset into this **DFU-Bootloader mode**. This is accomplished by powering-up the device while holding the **BOOT0** line (pin #44) into logic-high level (3.3V / **VCC**).

    ![uThingVOC](/img/Boot0-location.jpg)

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
