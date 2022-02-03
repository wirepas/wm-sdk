# Dual boot example

## Application scope

This application demonstrates how a dual boot solution can be implemented.

The system will have the Wirepas application running and in a dedicated area
in flash a second app that will not be able to communicate with the Wirepas stack.

Features:
- Switch from one application to the other
- Update the second app through Wirepas OTAP
- Exchange data from one application to the other through persistent storage

To make this example more concrete, a second app
is provided that executes a Zephyr ble example.

To reproduce the same setup and make modification, please check this [Readme](second_app/zephyr_ble_app/Readme.md).

## Application behavior

This application will by default starts by executing the second app (Zephyr Ble app in this example)

### Second app

Application is BLE GATT Thermometer that will be advertised as "Wirepas Dual boot Health Thermometer".

When pressing button 0 on the board, it will reboot to Wirepas app.

### In Wirepas app

When Wirepas app is booted, it will try to connect a Wirepas network with its hardcoded settings from config.mk.

When pressing button 0 on the board or receiving message on EP 5 from wirepas network (TO BE ADDED), it will reboot to ble app.

## How does it work?

### Build configurartion
A dedicated area for the second app is created in the app [ini file](nrf52840_app.ini).
And another area for shared storage is created (already present by default in other app examples).

The second app as an hex must be placed in [second_app](second_app) folder, with name app.hex

It will automatically be injected in the final hex image to be flashed to the device.

### Dynamic switch
The first word from this shared storage area is used by both app to implement the switch.
In fact, if an app wants to reboot to the other app, it writes or clear this first word and reboot the device.

In this example the logic is as followed:
- First word is 0xffffffff => second app is started
- First word is anything else => Wirepas app is started

This behavior can be modified in [this file](bootloader/app_early_init.c).

### Sharing data
Like what is done with the switch info in first word of the shared area, all the rest of this area can be used to exchange data.

## Customization

The dedicated area for the second app can have a different size depending on the need.
But if area is modified, do not forget to reflect the change also in [this file](bootloader/app_early_init.c) and still ensure that the second app is linked to be executed in this area.

## How to an Otap of second app
As the second app is flashed in a dedicated Wirepas flash area declared in the app [ini file](nrf52840_app.ini), it can be OTAPed with standard Wirepas OTAP mechanism.

In order to build a scratchpad image containing a new version of your second app, here are the steps to follow:
(In this case, we assume that nothing was changed compared to application example. Any modification like area id of second application or application name changed, must be reflected here)

1. The full application must have been build one time to ensure that build/pca10056/dual_boot/bootloader_full_config.ini is present
2. Build the new version of your app. *new_second_app.hex* in the name of your new second application
3. Run the following command from the root of SDK:
```shell
tools/genscratchpad.py --configfile build/pca10056/dual_boot/bootloader_full_config.ini new_second_app.otap 0.0.0.0:0xab67de06:new_second_app.hex
```
Version of app is 0.0.0.0 as it is anyway not stored in the second app memory area.
4. *new_second_app.otap* can now be used to update the network
