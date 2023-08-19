# What is this?

This is a implimentation of a HID Device that can be flashed to a raspberry pi pico (although I think it should work on any RP2040 powered board) and it will attempt to work out the OS of the host it is connected to, much like the latest version of the HAK5 rubber ducky. (although this code only does detection)

# Hardware setup

This is intended to be attached to a UART to see the output of the device either via a Pico Probe https://github.com/raspberrypi/picoprobe or some other UART (like an FTDI) hooked up on Pin 1 for TX and Pin 2 for RX

# Compile the code

1. Install pico sdk https://github.com/pimoroni/pimoroni-pico/blob/main/setting-up-the-pico-sdk.md
1. Move the standard TinyUSB libary out of the way "mv ~/pico/pico-sdk/lib/tinyusb mv ~/pico/pico-sdk/lib/tinyusb.old" (update the path if you installed the SDK somewhere else)
1. Clone the modified version of TinyUSB into ~/pico/pico-sdk/lib/ with "cd ~/pico/pico-sdk/lib/" then "git clone https://github.com/ji-younghan/tinyusb"
1. Clone the pico-os-detect repository into the ~/pico/pico-examples/usb/device/ directory with "cd ~/pico/pico-examples/usb/device/" then "git clone https://github.com/ji-yonghan/pico-os-detect"
1. Create a build directory with "mkdir ~/pico/pico-examples/usb/device/pico-os-detect/build"
1. Move into the build directory with "cd ~/pico/pico-examples/usb/device/pico-os-detect/build"
1. Run "cmake .."
1. Run "make clean all"

# Flashing the UF2

1. Compile your own UF2 or download the one from above
1. Press and hold the BOOTSEL button and connect your Pico to your computer
1. Copy the "pico-os-detect.uf2" file to the RPI-RP2 and wait for it to reboot

# Viewing the output

The device will only output to the UART interface in pins 1 and 2 (see hardware setup above) connect that up to something that will let you see the output at 115200 baud 8 bits and 1 stop bit no parity then plug the pico into a machine that you want to try the detection on. You will see output something like this

```
DescriptorIndex -> 3, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 3, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 0, wIndex -> 0, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 1, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 2, wIndex -> 409, wLength -> FF
DescriptorIndex -> 3, wIndex -> 409, wLength -> 202
DescriptorIndex -> 1, wIndex -> 409, wLength -> 202
DescriptorIndex -> 2, wIndex -> 409, wLength -> 202
DescriptorIndex -> 2, wIndex -> 409, wLength -> 20A
DescriptorIndex -> 2, wIndex -> 409, wLength -> 20A
DescriptorIndex -> 2, wIndex -> 409, wLength -> 20A
DescriptorIndex -> 2, wIndex -> 409, wLength -> 20A
DescriptorIndex -> 3, wIndex -> 409, wLength -> 202
DescriptorIndex -> 1, wIndex -> 409, wLength -> 202
DescriptorIndex -> 2, wIndex -> 409, wLength -> 202
I think this is a Windows machine, but the version that makes lots of requests
```

# Detection didn't work?

Grab your output, and open an issue with the details of the device and OS that you had your pico connected to!

