# RTC application python scripts

## Overview

This script allows to estimate the shift between nodes' time and real global time value.
The rtc application is sending rtc message of the estimation of the rtc and the script retrieves them, thanks to a MQTT connection.
Then, the script allows to store the rtc data to a file which can be used to draw the rtc time differences.

## test_time_difference

The script subscribes to the Data Event topic of the MQTT client connected to the gateway.
Each time a RTC data message is received, the gateway time at which the message was sent to the MQTT is taken and compared to the expected RTC time of the nodes when the message arrived at the sink.
A message will be displayed to see the useful information.
The message should not spend too much time between the sink and the MQTT to be critical in the time difference calculus.

The script can be interrupted at any time but it will stop listening to the MQTT.
It can be launch again with the same parameter but, it will add the next data at the end of the file.

To stop this endless algorithm, the user needs to input 'q' or 'quit()' in line command or stop it with interuption as ctrl+C.
The script will update the draw then exit.


Please run "python test_time_difference.py --help" to see the available options for more information.


## Notes

A precision of the travel time from nodes to gateway is only of 1/128s while it is of 1/1024s in downlink.
Furthermore, it takes around 10ms to send a message from the gateway.
Therefore, the precision of the time difference shown in the output of the script should be of 15-20ms.

[test_time_difference]: ./test_time_difference.py
