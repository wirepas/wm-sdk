/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stddef.h>
#include "bl_hardware.h"

#if defined(EFR32_PLATFORM)
#include "em_cmu.h"
#endif

#if defined(EFR32FG12) || defined(EFR32MG12) || defined(EFR32FG13)
static CMU_HFXOInit_TypeDef m_hfxoInit =
{
    false,               /* Low-noise mode for EFR32 */
    false,               /* Disable auto-start on EM0/1 entry */
    false,               /* Disable auto-select on EM0/1 entry */
    false,               /* Disable auto-start and select on RAC wakeup */
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,
    BOARD_HW_HFXO_CTUNE, /* Steady-state CTUNE for WSTK boards */
    _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,
    0x20,                /* Matching errata fix in CHIP_Init() */
    0x7,                 /* Recommended steady-state osc core bias current */
    0x6,                 /* Recommended peak detection threshold */
    _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,
    0xA,                 /* Recommended peak detection timeout  */
    0x4,                 /* Recommended steady timeout */
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,
    cmuOscMode_Crystal
};

static CMU_LFXOInit_TypeDef m_lfxoInit =
{
    BOARD_HW_LFXO_CTUNE,            /* ctune */
    _CMU_LFXOCTRL_GAIN_DEFAULT,     /* Default gain, 2 */
    _CMU_LFXOCTRL_TIMEOUT_DEFAULT,  /* Default start-up delay, 32 K cycles */
    cmuOscMode_Crystal,             /* Crystal oscillator */
};
#endif

#if defined(EFR32MG21) || defined(EFR32MG22)
static CMU_HFXOInit_TypeDef m_hfxoInit =
{
    cmuHfxoCbLsbTimeout_416us,
    cmuHfxoSteadyStateTimeout_833us,  /* First lock              */
    cmuHfxoSteadyStateTimeout_83us,   /* Subsequent locks        */
    0U,                               /* ctuneXoStartup          */
    0U,                               /* ctuneXiStartup          */
    32U,                              /* coreBiasStartup         */
    32U,                              /* imCoreBiasStartup       */
    cmuHfxoCoreDegen_None,
    cmuHfxoCtuneFixCap_Both,
    BOARD_HW_HFXO_CTUNE,              /* ctuneXoAna              */
    BOARD_HW_HFXO_CTUNE,              /* ctuneXiAna              */
    60U,                              /* coreBiasAna             */
    false,                            /* enXiDcBiasAna           */
    cmuHfxoOscMode_Crystal,
    false,                            /* forceXo2GndAna          */
    false,                            /* forceXi2GndAna          */
    false,                            /* DisOndemand             */
    false,                            /* ForceEn                 */
    false                             /* Lock registers          */
};

static CMU_LFXOInit_TypeDef m_lfxoInit =
{
    BOARD_HW_LFXO_GAIN,               /* gain            */
    BOARD_HW_LFXO_CTUNE,              /* capTune         */
    cmuLfxoStartupDelay_4KCycles,     /* timeout         */
    cmuLfxoOscMode_Crystal,           /* mode            */
    false,                            /* highAmplitudeEn */
    true,                             /* agcEn           */
    false,                            /* failDetEM4WUEn  */
    false,                            /* failDetEn       */
    false,                            /* DisOndemand     */
    false,                            /* ForceEn         */
    false                             /* Lock registers  */
};
#endif

#if defined(EFR32FG23)
static CMU_HFXOInit_TypeDef m_hfxoInit =
{
    cmuHfxoCbLsbTimeout_416us,
    cmuHfxoSteadyStateTimeout_833us,  /* First lock              */
    cmuHfxoSteadyStateTimeout_83us,   /* Subsequent locks        */
    0U,                               /* ctuneXoStartup          */
    0U,                               /* ctuneXiStartup          */
    32U,                              /* coreBiasStartup         */
    32U,                              /* imCoreBiasStartup       */
    cmuHfxoCoreDegen_None,
    cmuHfxoCtuneFixCap_Both,
    BOARD_HW_HFXO_CTUNE,              /* ctuneXoAna              */
    BOARD_HW_HFXO_CTUNE,              /* ctuneXiAna              */
    60U,                              /* coreBiasAna             */
    false,                            /* enXiDcBiasAna           */
    cmuHfxoOscMode_Crystal,
    false,                            /* forceXo2GndAna          */
    false,                            /* forceXi2GndAna          */
    false,                            /* DisOndemand             */
    false,                            /* ForceEn                 */
    false,                            /* Enable deep sleep       */
    false                             /* Lock registers          */
};

static CMU_LFXOInit_TypeDef m_lfxoInit =
{
    BOARD_HW_LFXO_GAIN,               /* gain            */
    BOARD_HW_LFXO_CTUNE,              /* capTune         */
    cmuLfxoStartupDelay_4KCycles,     /* timeout         */
    cmuLfxoOscMode_Crystal,           /* mode            */
    false,                            /* highAmplitudeEn */
    true,                             /* agcEn           */
    false,                            /* failDetEM4WUEn  */
    false,                            /* failDetEn       */
    false,                            /* DisOndemand     */
    false,                            /* ForceEn         */
    false                             /* Lock registers  */
};
#endif

#if defined(NRF52_PLATFORM)

static const hardware_capabilities_t m_hw =
{
    .crystal_32k = BOARD_HW_CRYSTAL_32K,
    .dcdc = BOARD_HW_DCDC,
    .platform.nrf52 = NULL
};
#elif defined(NRF91_PLATFORM)

static const hardware_capabilities_t m_hw =
{
    .crystal_32k = BOARD_HW_CRYSTAL_32K,
    .dcdc = BOARD_HW_DCDC,
    .platform.nrf91 = NULL
};
#elif defined(EFR32_PLATFORM)

static const platform_efr32_t m_platform_efr32 =
{
    .hfxoInit = &m_hfxoInit,
    .lfxoInit = &m_lfxoInit
};

static const hardware_capabilities_t m_hw =
{
    .crystal_32k = BOARD_HW_CRYSTAL_32K,
    .dcdc = BOARD_HW_DCDC,
    .platform.efr32 = &m_platform_efr32
};

#endif // defined(*_PLATFORM)

const hardware_capabilities_t * Hardware_getCapabilities(void)
{
#if defined(EFR32MG21) || defined(EFR32MG22) || defined(EFR32FG23)
    // If DevInfo.ModuleInfo contains valid calibration value for HFXO CTUNE,
    // use it.
    if ((DEVINFO->MODULEINFO & _DEVINFO_MODULEINFO_HFXOCALVAL_MASK) == 0) {
        m_hfxoInit.ctuneXoAna =
            DEVINFO->MODXOCAL & _DEVINFO_MODXOCAL_HFXOCTUNEXIANA_MASK;
        m_hfxoInit.ctuneXiAna =
            DEVINFO->MODXOCAL & _DEVINFO_MODXOCAL_HFXOCTUNEXIANA_MASK;
    }
    // If DevInfo.ModuleInfo contains valid calibration value for LFXO CapTune,
    // use it.
    if ((DEVINFO->MODULEINFO & _DEVINFO_MODULEINFO_LFXOCALVAL_MASK) == 0) {
        m_lfxoInit.capTune =
            DEVINFO->MODXOCAL & _DEVINFO_MODXOCAL_LFXOCAPTUNE_MASK;
    }
#endif

    return &m_hw;
}
