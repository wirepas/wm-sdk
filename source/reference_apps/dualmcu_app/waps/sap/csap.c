/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "csap.h"
#include "waps_private.h"
#include "waps_frames.h"
#include "function_codes.h"
#include "waddr.h"
#include "lock_bits.h"
#include "api.h"
#include "comm/uart/waps_uart.h"
#include "sap/persistent.h"

/** Key for reset command ("DoIt" in ASCII) */
#define RESET_KEY 0x74496f44

/* App attribute node roles */
typedef enum
{
    /** Node role config nibble */
    APP_ATTR_NODE_ROLE_INVALID              = 0x00,
    APP_ATTR_NODE_ROLE_SINK                 = 0x01,
    APP_ATTR_NODE_ROLE_NODE                 = 0x02,
    APP_ATTR_NODE_ROLE_SUBNODE              = 0x03,
    APP_ATTR_NODE_ROLE_MASK                 = 0x0F,
    /** Node role special mask nibble */
    APP_ATTR_NODE_ROLE_LL                   = 0x10,
    APP_ATTR_NODE_ROLE_RESV0                = 0x20,
    APP_ATTR_NODE_ROLE_RESV1                = 0x40,
    APP_ATTR_NODE_ROLE_AUTO                 = 0x80,
    APP_ATTR_NODE_ROLE_SPECIAL_MASK         = 0xF0,
} app_attr_node_role_e;

/* Map attr id to attr length */
static const uint8_t m_attr_size_lut[] =
{
    CSAP_ATTR_NODE_ID_SIZE,
    CSAP_ATTR_NETWORK_ADDRESS_SIZE,
    CSAP_ATTR_NETWORK_CHANNEL_SIZE,
    CSAP_ATTR_NODE_ROLE_SIZE,
    CSAP_ATTR_APDU_SIZE_SIZE,
    CSAP_ATTR_PDU_BUFF_SIZE_SIZE,
    CSAP_ATTR_SCRATCHPAD_SEQ_SIZE,
    CSAP_ATTR_WAPS_VERSION_SIZE,
    CSAP_ATTR_FIRMWARE_MAJOR_SIZE,
    CSAP_ATTR_FIRMWARE_MINOR_SIZE,
    CSAP_ATTR_FIRMWARE_MAINTENANCE_SIZE,
    CSAP_ATTR_FIRMWARE_DEVELOPMENT_SIZE,
    CSAP_ATTR_CIPHER_KEY_SIZE,
    CSAP_ATTR_AUTHENTICATION_KEY_SIZE,
    CSAP_ATTR_CHANNEL_LIMIT_SIZE,
    CSAP_ATTR_APPCFG_MAX_SIZE_SIZE,
    CSAP_ATTR_HWMAGIC_SIZE,
    CSAP_ATTR_STACK_PROFILE_SIZE,
    CSAP_ATTR_RESERVED_1_SIZE,
    CSAP_ATTR_OFFLINE_SCAN_SIZE,
    CSAP_ATTR_CHANNEL_ALLOC_MAP_SIZE,
    CSAP_ATTR_FEATURE_LOCK_BITS_SIZE,
    CSAP_ATTR_FEATURE_LOCK_KEY_SIZE,
    CSAP_ATTR_RESERVED_2_SIZE,
    CSAP_ATTR_RESERVED_CHANNELS_SIZE,
};

static bool attrReadReq(waps_item_t * item);
static attribute_result_e readAttr(attr_t attr_id,
                                   uint8_t * value,
                                   uint8_t * attr_size_p);
static bool attrWriteReq(waps_item_t * item);
static attribute_result_e writeAttr(attr_t attr_id,
                                    const uint8_t * value,
                                    uint8_t attr_size);
static bool resetReq(waps_item_t * item);
static attribute_result_e appRes2attrRes(app_res_e res)
{
    attribute_result_e result;

    switch(res)
    {
        case APP_RES_OK:
            result = ATTR_SUCCESS;
            break;
        case APP_RES_NOT_IMPLEMENTED:
            result = ATTR_UNSUPPORTED_ATTRIBUTE;
            break;
        case APP_RES_INVALID_VALUE:
        case APP_RES_INVALID_NULL_POINTER:
        case APP_RES_INVALID_CONFIGURATION:
            result = ATTR_INV_VALUE;
            break;
        case APP_RES_RESOURCE_UNAVAILABLE:
            result = ATTR_WRITE_ONLY;
            break;
        case APP_RES_INVALID_STACK_STATE:
            result = ATTR_INVALID_STACK_STATE;
            break;
        case APP_RES_ACCESS_DENIED:
            result = ATTR_ACCESS_DENIED;
            break;
        default:
            result = ATTR_UNSUPPORTED_ATTRIBUTE;
    }

    return result;
}

bool Csap_handleFrame(waps_item_t * item)
{
    switch (item->frame.sfunc)
    {
        case WAPS_FUNC_CSAP_ATTR_READ_REQ:
            return attrReadReq(item);
        case WAPS_FUNC_CSAP_ATTR_WRITE_REQ:
            return attrWriteReq(item);
        case WAPS_FUNC_CSAP_FACTORY_RESET_REQ:
            return resetReq(item);
        default:
            return false;
    }
}

static bool attrReadReq(waps_item_t * item)
{
    read_req_t * req_ptr = &item->frame.attr.read_req;
    attr_t attr_id = req_ptr->attr_id;
    attribute_result_e result = ATTR_UNSUPPORTED_ATTRIBUTE;
    uint32_t idx = attr_id - 1;
    uint8_t attr_size = 0;

    if (item->frame.splen != sizeof(read_req_t))
    {
        return false;
    }
    /* Check attribute ID */
    if (idx >= sizeof(m_attr_size_lut))
    {
        result = ATTR_UNSUPPORTED_ATTRIBUTE;
        goto build_response;
    }

    /* Check that CSAP attribute read feature is permitted */
    if (attr_id == CSAP_ATTR_SCRATCHPAD_SEQ)
    {
        if (!LockBits_isFeaturePermitted(LOCK_BITS_MSAP_SCRATCHPAD_STATUS))
        {
            result = ATTR_ACCESS_DENIED;
            goto build_response;
        }
    }
    else
    {
        if (!LockBits_isFeaturePermitted(LOCK_BITS_CSAP_ATTR_READ))
        {
            result = ATTR_ACCESS_DENIED;
            goto build_response;
        }
    }

    /* attribute found in LUT */
    attr_size = m_attr_size_lut[idx];

    /* Read attribute with attribute manager */
    result = readAttr(attr_id, item->frame.attr.read_cnf.attr, &attr_size);

    /* Processing done, build response over request */
build_response:
    Waps_item_init(item,
                   WAPS_FUNC_CSAP_ATTR_READ_CNF,
                   FRAME_READ_CNF_HEADER_SIZE);
    if (result == ATTR_SUCCESS)
    {
        item->frame.splen += attr_size;
        item->frame.attr.read_cnf.attr_len = attr_size;
    }
    else
    {
        item->frame.attr.read_cnf.attr_len = 0;
    }
    item->frame.attr.read_cnf.result = (uint8_t)result;
    item->frame.attr.read_cnf.attr_id = attr_id;
    return true;
}

static attribute_result_e readAttr(attr_t attr_id,
                                   uint8_t * value,
                                   uint8_t * attr_size_p)
{
    uint32_t tmp;   // Needed for 32-bit alignment
    app_res_e result = APP_RES_OK;
    app_addr_t addr;
    app_lib_settings_net_addr_t net_addr;
    app_lib_settings_net_channel_t net_ch;
    app_firmware_version_t fw_version;
    app_lib_system_radio_info_t radio_info;
    uint8_t attr_size = *attr_size_p;

    switch (attr_id)
    {
        case CSAP_ATTR_NODE_ID:
            result = lib_settings->getNodeAddress(&addr);
            if (result  == APP_RES_OK)
            {
                tmp = Addr_to_Waddr(addr);
            }
            break;
        case CSAP_ATTR_NETWORK_ADDR:
            result = lib_settings->getNetworkAddress(&net_addr);
            if (result == APP_RES_OK)
            {
                tmp = net_addr;
            }
            break;
        case CSAP_ATTR_NETWORK_CHANNEL:
            result = lib_settings->getNetworkChannel(&net_ch);
            if (result == APP_RES_OK)
            {
                tmp = net_ch;
            }
            break;
        case CSAP_ATTR_NODE_ROLE:
            {
                app_lib_settings_role_t role;
                result = lib_settings->getNodeRole(&role);
                if (result == APP_RES_OK)
                {
                    app_attr_node_role_e attr_role;
                    // Convert app role to attribute manager role
                    switch (app_lib_settings_get_base_role(role))
                    {
                        case APP_LIB_SETTINGS_ROLE_SINK:
                            attr_role = APP_ATTR_NODE_ROLE_SINK;
                            break;
                        case APP_LIB_SETTINGS_ROLE_HEADNODE:
                            attr_role = APP_ATTR_NODE_ROLE_NODE;
                            break;
                        case APP_LIB_SETTINGS_ROLE_SUBNODE:
                            attr_role = APP_ATTR_NODE_ROLE_SUBNODE;
                            break;
                        default:
                            // Unknown role
                            attr_role = APP_ATTR_NODE_ROLE_INVALID;
                    }

                    // Add role flags
                    app_lib_settings_role_t flags =
                        app_lib_settings_get_flags_role(role);
                    if (flags & APP_LIB_SETTINGS_ROLE_FLAG_LL)
                    {
                        attr_role |= APP_ATTR_NODE_ROLE_LL;
                    }
                    if (flags & APP_LIB_SETTINGS_ROLE_FLAG_AUTOROLE)
                    {
                        attr_role |= APP_ATTR_NODE_ROLE_AUTO;
                    }
                    tmp = (uint32_t)attr_role;
                }
            }
            break;
        case CSAP_ATTR_APP_MAXT_TRANS_UNIT:
            tmp = lib_data->getDataMaxNumBytes();
            break;
        case CSAP_ATTR_SCRATCHPAD_SEQ:
            tmp = lib_otap->getSeq();
            break;
        case CSAP_ATTR_WAPS_VERSION:
            tmp = WAPS_VERSION;
            break;
        case CSAP_ATTR_FIRMWARE_MAJOR:
            fw_version = global_func->getStackFirmwareVersion();
            tmp = fw_version.major;
            break;
        case CSAP_ATTR_FIRMWARE_MINOR:
            fw_version = global_func->getStackFirmwareVersion();
            tmp = fw_version.minor;
            break;
        case CSAP_ATTR_FIRMWARE_MAINTENANCE:
            fw_version = global_func->getStackFirmwareVersion();
            tmp = fw_version.maint;
            break;
        case CSAP_ATTR_FIRMWARE_DEVELOPMENT:
            fw_version = global_func->getStackFirmwareVersion();
            tmp = fw_version.devel;
            break;
        case CSAP_ATTR_PDU_BUFF_SIZE:
            tmp = lib_data->getNumBuffers();
            break;
        case CSAP_ATTR_CIPHER_KEY:
            if (lib_settings->getEncryptionKey(NULL) == APP_RES_OK)
            {
                result = APP_RES_RESOURCE_UNAVAILABLE;
            }
            else
            {
                result = APP_RES_INVALID_CONFIGURATION;
            }
            break;
        case CSAP_ATTR_AUTHENTICATION_KEY:
            if (lib_settings->getAuthenticationKey(NULL) == APP_RES_OK)
            {
                result = APP_RES_RESOURCE_UNAVAILABLE;
            }
            else
            {
                result = APP_RES_INVALID_CONFIGURATION;
            }
            break;
        case CSAP_ATTR_CHANNEL_LIMITS:
            {
                uint16_t min_ch_nbr = 0;
                uint16_t max_ch_nbr = 0;
                lib_settings->getNetworkChannelLimits(&min_ch_nbr, &max_ch_nbr);
                tmp = (((uint32_t)(min_ch_nbr & 0xff)) |
                       (((uint32_t)(max_ch_nbr & 0xff)) << 8));
                break;
            }
        case CSAP_ATTR_APPCFG_MAX_SIZE:
            tmp = lib_data->getAppConfigNumBytes();
            break;
        case CSAP_ATTR_HWMAGIC:
            lib_system->getRadioInfo(&radio_info, sizeof(radio_info));
            tmp = radio_info.hardware_magic;
            break;
        case CSAP_ATTR_STACK_PROFILE:
            lib_system->getRadioInfo(&radio_info, sizeof(radio_info));
            tmp = radio_info.protocol_profile;
            break;
        case CSAP_ATTR_OFFLINE_SCAN:
            {
                uint16_t max_scan = 0;
                result = lib_settings->getOfflineScan(&max_scan);
                tmp = max_scan;
            }
            break;
        case CSAP_ATTR_FEATURE_LOCK_BITS:
            result = lib_settings->getFeatureLockBits(&tmp);
            break;
        case CSAP_ATTR_FEATURE_LOCK_KEY:
            result = lib_settings->getFeatureLockKey(NULL);
            break;
        case CSAP_ATTR_CHANNEL_ALLOC_MAP:
            result = lib_settings->getChannelMap(&tmp);
            break;
        case CSAP_ATTR_RESERVED_CHANNELS:
            /* Determine actual attribute size, which can vary */
            attr_size = (get_num_channels() + 7) / 8;
            if (attr_size > WAPS_MAX_ATTR_LEN)
            {
                attr_size = WAPS_MAX_ATTR_LEN;
            }
            /* Read directly to value buffer */
            result = lib_settings->getReservedChannels(value, attr_size);
            /* Update size and mark as done, so value is not overwritten */
            *attr_size_p = attr_size;
            attr_size = 0;
            break;
        case CSAP_ATTR_RESERVED_1:
        case CSAP_ATTR_RESERVED_2:
        default:
            /* Unsupported attribute */
            result = APP_RES_NOT_IMPLEMENTED;
            break;
    }

    if ((result == APP_RES_OK) && (attr_size > 0))
    {
        memcpy(value, &tmp, attr_size);
    }

    return appRes2attrRes(result);
}

static bool attrWriteReq(waps_item_t * item)
{
    write_req_t * req_ptr = &item->frame.attr.write_req;
    attr_t attr_id = req_ptr->attr_id;
    uint32_t idx = attr_id - 1;
    uint8_t attr_size = 0;
    attribute_result_e result;

    if (item->frame.splen != (FRAME_WRITE_REQ_HEADER_SIZE +
                             req_ptr->attr_len))
    {
        return false;
    }
    /* Check attribute ID */
    if (idx >= sizeof(m_attr_size_lut))
    {
        result = ATTR_UNSUPPORTED_ATTRIBUTE;
        goto build_response;
    }

    /* Check that CSAP attribute feature lock bits write is permitted */
    if ((attr_id == CSAP_ATTR_FEATURE_LOCK_BITS) && LockBits_isKeySet())
    {
        result = ATTR_ACCESS_DENIED;
        goto build_response;
    }

    /* Check that CSAP attribute write feature is permitted */
    if ((attr_id != CSAP_ATTR_FEATURE_LOCK_KEY) &&
        !LockBits_isFeaturePermitted(LOCK_BITS_CSAP_ATTR_WRITE))
    {
        result = ATTR_ACCESS_DENIED;
        goto build_response;
    }

    /* attribute found in LUT */
    attr_size = m_attr_size_lut[idx];

    /* Check length */
    if (attr_size == 0)
    {
        /* Allow variable size attribute */
        attr_size = req_ptr->attr_len;
    }
    else if (attr_size != req_ptr->attr_len)
    {
        result = ATTR_INV_LENGTH;
        goto build_response;
    }

    /* Write attribute with attribute manager */
    result = writeAttr(attr_id, req_ptr->attr, attr_size);

    /* Processing done, build response over request */
build_response:
    Waps_item_init(item,
                   WAPS_FUNC_CSAP_ATTR_WRITE_CNF,
                   sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = (uint8_t)result;

    return true;
}

static attribute_result_e writeAttr(attr_t attr_id,
                                    const uint8_t * value,
                                    uint8_t attr_size)
{
    app_res_e result = APP_RES_OK;
    app_addr_t addr;
    app_lib_settings_net_addr_t net_addr;
    app_lib_settings_net_channel_t net_ch;
    uint32_t tmp = 0;
    if (attr_size <= 4)
    {
        memcpy(&tmp, value, attr_size);
    }
    switch (attr_id)
    {
        case CSAP_ATTR_NODE_ID:
            addr = Waddr_to_Addr(tmp);
            result = lib_settings->setNodeAddress(addr);
            break;
        case CSAP_ATTR_NETWORK_ADDR:
            net_addr = tmp;
            result = lib_settings->setNetworkAddress(net_addr);
            break;
        case CSAP_ATTR_NETWORK_CHANNEL:
            net_ch = tmp;
            result = lib_settings->setNetworkChannel(net_ch);
            break;
        case CSAP_ATTR_NODE_ROLE:
            {
                uint8_t role = (uint8_t) tmp;
                app_lib_settings_role_t app_role = APP_LIB_SETTINGS_ROLE_SINK;
                // Convert attribute manager role to app role
                switch (role & APP_ATTR_NODE_ROLE_MASK)
                {
                    case APP_ATTR_NODE_ROLE_SINK:
                        app_role = APP_LIB_SETTINGS_ROLE_SINK;
                        break;
                    case APP_ATTR_NODE_ROLE_NODE:
                        app_role = APP_LIB_SETTINGS_ROLE_HEADNODE;
                        break;
                    case APP_ATTR_NODE_ROLE_SUBNODE:
                        app_role = APP_LIB_SETTINGS_ROLE_SUBNODE;
                    break;
                    default:
                        return appRes2attrRes(APP_RES_INVALID_VALUE);
                }

                // Add flags
                if (role & APP_ATTR_NODE_ROLE_LL)
                {
                    app_role |= APP_LIB_SETTINGS_ROLE_FLAG_LL;
                }
                if (role & APP_ATTR_NODE_ROLE_AUTO)
                {
                    app_role |= APP_LIB_SETTINGS_ROLE_FLAG_AUTOROLE;
                }
                result = lib_settings->setNodeRole(app_role);

                if (result == APP_RES_OK)
                {
                    Waps_uart_powerReset();
#ifdef OTAP_FORCE_LEGACY
                    bool firstboot = false;

                    Persistent_isFirstboot(&firstboot);

                    if ((app_role & APP_LIB_SETTINGS_BASE_ROLE_MASK) ==
                                                APP_LIB_SETTINGS_ROLE_SINK &&
                        firstboot)
                    {
                        if (lib_otap->setTargetScratchpadAndAction(
                                                    0x00,
                                                    0x0000,
                                                    APP_LIB_OTAP_ACTION_LEGACY,
                                                    0x00) == APP_RES_OK)
                        {
                            Persistent_setFirstboot(false);
                        }
                    }
#endif // OTAP_FORCE_LEGACY
                }
            }
            break;
        case CSAP_ATTR_CIPHER_KEY:
            result = lib_settings->setEncryptionKey(value);
            break;
        case CSAP_ATTR_AUTHENTICATION_KEY:
            result = lib_settings->setAuthenticationKey(value);
            break;
        case CSAP_ATTR_OFFLINE_SCAN:
            result = lib_settings->setOfflineScan(tmp);
            break;
        case CSAP_ATTR_CHANNEL_ALLOC_MAP:
            result = lib_settings->setChannelMap(tmp);
            break;
        case CSAP_ATTR_FEATURE_LOCK_BITS:
            result = lib_settings->setFeatureLockBits(tmp);
            break;
        case CSAP_ATTR_FEATURE_LOCK_KEY:
            result = lib_settings->setFeatureLockKey(value);
            break;
        case CSAP_ATTR_RESERVED_CHANNELS:
            result = lib_settings->setReservedChannels(value, attr_size);
            break;
        case CSAP_ATTR_RESERVED_1:
        case CSAP_ATTR_RESERVED_2:
        default:
            // Unsupported attribute
            result = APP_RES_NOT_IMPLEMENTED;
            break;
    }

    return appRes2attrRes(result);
}

static bool resetReq(waps_item_t * item)
{
    csap_reset_e result = CSAP_RESET_OK;

    if (item->frame.splen != sizeof(csap_reset_req_t))
    {
        return false;
    }

    if (!LockBits_isFeaturePermitted(LOCK_BITS_CSAP_FACTORY_RESET))
    {
        result = CSAP_RESET_ACCESS_DENIED;
        goto create_response;
    }

    // Check the reset key
    if(item->frame.csap.reset_req.reset_key != RESET_KEY)
    {
        result = CSAP_RESET_INVALID_KEY;
    }
    else
    {
        // Do the reset
        if (lib_settings->resetAll() != APP_RES_OK)
        {
            result = CSAP_RESET_INVALID_STATE;
        }
        else
        {
            // Need to re-init as flash content is modified by the stack.
            Persistent_init();
        }
    }
create_response:
    /* Processing done, build response over request */
    Waps_item_init(item,
                   WAPS_FUNC_CSAP_FACTORY_RESET_CNF,
                   sizeof(simple_cnf_t));
    item->frame.simple_cnf.result = (uint8_t) result;
    return true;
}
