# Shared_data

## Application scope

This application can be used to demonstrate how to use the Shared data library, which is made to share send and receive functions/callback between multiple modules in the application.

## Application API 

The following packet filters are present in this application :
- Unicast packet on endpoints [10:10]
- All types of packet (Unicast/Multicast/Broadcast) on endpoints [20:20]
- Multicast packets on group#2 (0x80000002) on any endpoints.

Each packets sent on endpoints [20:20] will modify filter to [30:30] and vice-versa.

## SDK libraries used in this application

This application uses the [shared_data](../../../libraries/shared_data/shared_data.h) library. 

It also uses the [app_scheduler library](../../../libraries/scheduler/app_scheduler.h) library. 



