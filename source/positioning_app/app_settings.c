/**
    @file   app_settings.c
    @brief  Application settings module.

    @copyright  Wirepas Oy 2019
*/
#include "api.h"
#include "app_settings.h"
#include "measurements.h"
#include "ble_beacon.h"


static positioning_settings_t m_app_settings;


/**
    @brief      Sets the device settings.
*/
static void set_device_settings()
{
    app_addr_t node_address;

    if (lib_settings->getNodeAddress(&node_address) ==
            APP_RES_INVALID_CONFIGURATION)
    {
        lib_settings->setNodeAddress(getUniqueAddress());
        lib_settings->getNodeAddress(&m_app_settings.node_address);

        // enforce the node role, as this is the 1st boot
        lib_settings->setNodeRole(m_app_settings.node_role);
    }
    else
    {
        m_app_settings.node_address = node_address;
        lib_settings->getNodeRole(&m_app_settings.node_role);
    }
}

/**
    @brief      Sets the network settings for the device.
*/
static void set_network_settings()
{
    app_lib_settings_net_addr_t network_address;
    app_lib_settings_net_channel_t network_channel;

    // network settings
    if (lib_settings->getNetworkAddress(&network_address) ==
            APP_RES_INVALID_CONFIGURATION)
    {
        lib_settings->setNetworkAddress(m_app_settings.network_address);
        lib_settings->getNetworkAddress(&m_app_settings.network_address);
    }
    else
    {
        m_app_settings.network_address = network_address;
    }

    if (lib_settings->getNetworkChannel(&network_channel) ==
            APP_RES_INVALID_CONFIGURATION)
    {
        lib_settings->setNetworkChannel(m_app_settings.network_channel);
        lib_settings->getNetworkChannel(&m_app_settings.network_channel);
    }
    else
    {
        m_app_settings.network_channel = network_channel;
    }
}

/**
    @brief      Read configure values from the persistent memory and configure
                the device.
    @return     True if read and configured
*/
static bool read_persistent_memory()
{
#if defined(USE_PERSISTENT_MEMORY)

    app_addr_t node_addr;
    app_lib_settings_net_addr_t network_addr;
    app_lib_settings_net_channel_t network_ch;
    app_lib_settings_role_t role;

    // If one of the parameters haven't been set then this is the first boot.
    if ((lib_settings->getNodeRole(&role) != APP_RES_OK) ||
        (lib_settings->getNodeAddress(&node_addr) != APP_RES_OK) ||
        (lib_settings->getNetworkAddress(&network_addr) != APP_RES_OK) ||
        (lib_settings->getNetworkChannel(&network_ch) != APP_RES_OK))
    {
        if (Mcu_Persistent_isValid(PERSISTENT_MAGIC_KEY) == PERSISTENT_RES_OK)
        {
            persistent_settings_t data;
            uint8_t buffer[MAX_READ_SIZE+1] = {0};

            Mcu_Persistent_read(buffer, MAX_READ_SIZE);
            memcpy(&data, buffer, sizeof(persistent_settings_t));
            // Node Id
            if (data.node_address != 0xFFFFFF)
            {
                m_app_settings.node_address = data.node_address;
            }
            // Network address
            if (data.network_address != 0xFFFFFF)
            {
                m_app_settings.network_address = data.network_address;
            }
            // Network channel Id
            if (data.network_channel != 0xFF)
            {
                m_app_settings.network_channel = data.network_channel;
            }
            // Node role
            if (data.node_role != 0xFF)
            {
                m_app_settings.node_role = data.node_role;
            }

            // If the reading configure values from Persistent memory are
            // unformatted, then will be use default values from config.mk
            if (lib_settings->setNodeRole(m_app_settings.node_role) == APP_RES_OK)
            {
                if (configureNode(m_app_settings.node_address,
                                  m_app_settings.network_address,
                                  m_app_settings.network_channel,
                                  NULL,
                                  NULL) == APP_RES_OK)
                {
                    return true;
                }
            }
        }
    }
#endif
    return false;
}

/**
    @brief      Retrieves the settings.

    @return     The settings
*/
positioning_settings_t* Pos_get_settings(void)
{
    return &m_app_settings;
}

/**
    @brief      Initializes the settings to its default values.

    @param[in]  on_boot  On boot
*/
void Pos_settings_init(bool on_boot)
{
    // application settings - defaults set in positioning_app.h
    if(on_boot)
    {
        m_app_settings.destination_endpoint = DESTINATION_ENDPOINT;
        m_app_settings.source_endpoint = SOURCE_ENDPOINT;

        m_app_settings.max_beacons = MAX_BEACONS;

        m_app_settings.node_class = CONF_DEVICE_CLASS; // see config.mk
        switch(m_app_settings.node_class)
        {

            case POS_APP_CLASS_A:
            case POS_APP_CLASS_B:
            case POS_APP_CLASS_C:
            case POS_APP_CLASS_D:
            case POS_APP_CLASS_E:
            case POS_APP_CLASS_F:
                break;
            default:
                m_app_settings.node_class = POS_APP_CLASS_DEFAULT;
        }

        m_app_settings.node_mode = CONF_DEVICE_MODE; // see config.mk
        m_app_settings.payload_qos = APP_LIB_DATA_QOS_HIGH;

        // default values - will not be set if they are already present in the stack
        m_app_settings.network_address = CONF_NETWORK_ADDRESS; // see config.mk
        m_app_settings.network_channel = CONF_NETWORK_CHANNEL; // see config.mk

        m_app_settings.node_role_base = CONF_ROLE;// see config.mk
        m_app_settings.node_role_flag = CONF_ROLE_FLAG; // see config.mk
        m_app_settings.node_role = app_lib_settings_create_role(CONF_ROLE,
                                   CONF_ROLE_FLAG);

        m_app_settings.scan_filter = APP_LIB_STATE_SCAN_NBORS_ALL;
        m_app_settings.access_cycle = ACCESS_CYCLE_S;
        m_app_settings.scan_period = CONF_SCAN_PERIOD_S;

        m_app_settings.timeout_ack = TIMEOUT_ACK;
        m_app_settings.timeout_connection = TIMEOUT_CONNECTION;
    }

}

/**
    @brief      Configure the device.

    @param[in]  on_boot  On boot
*/
void App_Settings_configureNode(bool on_boot)
{
    bool ret;

    ret = read_persistent_memory();

    if (on_boot && !ret)
    {
        set_device_settings();
        set_network_settings();
    }

    switch(m_app_settings.node_role & APP_LIB_SETTINGS_BASE_ROLE_MASK)
    {
        case APP_LIB_SETTINGS_ROLE_SINK:
        case APP_LIB_SETTINGS_ROLE_HEADNODE:

            if(is_node_mode_anchor(CONF_DEVICE_MODE))
            {
                m_app_settings.node_mode = CONF_DEVICE_MODE;
            }
            else
            {
              //the default will be active if role is changed through remote API
                m_app_settings.node_mode = DEFAULT_ANCHOR_MODE;
            }
        break;

        case APP_LIB_SETTINGS_ROLE_SUBNODE:

           if(is_node_mode_tag(CONF_DEVICE_MODE))
            {
                m_app_settings.node_mode = CONF_DEVICE_MODE;
           }
           else
           {
                //the default will be active if role is changed through remote API
                m_app_settings.node_mode = DEFAULT_TAG_MODE;
           }
        break;
    }

}

/**
    @brief      Initializes the Ble Beacon module.
*/
void App_Settings_initBleBeacon()
{
    m_app_settings.ble_beacon_setup = CONF_BLEBEACON_SETUP;
    m_app_settings.ble_beacon_selection = CONF_BLEBEACON_SELECTION;
    m_app_settings.ble_beacon_offline_waittime = OFFLINE_TIMER_S;

    Ble_Beacon_init(m_app_settings.ble_beacon_setup);
}

/**
    @brief  Checks if the node mode is anchor.

    @param[in]  node_mode  The mode to be tested
*/
bool is_node_mode_anchor(positioning_mode_e node_mode)
{

bool ret = ((node_mode == POS_APP_MODE_AUTOSCAN_ANCHOR) ||
            (node_mode == POS_APP_MODE_OPPORTUNISTIC_ANCHOR));

return ret;
}

/**
    @brief  Checks if the node mode is tag.

    @param[in]  node_mode  The mode to be tested
*/
bool is_node_mode_tag(positioning_mode_e node_mode)
{
    bool ret = ((node_mode == POS_APP_MODE_NRLS_TAG)
                ||(node_mode == POS_APP_MODE_AUTOSCAN_TAG));
    return ret;
}
