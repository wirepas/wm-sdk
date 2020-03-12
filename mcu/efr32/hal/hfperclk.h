/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#ifndef MCU_EFR32_HAL_HFPERCLK_H_
#define MCU_EFR32_HAL_HFPERCLK_H_

// High Frequency Peripheral Cloack freq
#define HFPERCLK_FREQ                  (getHFPERCLK())

/**
 * \brief   Get the High Frequency Peripheral Clock freq
 * \return  The HFPERCLK freq.
 *
 */
uint32_t getHFPERCLK(void);

/**
 * \brief   Enable or disable HFPERCLK
 * \param   enabled
 *          True, if HFPERCLK should be enabled
 *          False, if HFPERCLK should be disabled
 */
void enableHFPERCLK(bool enable);

#endif /* MCU_EFR32_HAL_HFPERCLK_H_ */
