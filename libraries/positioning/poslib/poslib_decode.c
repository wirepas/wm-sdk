/**
 * @file       poslib_decode.c
 * @brief      Decodes received app-config for PosLib configuration.
 *
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB_DECODE"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "poslib.h"
#include "poslib_ble_beacon.h"
#include "poslib_control.h"
#include "poslib_decode.h"
#include "tlv.h"
#include "poslib_tlv.h"
#ifdef MOTION_SUPPORTED
#include "motion.h"
#endif

/** BLE config - control special values */
#define POSLIB_DECODE_BEACON_OFF    0
#define POSLIB_DECODE_BEACON_ON     65535

/** Motion special values */
#define POSAPP_MOTION_THRESHOLD_MG_DISABLED 65535

/**
 * @brief   Checks if the node mode is anchor.
 * 
 * @param   node_mode  The mode to be tested
 * 
 * @return  true: anchor, false: not anchor
 */
static bool is_node_mode_anchor(poslib_mode_e node_mode)
{

bool ret = ((node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR) ||
            (node_mode == POSLIB_MODE_OPPORTUNISTIC_ANCHOR));

return ret;
}

/**
 * @brief  Checks if the node mode is tag.
 * 
 * @param  node_mode  The mode to be tested
 * 
 * @return true: tag, false: not tag
 */
static bool is_node_mode_tag(poslib_mode_e node_mode)
{
    bool ret = ((node_mode == POSLIB_MODE_NRLS_TAG)
                || (node_mode == POSLIB_MODE_AUTOSCAN_TAG) 
                || (node_mode == POSLIB_MODE_DA_TAG));
    return ret;
}

/**
 * @brief   Extract an uint (byte | word | dword) from TLV item
 * @param   item
 *          TLV containing the command
 * @param   uint_type
 *          the expected uint size (1 | 2 | 4 bytes)
 * @param   value
 *          Pointer to output value.
 * 
 * @note    The uint variable pointed must be intialized 
 *          to 0 if is larger than uint_type
 * @return  true: value decoded succesfully, false: value cannot be decoded
 *          
 */

bool get_uint(poslib_tlv_item_t *item, uint8_t uint_type,  void *value)
{

    if (uint_type == item->length && 
        (uint_type == sizeof(uint8_t) ||
        uint_type == sizeof(uint16_t) ||
        uint_type == sizeof(uint32_t)))
        {
            memcpy(value, item->value, item->length);
            return true;
        }
    return false;
}

#define get_uint8(i,v) get_uint(i, 1, v)
#define get_uint16(i,v) get_uint(i, 2, v)
#define get_uint32(i,v) get_uint(i, 4, v)

/**
 * @brief   Checks if node address record matched current node address
 * 
 * @param   item
 *          TLV containing the command
 * @return  true: match, false:  not a match | cannot decode
 *         
 */

bool check_node_address(poslib_tlv_item_t *item)
{
    uint32_t target_node_addr = 0;
    app_addr_t node_addr = 0;

    if(item->type != POSLIB_NODE_ADDR_SELECTION)
    {
        LOG(LVL_ERROR, "Expecting node selection type. Decoded %x",item->type);
        return false;
    }

    if(get_uint32(item, &target_node_addr) && 
        lib_settings->getNodeAddress(&node_addr) == APP_RES_OK && 
        target_node_addr == (uint32_t) node_addr)
    {
        LOG(LVL_INFO, "Node selection match. Addr %u", target_node_addr);
        return true;         
    }
    return false;
}

/**
 * @brief   Decode and set the static | dynamic | offline measurement rate
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */

static bool set_measurement_rate(poslib_tlv_item_t *item,
                                    poslib_settings_t * settings)
{
    uint32_t meas_rate_s = 0; 

    if (!get_uint16(item, &meas_rate_s) &&
        !get_uint32(item, &meas_rate_s))
    {
       LOG(LVL_ERROR, "Incorrect update rate! type %u, length %u", 
            item->type, item->length);
       return false;  
    }

    if (meas_rate_s > POSLIB_MAX_MEAS_RATE_S ||
        (meas_rate_s < POSLIB_MIN_MEAS_RATE_S && 
         meas_rate_s != 0))
    {
        LOG(LVL_ERROR, "Update rate type %u value: %u rejected!", 
            item->type, meas_rate_s);
        return false;
    }
 
    switch(item->type)
    {
        case POSLIB_MEASUREMENT_RATE_STATIC:
        {
            if(meas_rate_s == 0)
            {
                LOG(LVL_ERROR, "Update rate type %u value: %u rejected!", 
                item->type, meas_rate_s);
                return false;
            }
            settings->update_period_static_s = meas_rate_s;
            LOG(LVL_INFO, "Update period static %u sec", meas_rate_s);
            break;
        }
        case POSLIB_MEASUREMENT_RATE_DYNAMIC:
        {
            settings->update_period_dynamic_s = meas_rate_s;
            LOG(LVL_INFO, "Update period dynamic %u sec", meas_rate_s);
            break;
        }
        case POSLIB_MEASUREMENT_RATE_OFFLINE:
        {
            settings->update_period_offline_s = meas_rate_s;
            LOG(LVL_INFO, "Update period offline %u sec", meas_rate_s);
            break;
        }
        default:
        {
             LOG(LVL_ERROR,"Unknown measurement rate type: %u", item->type);
             return false;
        }
    }
    return true;
}

/**
 * @brief   Decode BLE setting
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_blebeacon(poslib_tlv_item_t *item,
                            poslib_settings_t * settings)
{
    uint16_t activation_delay_s;
    if (!get_uint16(item, &activation_delay_s))
    {
       LOG(LVL_ERROR, "Incorrect BLE control value");
       return false;  
    }

    settings->ble.activation_delay_s = 
        (uint16_t) activation_delay_s;

    switch(activation_delay_s)
    {
        case POSLIB_DECODE_BEACON_OFF:
        {
            settings->ble.mode = POSLIB_BLE_OFF;
            LOG(LVL_INFO, "BLE beacon mode: OFF");
            break;
        }
        case POSLIB_DECODE_BEACON_ON:
        {
            settings->ble.mode = POSLIB_BLE_ON;
            LOG(LVL_INFO, "BLE beacon mode: always ON");
            break;
        }
        default:
        {
            settings->ble.mode = POSLIB_BLE_ON_WHEN_OFFLINE;
            LOG(LVL_INFO, "BLE beacon mode: ON when offline. delay: %u s", activation_delay_s);
            break;
        }
    }
    return true;
}

/**
 * @brief   Check and set node class
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 */
static bool set_class(poslib_tlv_item_t * item, 
                        poslib_settings_t * settings)
{
    uint8_t class;
 
    if (!get_uint8(item, &class))
    {
       LOG(LVL_ERROR, "Incorrect class decoding");
       return false;  
    }

    switch(class)
    {
        case POSLIB_CLASS_DEFAULT:
        case POSLIB_CLASS_A:
        case POSLIB_CLASS_B:
        case POSLIB_CLASS_C:
        case POSLIB_CLASS_D:
        case POSLIB_CLASS_E:
        case POSLIB_CLASS_F:
        {
            settings->node_class = class;
            LOG(LVL_INFO, "Class changed to: %u", class);
            return true;
            break;
        }
        default:
        {
            LOG(LVL_ERROR, "Invalid class: %u", class);
            break;
        }
    }
    return false;
}

/**
 * @brief   Decode and trigger OTAP if requested
 * @param   item
 *          TLV containing the command
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_otap(poslib_tlv_item_t * item)
{
    bool is_valid = false;
    bool is_processed = false;
    uint8_t target_seq = 0;
    
    if (!get_uint8(item, &target_seq))
    {
       LOG(LVL_ERROR, "Incorrect OTAP setting decoding");
       return false;  
    }

    app_lib_otap_seq_t current_seq = 0;

    is_valid = lib_otap->isValid();
    is_processed = lib_otap->isProcessed();

    /** perform operation */
    if (is_valid && !is_processed)
    {
        current_seq = lib_otap->getSeq();
        LOG(LVL_INFO, "OTAP: current seq: %d target: %d)", current_seq, 
                        target_seq);

        if (target_seq == current_seq)
        {
            lib_otap->setToBeProcessed();
            LOG(LVL_WARNING, "OTAP: update triggered! Reboot!");

            /** Start the reboot */
            lib_state->stopStack();
        }
    }
    else
    {
        LOG(LVL_DEBUG, "OTAP: no update required");
    }
    return true;
}

/**
 * @brief   Decode and set opperating mode
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_mode(poslib_tlv_item_t * item, 
                        poslib_settings_t * settings, bool node_specific)
{
    app_lib_settings_role_t role;
    uint8_t node_mode;
    bool force = false;

    /* if node specific setting & persistent storage supported accept a mode 
    even when does not match the node role */
#ifdef CONF_USE_PERSISTENT_MEMORY
    force = node_specific;
#endif

    if (!get_uint8(item, &node_mode))
    {
       LOG(LVL_ERROR, "Incorrect node mode");
       return false;  
    }

    lib_settings->getNodeRole(&role);

    switch(role)
    {
        case APP_LIB_SETTINGS_ROLE_SINK_LE:
        case APP_LIB_SETTINGS_ROLE_SINK_LL:
        case APP_LIB_SETTINGS_ROLE_HEADNODE_LE:
        case APP_LIB_SETTINGS_ROLE_HEADNODE_LL:
        {
            /** only node modes for anchors are accepted */
            if(is_node_mode_anchor(node_mode) || force)
            {
                settings->node_mode = node_mode;
                LOG(LVL_INFO, "Update mode: %d", settings->node_mode);
            }
            break;
        }
        case APP_LIB_SETTINGS_ROLE_SUBNODE_LE:
        case APP_LIB_SETTINGS_ROLE_SUBNODE_LL:
        {    /** only node modes for tags are accepted */
            if(is_node_mode_tag(node_mode) || force)
            {
                settings->node_mode = node_mode;
                LOG(LVL_INFO, "Update mode: %d", settings->node_mode);
            }
            break;
        }
        case APP_LIB_SETTINGS_ROLE_ADVERTISER:
        {    /** only node modes for tags are accepted */
            if(is_node_mode_tag(node_mode) || force)
            {
                settings->node_mode = node_mode;
                LOG(LVL_INFO, "Update mode: %d", settings->node_mode);
            }
            break;
        }
        default:
        {
            LOG(LVL_ERROR, "Node role is not set: %d", role);
            break;
        }
    }
    return true;
}

#ifdef MOTION_SUPPORTED
/**
 * @brief   Decode motion threshold
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_motion_threshold(poslib_tlv_item_t *item,
                            poslib_settings_t * settings)
{
    uint16_t threshold_mg;
    if (!get_uint16(item, &threshold_mg))
    {
       LOG(LVL_ERROR, "Incorrect motion threshold value");
       return false;  
    }


    if (threshold_mg >= POSAPP_MOTION_THRESHOLD_MG_MIN && 
        threshold_mg <= POSAPP_MOTION_THRESHOLD_MG_MAX)
    {
       settings->motion.enabled = true;
       settings->motion.threshold_mg = threshold_mg; 
       return true;
    }
    else if (threshold_mg == POSAPP_MOTION_THRESHOLD_MG_DISABLED)
    {
        settings->motion.enabled = false;
        settings->motion.threshold_mg = POSAPP_MOTION_THRESHOLD_MG_MAX;
        return true;
    }
    else 
    {
        LOG(LVL_ERROR, "Motion threshold %u outside range.", threshold_mg);
    }
     return false;
}

/**
 * @brief   Decode motion duration
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib settings (updated if decoding succesfull)
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_motion_duration(poslib_tlv_item_t *item,
                            poslib_settings_t * settings)
{
    uint16_t duration_ms;
    if (!get_uint16(item, &duration_ms))
    {
       LOG(LVL_ERROR, "Incorrect motion duration value");
       return false;  
    }

    if (duration_ms <= POSAPP_MOTION_DURATION_MS_MIN || 
        duration_ms > POSAPP_MOTION_DURATION_MS_MAX)
    {
        LOG(LVL_ERROR, "Motion duration %u outside range.", duration_ms);
        return false;
    }

    settings->motion.duration_ms = duration_ms;
    return true;
}
#endif

/**
 * @brief   Decode led on duration
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib auxiliary settings
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_led_on_duration(poslib_tlv_item_t *item,
                                poslib_aux_settings_t * aux)
{
    uint16_t led_on_duration_s;
    if (!get_uint16(item, &led_on_duration_s))
    {
       LOG(LVL_ERROR, "Incorrect LED on duration value");
       return false;  
    }
    aux->led_on_duration_s = led_on_duration_s;
    return true;
}

/**
 * @brief   Decode led command
 * @param   item
 *          TLV containing the command
 * @param   settings
 *          Pointer to PosLib auxiliary settings
 * @return  bool
 *          true: parameters decoded succesfully, false: error encountered
 */
static bool set_led_cmd(poslib_tlv_item_t *item,
                        poslib_aux_settings_t * aux)
{
    uint8_t led_cmd_seq;
    if (!get_uint8(item, &led_cmd_seq))
    {
       LOG(LVL_ERROR, "Incorrect LED command sequence value");
       return false;  
    }
    aux->led_cmd_seq = led_cmd_seq;
    return true;
}

/**
 * @brief   Decodes PosLib TLV configuration commands for a given class.
 * 
 * @param   record Pointer to a record structure ( \ref poslib_tlv_record ) containing the data 
 * 
 * @param   node_specific  true if we have a node specific class, false otherwise
 * 
 * @param   settings
 *          Pointer to PosLib settings structure to be updated with 
 *          decoded parameters
 * @return  bool
 *          true: parameters decoded succesfully, false: no parameter were decoded | 
 *          error encountered 
 */
static bool decode_configuration_commands(poslib_tlv_record * record, bool node_specific, poslib_settings_t * settings, poslib_aux_settings_t * aux)
{
    bool ret = false;
    poslib_tlv_res_e res;
    poslib_tlv_item_t item;

    while (true)
    {
        res = PosLibTlv_Decode_getNextItem(record, &item); 
        if (res == POSLIB_TLV_RES_END)
        {
            break; //exit decoding loop
        }
        else if (res != POSLIB_TLV_RES_OK)
        {
           LOG(LVL_ERROR, "Incorrect configuration payload encoding!");
           break;  //exit decoding loop
        } 

        switch(item.type)
        {
            case POSLIB_MEASUREMENT_RATE_STATIC:
            case POSLIB_MEASUREMENT_RATE_DYNAMIC:
            case POSLIB_MEASUREMENT_RATE_OFFLINE:
            {
                ret |= set_measurement_rate(&item, settings);
                break;
            }
            case POSLIB_BEACON:
            {
                ret |= set_blebeacon(&item, settings);
                break;
            }
            case POSLIB_OPERATING_MODE:
            {
                ret |= set_mode(&item, settings, node_specific);
                break;
            }
            case POSLIB_OTAP:
            {
                ret |= set_otap(&item);
                break;
            }
            case POSLIB_DEVICE_CLASS_CHANGE:
            {
                if (node_specific)
                {
                    ret |= set_class(&item, settings);
                }
                break;
            }
#ifdef MOTION_SUPPORTED
            case POSLIB_MOTION_THRESHOLD:
            {
                ret |= set_motion_threshold(&item, settings);
                break;
            }
            case POSLIB_MOTION_DURATION:
            {
                ret |= set_motion_duration(&item, settings);
                break;
            }
#endif
            case POSLIB_LED_ON_DURATION:
            {
                ret |= set_led_on_duration(&item, aux);
                break;
            }
            case POSLIB_LED_CMD_SEQ:
            {
                ret |= set_led_cmd(&item, aux);
                break;
            }
            default:
            {
                LOG(LVL_INFO, "Unknown command: %u!", item.type);
                break;
            }
        }
    }

    return ret;
}

/**
 * @brief   Decodes PosLib TLV configuration for a given class.
 * 
 * @param   class
 *          The class for which decoding is done
 * @param   buffer
 *          Pointer to data buffer
 * @param   length
 *          Length of data buffer
 * 
* @param   settings
 *          Pointer to PosLib settings structure to be updated with 
 *          decoded parameters
 * @return  bool
 *          true: parameters decoded succesfully, false: no parameter were decoded | 
 *          error encountered 
 */
static bool decode_configuration_record(uint16_t class, uint8_t * buffer, uint8_t length,
                                        poslib_settings_t * settings, poslib_aux_settings_t * aux)
{
   
    poslib_tlv_record record;
    poslib_tlv_item_t item;
    poslib_tlv_res_e res;
    bool node_specific = false;
    bool ret = false;

    PosLibTlv_init(&record, buffer, length);

    /* If we decode the node specific class (0xF8) then the first item 
    shall be "node selection" */
    if (class == POSLIB_CLASS_NODE_SPECIFIC)
    {
        res = PosLibTlv_Decode_getNextItem(&record, &item); 
        if (res != POSLIB_TLV_RES_OK)
        {
           LOG(LVL_ERROR, "Incorrect encoding!");
           return false;  
        }

        if (!check_node_address(&item))
        {
            return false;
        }
        node_specific = true;
        //continue decoding when node address matches
    }

    ret = decode_configuration_commands(&record, node_specific, settings, aux);
    
    return ret;
}

 /** @brief   Iterates on app-config buffer, locates & decodes a given 
 *            class payload
 * 
 * @param   class
 *          The class for which decoding is done
 * @param   buffer
 *          Pointer to data buffer
 * @param   length
 *          Length of data buffer
 * 
 * @param   settings
 *          Pointer to PosLib settings structure to be updated with 
 *          decoded parameters
 * @return  bool
 *          true: parameters decoded succesfully, false: no parameter were decoded | 
 *          error encountered 
 */

bool decode_configuration(uint16_t class, uint8_t *buffer, uint8_t length,
                            poslib_settings_t * settings, poslib_aux_settings_t * aux)
{
    tlv_record record;
    tlv_item_t item;
    tlv_res_e tlv_res;
    bool ret = false;
   
    Tlv_init(&record, buffer, length);

    while (true)
    {
        tlv_res = Tlv_Decode_getNextItem(&record, &item);
        if (tlv_res == TLV_RES_END)
        {
            break;
        }
        else if (tlv_res != TLV_RES_OK)
        {
           LOG(LVL_ERROR, "Incorrect encoding!");
           break;  
        } 

        if (item.type == class)
        {
            ret |= decode_configuration_record(class, item.value, item.length,
                                                settings, aux);
        }    
    }
    return ret; 
}

bool PosLibDecode_config(const uint8_t * buffer, uint32_t length,
                            poslib_settings_t * settings, poslib_aux_settings_t * aux)
{
    bool ret = false;

    /* First pass:  decode node specific settings */
    ret |= decode_configuration(POSLIB_CLASS_NODE_SPECIFIC, (uint8_t *) buffer,
                                length, settings, aux);

    /* Second pass: decode the class specific settings. 
    Note that if node class changed due to node specific request the new node class 
    will be used */
    ret |= decode_configuration(settings->node_class, (uint8_t *) buffer, 
                                length, settings, aux);
    return ret;  
}