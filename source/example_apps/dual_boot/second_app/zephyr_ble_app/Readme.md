# Zephyr app for Dual boot example

The Zephyr version used in this example is from [here](https://github.com/zephyrproject-rtos/zephyr/tree/329321714dba30a218274d2336aaec79fab12b92).
It will probably work also on newer/older version but may require some adaptations.

And the base example used is [samples/bluetooth/peripheral_ht](https://github.com/zephyrproject-rtos/zephyr/tree/329321714dba30a218274d2336aaec79fab12b92/samples/bluetooth/peripheral_ht)

## Apply the patch

The provided [patch](0001-Wirepas-Dual-Boot-demo.patch) will add the "switch" feature to the application.
Code should be sufficiently commented to explain the behavior.

Patch can be applied with [git am tool](https://git-scm.com/docs/git-am).

## How to build your app

To instruct Zephyr build system about image destination in flash and give the right information for app to access the shared memory, an [overlay](nrf52840dk_nrf52840_wm_dualboot.overlay) is needed.

This overlay will remove some flash partitions (not needed as OTAP is done from Wirepas side) and move the useful one to be aligned with Wirepas vision of flash defined in [nrf52840_app.ini](../../nrf52840_app.ini).

If flash areas are modified, both files must stay in sync.

To build your app and prepare your environment, please follow the good [Getting Started](https://docs.zephyrproject.org/2.7.0/getting_started/index.html) guide from Zephyr and stop at the [build the Blinky Sample section](https://docs.zephyrproject.org/2.7.0/getting_started/index.html#build-the-blinky-sample).

The build command to apply the overlay is the following one:

```
west build -p auto -b nrf52840dk_nrf52840 samples/bluetooth/peripheral_ht/ -DDTC_OVERLAY_FILE=<full path to file nrf52840dk_nrf52840_wm_dualboot.overlay>
```
Please note that the path to the overlay file must be absolute.

Once done, copy the built Zephyr firmware generated in build/zephyr/zephyr.hex to [this folder](.) with app.hex name.

For convenience, a prebuilt image is already available.

