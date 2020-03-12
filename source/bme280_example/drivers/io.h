/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef IO_H_
#define IO_H_

#include "api.h"
#include <stdbool.h>

/**
 * \brief   Initialize IO's to a known state
 *
 */
void Io_init(void);

/**
 * \brief   Select or unselect BME280 with its chip select signal
 * \param   select
 *          True to select and false to unselect
 */
void Io_select_BME280(bool select);


#endif /* IO_H_ */
