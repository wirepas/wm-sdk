/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _FEM_DRIVER_H__
#define _FEM_DRIVER_H__

/**
 * \brief   Initialize radio FEM (Front End Module, AFE) driver.
 *          This function must be called from App_init() and only once.
 *
 *          Wirepas customers are responsible for creating the suitable
 *          driver for their (optional) FEM (AFE) HW. This simple reference
 *          driver can be used as a template.
 *
 *          This function has to fill the FEM configuration to the structure
 *          app_lib_radio_fem_config_t and pass it to lib_radio_fem->setup().
 *          Wirepas firmware will then use the defined callback functions to
 *          control the TX power level and FEM state.
 */
void Fem_init();

#endif
