#include "dualmcu_lib.h"
#include "waps.h"

dualmcu_lib_res_e Dualmcu_lib_init(uint32_t baudrate, bool flow_ctrl)
{
    // Initialize the Dual-MCU API protocol
    if (Waps_init(baudrate, flow_ctrl))
    {
        return DUALMCU_LIB_RES_OK;
    }
    else
    {
        return DUALMCU_LIB_RES_INTERNAL_ERROR;
    }
}
