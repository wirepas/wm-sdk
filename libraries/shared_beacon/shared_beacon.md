# Shared beacon library

## Overview

This wrapper allows the usage of the Wirepas SDK internal API lib_beacon_tx library functions to enable and disable multiple beacons.

There is possibility to start up to max APP_LIB_BEACON_TX_MAX_INDEX beacons using the same interval.
The smallest interval set is selected dynamically as common send interval

Following lib_beacons_tx functions are used in the library:

app_lib_beacon_tx_clear_beacons_f          lib_beacon_tx->clearBeacons()
app_lib_beacon_tx_enable_beacons_f         lib_beacon_tx->enableBeacons()
app_lib_beacon_tx_set_beacon_interval_f    lib_beacon_tx->setBeaconInterval()
app_lib_beacon_tx_set_beacon_power_f       lib_beacon_tx->setBeaconPower()
app_lib_beacon_tx_set_beacon_channels_f    lib_beacon_tx->setBeaconChannels()
app_lib_beacon_tx_set_beacon_contents_f    lib_beacon_tx->setBeaconContents()

## Example

Example how to use:

Init needs to called only once in the firmware:
Shared_Beacon_init();
If this library is enabled, init call is automatically called and application doesn't
have to call it again.



Variable needed to get the index of the activated beacon tx:
static uint8_t shared_beacon_index_1;

Starting the beacon:

    shared_beacon_res_e sh_res;

    sh_res = Shared_Beacon_startBeacon(interval,
                                      &power_out,
                                      channels,
                                      beacon,
                                      beacon_num_bytes,
                                      &shared_beacon_index_1);

    if (sh_res != SHARED_BEACON_RES_OK)
    {
        Error code from shared_becon_res_e needs to be checked
    }
...

Using the same function there is possibility to enable other beacons with own parameters.

If needed to stop the beacon:
    Shared_Beacon_stopBeacon(shared_beacon_index_1);