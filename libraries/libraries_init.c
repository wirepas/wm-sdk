/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "libraries_init.h"

// Libraries are considered in use if their associated interface (.h)
// file is in the include list. It could be done with a dedicated C flag
// but it would have the same effect

#if __has_include("app_scheduler.h")
#include "app_scheduler.h"
#endif

#if __has_include("shared_appconfig.h")
#include "shared_appconfig.h"
#endif

#if __has_include("shared_data.h")
#include "shared_data.h"
#endif

#if __has_include("stack_state.h")
#include "stack_state.h"
#endif

#if __has_include("app_persistent.h")
#include "app_persistent.h"
#endif

#if __has_include("shared_beacon.h")
#include "shared_beacon.h"
#endif

#if __has_include("shared_neighbors.h")
#include "shared_neighbors.h"
#endif

#if __has_include("shared_offline.h")
#include "shared_offline.h"
#endif

#if __has_include("rtc.h")
#include "rtc.h"
#endif

void Libraries_init(void)
{
#if __has_include("app_scheduler.h")
    App_Scheduler_init();
#endif

#if __has_include("shared_appconfig.h")
    Shared_Appconfig_init();
#endif

#if __has_include("shared_data.h")
    Shared_Data_init();
#endif

#if __has_include("stack_state.h")
    Stack_State_init();
#endif

#if __has_include("app_persistent.h")
    App_Persistent_init();
#endif

#if __has_include("shared_beacon.h")
    Shared_Beacon_init();
#endif

#if __has_include("shared_neighbors.h")
    Shared_Neighbors_init();
#endif

#if __has_include("shared_offline.h")
    Shared_Offline_init();
#endif

#if __has_include("rtc.h")
    RTC_init();
#endif

}
