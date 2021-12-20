/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef LIBRARIES_INIT_H_
#define LIBRARIES_INIT_H_

/**
 * \brief   Generic Libraries initialization
 * \note    Libraries must have an init function without parameter
 *          And for backward compatibility reason, libraries should
 *          behave correctly in case of multiple call to init
 */
void Libraries_init(void);

#endif /* LIBRARIES_INIT_H_ */
