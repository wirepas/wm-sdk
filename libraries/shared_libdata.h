/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file shared_libdata.h
 *
 * Shared_LibData library has been renamed Shared_Data.
 *
 * Please change the following in your code to be compatible with the new
 * library name:
 *  - #include "shared_data.h" instead of #include "shared_libdata.h"
 *  - Shared_Data_init(); instead of Shared_LibData_init();
 *  - Shared_Data_addDataReceivedCb(...); instead of
 *    Shared_LibData_addDataReceivedCb();
 *  - Shared_Data_removeDataReceivedCb(...); instead of
 *    Shared_LibData_removeDataReceivedCb();
 *  - Shared_Data_sendData(...); instead of Shared_LibData_sendData(...)
 *
 * Note also that Multicast packet filtering has changed to make it simpler
 * to manage large number of multicast groups.
 */

#ifndef _SHARED_LIBDATA_H_
#define _SHARED_LIBDATA_H_

#error "Shared_LibData library as been renamed, please see libraries/shared_libdata.h for an explanation."

#endif //_SHARED_LIBDATA_H_
