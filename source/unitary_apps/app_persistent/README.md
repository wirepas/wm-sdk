# Application persistent

## Application scope

This application demonstrates how to read and write in the persistent memory freely available to the user application.  

## Application API 

This application demonstrate the following functionalities: 
- Check if app_persistent area is present
- Write content to persistent area
- Read it back
- Check if content read back matches with what is expected.

## SDK libraries used in this application

It uses the [app_persistent library](../../../libraries/app_persistent/app_persistent.h).

## Customization
This application can be used to save some data in the persistent memory of a node. 
For example, it is done for the multicast group in the [Low_latency application](https://developer.wirepas.com/support/solutions/articles/77000509314-getting-started-with-low-latency-application).


