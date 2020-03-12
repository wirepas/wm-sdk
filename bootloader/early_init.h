/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef EARLY_INIT_H_
#define EARLY_INIT_H_

/**
 * \brief  Function called early in the boot process.
 *         It can be used to configure I/Os just after power-up.
 */
 void early_init(void);

/**
 * \brief  Function called on time before the flash is locked.
 *         It can be used to configure register that are not accessible
 *         otherwise.
 * \note   Global variables can't be used as .bss and .data are not initialized
 *         at this stage.
 */
void first_boot(void);

#endif //EARLY_INIT_H_
