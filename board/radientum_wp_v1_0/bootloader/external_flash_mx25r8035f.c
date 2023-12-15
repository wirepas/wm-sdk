/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

// References:
// [1] MX25R8035F datasheet
// https://www.macronix.com/Lists/Datasheet/Attachments/8746/MX25R8035F,%20Wide%20Range,%2064Mb,%20v1.5.pdf
// [2] EFR32xG23 Wireless SoC Reference Manual
// https://www.silabs.com/documents/public/reference-manuals/efr32xg23-rm.pdf

#include <string.h>
#include "board.h"
#include "mcu.h"
#include "external_flash.h"
#include "external_flash_mx25r8035f.h"

#define MIN(x,y)                            ((x) < (y) ? (x) : (y))
#define USART_EXTFLASH_FREQUENCY            8000000

// Checked with a logic analyzer
#define LOOP_COUNT_PER_MS                   4240
#define TIMING_POWERDOWN_MAX_MS             10
#define TIMING_POWERON_MAX_MS               30
// Dummy byte for SPI transfer
#define DUMMY_BYTE                          0xFF

#define EXTFLASH_HIGH_PERF_MODE_SHIFT       1

// Configuration of the Control Register of the USART peripheral
// Refer to doc [2] section 20.5.3 USART_CTRL - Control Register
// and doc [1] section 8. DEVICE OPERATION
// Meaningful bits in the USART Control Register for SPI:
//   - SYNC (USART Synchronous Mode): set
//      USART operates in synchronous mode
//   - CLKPOL (Clock Polarity): unset (IDLELOW)
//      The bus clock used in synchronous mode has a low base value
//   - CLKPHA (Clock Edge For Setup/Sample): unset (SAMPLELEADING)
//      Data is sampled on the leading edge and set-up on the trailing
//      edge of the bus clock in synchronous mode
//   => CLKPOL = IDLELOW and CLKPHA = SAMPLELEADING => SPI clock mode 0
//   - MSBF (Most Significant Bit First): set (ENABLE)
//      Data is sent with the most significant bit first
#define USART_CTRL_REG_SPI_SETTING          (_USART_CTRL_RESETVALUE | \
                                             USART_CTRL_SYNC | \
                                             USART_CTRL_CLKPOL_IDLELOW | \
                                             USART_CTRL_CLKPHA_SAMPLELEADING | \
                                             USART_CTRL_MSBF)

//#define EXT_FLASH_DRIVER_DEBUG_LED

/* Timing test results (from test app using lib_memory)
 *  [                      ][Theorical (uS)][Measured (uS)][# samples]
 *  [byte_write_time       ][           100][           23][      256]
 *  [page_write_time       ][          4000][         1065][      255]
 *  [sector_erase_time     ][       1500000][       240098][        2]
 *  [byte_write_call_time  ][            43][           40][      256]
 *  [page_write_call_time  ][           590][          596][      255]
 *  [sector_erase_call_time][            40][           39][        2]
 *  [is_busy_call_time     ][            10][            9][    69516]
 *
 * For theorical values, refer to
 * section 15. ERASE AND PROGRAMMING PERFORMANCE in doc [1]
 */
static const flash_info_t m_flash_info = {
    .flash_size = 1024UL * 1024UL,        // 8-mbit flash
    .write_page_size = 256,
    .erase_sector_size = 32UL * 1024UL,   // 32kB
    .write_alignment = 1,
    .byte_write_time = 100,               // Typical 32, Max 100
    .page_write_time = 4000,              // Typical 850, Max 4000
    .sector_erase_time = 1500000,         // Typical 240 000, Max 1 500 000.
    .byte_write_call_time = 43,           // isBusy time (3b) + write_enable (7b) + 4+1 bytes transfered on SPI bus + code exec time.
    .page_write_call_time = 590,          // isBusy time (3b) + write_enable (7b) + 4+256 bytes transfered on SPI bus + code exec time.
    .sector_erase_call_time = 40,         // isBusy time (3b) + write_enable (7b) + 4 bytes transfered on SPI bus + code exec time.
    .is_busy_call_time = 10               // 3 bytes transfered on SPI bus (6.45 us measured from CS down to CS up)
};

#if defined EXT_FLASH_DRIVER_DEBUG_LED
typedef struct
{
    /** GPIO port  */
    uint8_t port;
    /** Relative GPIO pin number */
    uint8_t pin;
} gpio_pin_t;

static const gpio_pin_t m_pin_list[] = BOARD_GPIO_PIN_LIST;
static const uint8_t m_id_map[] = BOARD_LED_ID_LIST;

/** ============================================================================
 * @brief  Initialize the LEDs (if EXT_FLASH_DRIVER_DEBUG_LED is defined)
 * @return None
 * ===========================================================================*/
static void ext_flash_debug_init(void)
{
    /* Enable clock */
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;

    // Configure LED GPIOs to output and switch LEDs off
    for (uint8_t i = 0; i < sizeof(m_id_map) / sizeof(m_id_map[0]); i++)
    {
        const gpio_pin_t * pin_p = &m_pin_list[i];
        hal_gpio_set_mode(pin_p->port, pin_p->pin, GPIO_MODE_OUT_PP);
        hal_gpio_clear(pin_p->port, pin_p->pin);
    }
}

/** ============================================================================
 * @brief  Switch-on one or two LEDs to provide a visual feedback according
 *         to the driver intialization result
 *         (if EXT_FLASH_DRIVER_DEBUG_LED is defined)
 * @param  success
 *         boolean indicating if the driver initialization has been successful
 * @return None
 * ===========================================================================*/
static void ext_flash_debug_result(bool success)
{
    // one LED = OK, two LEDS KO
    uint8_t count = success ? 1 : 2;

    for (uint8_t i = 0; i < sizeof(m_id_map) / sizeof(m_id_map[0]); i++)
    {
        const gpio_pin_t * pin_p = &m_pin_list[i];
        if (count) {
            hal_gpio_set(pin_p->port, pin_p->pin);
            count--;
        }
        else {
            hal_gpio_clear(pin_p->port, pin_p->pin);
        }
    }
}

#else // defined EXT_FLASH_DRIVER_DEBUG_LED

#define ext_flash_debug_init()
#define ext_flash_debug_result(a)

#endif // defined EXT_FLASH_DRIVER_DEBUG_LED

/** ============================================================================
 * @brief  This function retrieves the frequency of the clock
 *         which is needed to compute the clock divider
 *         Refer to GSDK: platform/bootloader/driver/btl_driver_util.c
 * @return clock frequency
 * ===========================================================================*/
static uint32_t util_getClockFreq(void)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
    const uint8_t frequencies[] = {  4,  0,  0,  7,  0,  0, 13, 16, 19, 0,
                                    26, 32, 38, 48, 56, 64, 80 };
#else
    const uint8_t frequencies[] = {  4,  0,  0,  7,  0,  0, 13, 16, 19, 0, 26,
                                    32, 38, 48, 56, 64, 72 };
#endif
    uint32_t clockFreq;
#if defined(_SILICON_LABS_32B_SERIES_2)
    if ((CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK) ==
                                                CMU_SYSCLKCTRL_CLKSEL_HFXO)
    {
#if defined(BSP_CLK_HFXO_FREQ)
        clockFreq = BSP_CLK_HFXO_FREQ;
#else
        clockFreq = 38400000UL;
#endif
    }
    else {
#if defined(_CMU_CLKEN0_MASK)
        CMU->CLKEN0_SET = CMU_CLKEN0_HFRCO0;
#endif
        clockFreq = (HFRCO0->CAL & _HFRCO_CAL_FREQRANGE_MASK) >>
                                                    _HFRCO_CAL_FREQRANGE_SHIFT;
        if (clockFreq > 16)
        {
            clockFreq = 19000000UL;
        }
        else
        {
            clockFreq = frequencies[clockFreq] * 1000000UL;
        }
        if (clockFreq == 4000000UL)
        {
            clockFreq /= (0x1 << ((HFRCO0->CAL & _HFRCO_CAL_CLKDIV_MASK) >>
                                                    _HFRCO_CAL_CLKDIV_SHIFT));
        }
    }
    clockFreq /= (1U + ((CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_HCLKPRESC_MASK)  >>
                                            _CMU_SYSCLKCTRL_HCLKPRESC_SHIFT));
#else
    if ((CMU->HFCLKSTATUS & _CMU_HFCLKSTATUS_SELECTED_MASK) ==
                                            CMU_HFCLKSTATUS_SELECTED_HFXO)
    {
#if defined(BSP_CLK_HFXO_FREQ)
        clockFreq = BSP_CLK_HFXO_FREQ;
#else
        clockFreq = 38400000UL;
#endif
    }
    else
    {
        clockFreq = (CMU->HFRCOCTRL & _CMU_HFRCOCTRL_FREQRANGE_MASK) >>
                                                _CMU_HFRCOCTRL_FREQRANGE_SHIFT;
        if (clockFreq > 16)
        {
            clockFreq = 19000000UL;
        }
        else
        {
            clockFreq = frequencies[clockFreq] * 1000000UL;
        }
        if (clockFreq == 4000000UL)
        {
            clockFreq /= (0x1 <<
                            ((CMU->HFRCOCTRL & _CMU_HFRCOCTRL_CLKDIV_MASK) >>
                            _CMU_HFRCOCTRL_CLKDIV_SHIFT));
        }
    }
    clockFreq /= (1U + ((CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK) >>
                                                    _CMU_HFPRESC_PRESC_SHIFT));
#endif
    return clockFreq;
}

/** ============================================================================
 * @brief  Active wait
 * @param  wait_ms
 *         Number of milliseconds to wait
 * @return None
 * ===========================================================================*/
static void active_wait_ms(uint32_t wait_ms) {
    // Checked with a logic analyzer
    volatile uint32_t counter = wait_ms * LOOP_COUNT_PER_MS;

    while (0 < counter)
    {
        counter--;
    }
}

/** ============================================================================
 * @brief  Enable GPIO and USART clocks
 * @return None
 * ===========================================================================*/
static void enable_clocks(void)
{
    // Refer to the clock trees Figure 8.1. Bus Clocks in doc [2]
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;
    CMU->CLKEN0_SET = CMU_CLKEN0_USART0;
}

/** ============================================================================
 * @brief       Set the address of the current flash command byte buffer
 *              Addressing is done through a 3-Byte Address Mode for NOR flash
 *              with capacity up to 128 Mbit
 *              Refer to section 10 Command description of the doc [1]
 * @param[in]   address
 *              address of the operation to performed in flash
 * @param[out]  command_p
 *              pointer on the byte buffer that stores the 3-Byte address
 * @return None
 * ===========================================================================*/
static inline void set_address(uint8_t * command_p, uint32_t address)
{
    *command_p++ = (address >> 16) & 0xFF;
    *command_p++ = (address >> 8) & 0xFF;
    *command_p   = address & 0xFF;
}

/** ============================================================================
 * @brief       Set the Chip Select (CS) active
 *              CS is active low, refer to doc [1]
 * @return None
 * ===========================================================================*/
static void ext_flash_set_cs_active(void)
{
    hal_gpio_clear(BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PORT,
                   BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PIN);
}

/** ============================================================================
 * @brief       Set the Chip Select (CS) inactive
 *              CS is active low, refer to doc [1]
 * @return None
 * ===========================================================================*/
static void ext_flash_set_cs_inactive(void)
{
    hal_gpio_set(BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PORT,
                 BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PIN);
}

/** ============================================================================
 * @brief       Perform an 8-bit SPI transfer
 *              Refer to doc [2] sections:
 *                - 20.5.7  USART_STATUS - USART Status Register
 *                - 20.5.16 USART_TXDATA - TX Buffer Data Register
 *                - 20.5.10 USART_RXDATA - RX Buffer Data Register
 * @return      Data received from the flash
 * ===========================================================================*/
static uint8_t ext_flash_spi_transfer(uint8_t data)
{
    // Wait while the TX Buffer is full
    while (! (BOARD_BOOTLOADER_SPI->STATUS & USART_STATUS_TXBL));
    // Send the data, refer to section 20.5.16
    // USART_TXDATA - TX Buffer Data Register of doc [2]
    BOARD_BOOTLOADER_SPI->TXDATA = (uint32_t) data;
    // Wait while the TX Complete bit is not set
    while (! (BOARD_BOOTLOADER_SPI->STATUS & USART_STATUS_TXC));

    // Return the data received from the flash
    return (uint8_t) BOARD_BOOTLOADER_SPI->RXDATA;
}

/** ============================================================================
 * @brief       Enable flash write operation by sending a Write Enable (WREN)
 *              instruction to set the Write Enable Latch (WEL) bit
 *              Refer in doc [1] to section 10-1. Write Enable (WREN) and
 *              Status Register : WIP bit and WEL bit page 30 of doc [1]
 * @return      None
 * ===========================================================================*/
static bool ext_flash_write_enable()
{
    uint8_t reply[2];

    // Send Write Enable command
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_WRITE_ENABLE);
    ext_flash_set_cs_inactive();

    // Wait while WIP bit is set
    while (externalFlash_isBusy());

    // Send Read Status command
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_READ_STATUS);
    reply[0] = ext_flash_spi_transfer(DUMMY_BYTE);
    reply[1] = ext_flash_spi_transfer(DUMMY_BYTE);
    ext_flash_set_cs_inactive();

    // Check that the WEL bit is set
    return (reply[1] & MX25_STATUS_WEL) == MX25_STATUS_WEL;
}

/** ============================================================================
 * @brief       Enable the high performance mode
 *              This flash supports two modes:
 *                - an ultra low power mode
 *                - an high performance mode.
 *              The differences in term of performance are listed in the section
 *              15. ERASE AND PROGRAMMING PERFORMANCE of doc [1]
 *              The differences in term of power consumption are listed in
 *              Table 16. DC Characteristics and Table 17. AC Characteristics
 * @return      None
 * ===========================================================================*/
static bool ext_flash_set_high_performance_mode()
{
    // Enable write access
    if (! ext_flash_write_enable())
    {
        return false;
    }

    // Send Write Status command to enable high performance mode
    // Refer to section 10-8. Read Status Register (RDSR) of doc [1] for a
    // description of the Status register and Configuration Register and
    // especially the L/H switch bit page 31
    // and section 10-10. Write Status Register (WRSR) for the command itself
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_WRITE_STATUS);
    // Status register
    ext_flash_spi_transfer(0);
    // Configuration register-1
    ext_flash_spi_transfer(0);
    // Configuration register-2
    ext_flash_spi_transfer(1 << EXTFLASH_HIGH_PERF_MODE_SHIFT);
    ext_flash_set_cs_inactive();

    // Wait while WIP is set
    while (externalFlash_isBusy());

    return true;
}

/** ============================================================================
 * @brief       Reconfigure the USART for SPI transfer in case it has been
 *              modified by another user of the USART peripheral
 * @return      None
 * ===========================================================================*/
static void reconfigure_usart_block_for_spi(void)
{
    // Control Register configuration is explained in the definition of
    // USART_CTRL_REG_SPI_SETTING
    if (BOARD_BOOTLOADER_SPI->CTRL != USART_CTRL_REG_SPI_SETTING)
    {
        externalFlash_init();
    }
}

/** ============================================================================
 * @brief       External flash driver initialization
 *              Refer to doc [2] section 20.3.3 Synchronous Operation
 * @return      Result code, \ref EXTFLASH_RES_OK if successful
 *              See \ref extFlash_res_e for other result codes
 * ===========================================================================*/
extFlash_res_e externalFlash_init(void)
{
    uint8_t man_id, mem_type, mem_density;

    // Enable GPIO and USART clocks for the configuration period
    enable_clocks();

    ext_flash_debug_init();

    // Pin configuration: refer to doc [2] section 24.3.1 Pin Configuration
    // Setup the Chip Select (CS) pin, active low, PUSHPULL mode
    hal_gpio_set_mode(BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PORT,
                      BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_set(BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PORT,
                 BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PIN);

    // Setup the MOSI pin (TX), PUSHPULL mode
    hal_gpio_set_mode(BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PORT,
                      BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_clear(BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PORT,
                   BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PIN);

    // Setup the MISO pin (RX), INPUT mode
    hal_gpio_set_mode(BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PORT,
                      BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PIN,
                      GPIO_MODE_IN_OD_NOPULL);
    hal_gpio_clear(BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PORT,
                   BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PIN);

    // Setup the SCLK pin, PUSHPULL mode:
    //    - serial mode 0 and 1 => SCLK idle low
    //    - serial mode 2 and 3 => SCLK idle high)
    // MX25R8025F supports mode 0 and 3, refer to [1] 8. DEVICE OPERATION
    // Serial mode 0 will be used so set it low
    hal_gpio_set_mode(BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PORT,
                      BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_clear(BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PORT,
                   BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PIN);

    // Enable USART by setting the USART Enable bit in the USART_EN register
    // Refer to doc [2] section 20.5.2 USART_EN - USART Enable
    BOARD_BOOTLOADER_SPI->EN_SET = USART_EN_EN;

    // Make sure commands are disabled first, before resetting other registers
    // Refer to doc [2] section 20.5.6 USART_CMD - Command Register
    BOARD_BOOTLOADER_SPI->CMD = USART_CMD_RXDIS | USART_CMD_TXDIS |
                                USART_CMD_MASTERDIS | USART_CMD_RXBLOCKDIS |
                                USART_CMD_TXTRIDIS | USART_CMD_CLEARTX |
                                USART_CMD_CLEARRX;
    // Refer to [2] 20.5.5 USART_TRIGCTRL - USART Trigger Control Register
    BOARD_BOOTLOADER_SPI->TRIGCTRL = _USART_TRIGCTRL_RESETVALUE;
    // Refer to doc [2] section 20.5.20 USART_IEN - Interrupt Enable Register
    BOARD_BOOTLOADER_SPI->IEN = _USART_IEN_RESETVALUE;
    // Refer to doc [2] section 20.5.19 USART_IF - Interrupt Flag Register
    BOARD_BOOTLOADER_SPI->IF_CLR = _USART_IF_MASK;

    // Set up for SPI serial mode 0
    // Control Register configuration is explained in the definition of
    // USART_CTRL_REG_SPI_SETTING
    BOARD_BOOTLOADER_SPI->CTRL = USART_CTRL_REG_SPI_SETTING;

    // Configure databits, leave stopbits and parity at reset default (not used)
    // Refer to doc [2] section 20.5.4 USART_FRAME - USART Frame Format Register
    BOARD_BOOTLOADER_SPI->FRAME = _USART_FRAME_RESETVALUE | USART_FRAME_DATABITS_EIGHT;

    // Compute the clock divider for the USART
    // Refer to doc 2 section 20.3.3.2 Clock Generation
    uint32_t clkdiv = util_getClockFreq();
    clkdiv = (clkdiv - 1) / (2 * USART_EXTFLASH_FREQUENCY);
    clkdiv = clkdiv << 8;
    clkdiv &= _USART_CLKDIV_DIV_MASK;
    // Refer to doc [2] 20.5.8 USART_CLKDIV - Clock Control Register
    BOARD_BOOTLOADER_SPI->CLKDIV = clkdiv;

    // Enable main interface mode
    // Refer to doc [2] section 20.5.3 USART_CTRL - Control Register
    BOARD_BOOTLOADER_SPI->CMD = USART_CMD_MASTEREN;

    // Set the routes
    // Refer to doc 2 sections:
    //  - 24.6.192 GPIO_USART0_TXROUTE - TX Port/Pin Select
    //  - 24.6.190 GPIO_USART0_RXROUTE - RX Port/Pin Select
    //  - 24.6.191 GPIO_USART0_CLKROUTE - SCLK Port/Pin Select
    BOARD_BOOTLOADER_SPIROUTE.TXROUTE =
            (BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PORT <<
                                            _GPIO_USART_TXROUTE_PORT_SHIFT) |
            (BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PIN <<
                                            _GPIO_USART_TXROUTE_PIN_SHIFT);
    BOARD_BOOTLOADER_SPIROUTE.RXROUTE =
            (BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PORT <<
                                            _GPIO_USART_RXROUTE_PORT_SHIFT) |
            (BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PIN <<
                                            _GPIO_USART_RXROUTE_PIN_SHIFT);
    BOARD_BOOTLOADER_SPIROUTE.CLKROUTE =
            (BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PORT <<
                                            _GPIO_USART_CLKROUTE_PORT_SHIFT) |
            (BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PIN <<
                                            _GPIO_USART_CLKROUTE_PIN_SHIFT);

    // Enable TX, RX and SCLK pins
    // Refer to doc [2] 24.6.186 GPIO_USART0_ROUTEEN - USART0 Pin Enable
    BOARD_BOOTLOADER_SPIROUTE.ROUTEEN = GPIO_USART_ROUTEEN_TXPEN |
                                        GPIO_USART_ROUTEEN_RXPEN |
                                        GPIO_USART_ROUTEEN_CLKPEN;

    // Enable data transmission and reception
    // Refer to doc [2] section 20.5.3 USART_CTRL - Control Register
    BOARD_BOOTLOADER_SPI->CMD = USART_CMD_RXEN | USART_CMD_TXEN;

    // Ensure the device is ready to access after applying power
    // We delay even if shutdown control isn't used to play it safe
    // since we don't know how quickly init may be called after boot
    // Refer to doc [1] section 14. OPERATING CONDITIONS
    active_wait_ms(TIMING_POWERDOWN_MAX_MS);

    // Release the chip from powerdown mode by sending the Read Electronic
    // Signature command
    // Refer to doc [1] section 10-5. Read Electronic Signature (RES)
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_RES);
    ext_flash_set_cs_inactive();

    // Refer to doc [1] section 14. OPERATING CONDITIONS
    active_wait_ms(TIMING_POWERON_MAX_MS);

    // Sending Read Identification command
    // Refer to doc [1] section 10-3. Read Identification (RDID)
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_READ_IDENTIFICATION);
    man_id = ext_flash_spi_transfer(DUMMY_BYTE);
    mem_type = ext_flash_spi_transfer(DUMMY_BYTE);
    mem_density = ext_flash_spi_transfer(DUMMY_BYTE);
    ext_flash_set_cs_inactive();

    bool init_ok = false;
    // Verify that the flash is a known one
    switch (man_id)
    {
        case MX25_MAN_ID:
            if (MX25R8035F_MEM_TYPE == mem_type &&
                MX25R8035F_MEM_DENSITY == mem_density)
            {
                // Flash chip is detected
                // Set it to high performance mode
                if (ext_flash_set_high_performance_mode())
                {
                    init_ok = true;
                }
            }
        default:
            break;
    }

    ext_flash_debug_result(init_ok);
    if (init_ok)
    {
        // Initialization completed successfully
        return EXTFLASH_RES_OK;
    }
    return EXTFLASH_RES_ERROR;
}

/** ============================================================================
 * @brief  Read bytes from external flash
 *         Refer to doc [1] section 10-11. Read Data Bytes (READ)
 * @param  to
 *         Pointer in RAM to store read data.
 * @param  from
 *         Address in flash to read data from.
 * @param  amount
 *         Number of bytes to read.
 * @return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 * ===========================================================================*/
extFlash_res_e externalFlash_startRead(void * to, const void * from,
                                       size_t to_read_nb)
{
    uint8_t read_cmd[4];
    uint8_t * read_buf = (uint8_t *)to;
    uint32_t read_addr = (uint32_t)from;

    enable_clocks();

    reconfigure_usart_block_for_spi();

    // Check if flash is busy
    if (externalFlash_isBusy())
    {
        return EXTFLASH_RES_BUSY;
    }

    // Check parameters
    if ((read_addr + to_read_nb) > m_flash_info.flash_size)
    {
        return EXTFLASH_RES_PARAM;
    }

    read_cmd[0] = MX25_CMD_READ_ARRAY;
    set_address(&read_cmd[1], read_addr);

    // Send Read Data Bytes command
    ext_flash_set_cs_active();
    for (uint8_t i = 0; i < sizeof(read_cmd); i++)
    {
        ext_flash_spi_transfer(read_cmd[i]);
    }

    // Read the data
    while (to_read_nb > 0)
    {
        *read_buf++ = ext_flash_spi_transfer(DUMMY_BYTE);
        to_read_nb--;
    }
    ext_flash_set_cs_inactive();

    return EXTFLASH_RES_OK;
}

/** ============================================================================
 * @brief  Write bytes to flash.
 *         Refer to doc [1] section 10-23. Page Program (PP)
 * @param  to
 *         Address in flash to write data to.
 * @param  from
 *         Pointer in RAM to the data to be written.
 * @param  amount
 *         Number of bytes to write.
 * @return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 * ===========================================================================*/
extFlash_res_e externalFlash_startWrite(void * to, const void * from,
                                        size_t to_write_nb)
{
    uint8_t write_cmd[4];
    uint32_t write_addr = (uint32_t)to;
    uint8_t * source_buf = (uint8_t *)from;

    enable_clocks();

    reconfigure_usart_block_for_spi();

    // Check if flash is busy
    if (externalFlash_isBusy())
    {
        return EXTFLASH_RES_BUSY;
    }

    /* Check that write do not cross page boundary */
    uint32_t next_page = (write_addr &
                          (0xFFFFFFFF - (m_flash_info.write_page_size - 1))) +
                          m_flash_info.write_page_size;

    if((write_addr + to_write_nb) > next_page)
    {
        return EXTFLASH_RES_PARAM;
    }

    if ((write_addr + to_write_nb) > m_flash_info.flash_size)
    {
        return EXTFLASH_RES_PARAM;
    }

    if (to_write_nb > m_flash_info.write_page_size)
    {
        return EXTFLASH_RES_PARAM;
    }

    // Enable Write
    if (! ext_flash_write_enable())
    {
        return EXTFLASH_RES_ERROR;
    }

    // Send Page Program command
    write_cmd[0] = MX25_CMD_PROGRAM_PAGE;
    set_address(&write_cmd[1], write_addr);

    ext_flash_set_cs_active();

    for (uint8_t i = 0; i < sizeof(write_cmd); i++)
    {
        ext_flash_spi_transfer(write_cmd[i]);
    }

    // Send the data
    while (to_write_nb > 0)
    {
        ext_flash_spi_transfer(*source_buf++);
        to_write_nb--;
    }
    ext_flash_set_cs_inactive();

    return EXTFLASH_RES_OK;
}

/** ============================================================================
 * @brief  Checks if flash driver is busy.
 *         Refer to doc [1] section 10-8. Read Status Register (RDSR)
 * @return true: driver is busy, false otherwise.
 * ===========================================================================*/
bool externalFlash_isBusy(void)
{
    uint8_t reply[2];

    enable_clocks();

    reconfigure_usart_block_for_spi();

    // Send Read Status command
    ext_flash_set_cs_active();
    ext_flash_spi_transfer(MX25_CMD_READ_STATUS);
    reply[0] = ext_flash_spi_transfer(DUMMY_BYTE);
    reply[1] = ext_flash_spi_transfer(DUMMY_BYTE);
    ext_flash_set_cs_inactive();

    // Check if the WIP bit is set
    return (reply[1] & MX25_STATUS_WIP) == MX25_STATUS_WIP;
}

/** ============================================================================
 * @brief  Erase a sector of flash.
 *         Refer to doc [1] section 10-20. Block Erase (BE32K)
 * @param  sector_base
 *         pointer to the base address of the sector to be erased. If the flash
 *         driver can’t erase all requested sector, return the base address of
 *         the next sector to be erased.
 * @param  number_of_sector
 *         Pointer to number of sector to erase.
 *         Returns the number of remaining sector to erase.
 * @return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 * ===========================================================================*/
extFlash_res_e externalFlash_startErase(size_t * sector_base,
                                        size_t * number_of_sector)
{
    uint8_t cmd[4];
    uint32_t addr = (uint32_t)*sector_base;

    enable_clocks();

    reconfigure_usart_block_for_spi();

    // Check if flash is busy
    if (externalFlash_isBusy())
    {
        return EXTFLASH_RES_BUSY;
    }

    // Check parameters
    if ((addr % m_flash_info.erase_sector_size) != 0)
    {
        return EXTFLASH_RES_PARAM;
    }

    if ((addr + m_flash_info.erase_sector_size) > m_flash_info.flash_size)
    {
        return EXTFLASH_RES_PARAM;
    }

    // Enable Write
    if (! ext_flash_write_enable())
    {
        return EXTFLASH_RES_ERROR;
    }

    // Send Block Erase command
    cmd[0] = MX25_CMD_BLOCK_ERASE_32K;
    set_address(&cmd[1], addr);

    ext_flash_set_cs_active();

    for (uint8_t i = 0; i < sizeof(cmd); i++)
    {
        ext_flash_spi_transfer(cmd[i]);
    }

    ext_flash_set_cs_inactive();

    // Increment the base address and decrement number of block to erase
    *sector_base += m_flash_info.erase_sector_size;
    *number_of_sector -= 1;

    return EXTFLASH_RES_OK;
}

/** ============================================================================
 * @brief  Fills a structure with info about flash.
 * @param  info
 *         pointer to an \ref flash_info_t structure.
 * @return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 * ===========================================================================*/
extFlash_res_e externalFlash_getInfo(flash_info_t * info)
{
    // Copy flash characteristics
    memcpy(info, &m_flash_info, sizeof(flash_info_t));

    return EXTFLASH_RES_OK;
}
