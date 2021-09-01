/**
* @file         poslib_mbcn.h
* @brief        Header file for the poslib mini-beacon module
* @copyright    Wirepas Ltd. 2021
*/

#ifndef _POSLIB_MBCN_H_
#define _POSLIB_MBCN_H_

#include "poslib.h"
#define PACKED_STRUCT struct __attribute__((__packed__)) 

/**
    @brief Mini-beacon payload struct
*/
typedef PACKED_STRUCT
{
    uint16_t seq;
    uint8_t data[102 - sizeof(uint16_t)];  //FixME: replace 102 with define if exists!
} poslib_mbcn_payload_t;

/**
    @brief  Starts sending periodic mini-beacons
    @param mbcn Pointer to poslib_mbcn_config_t structure with configuration.
    @return true: mini-beacons started,  false: mini-beacons not started
*/
bool PosLibMbcn_start(poslib_mbcn_config_t * );

/**
    @brief  Stops periodic mini-beacons
    @return void
*/
void PosLibMbcn_stop();


/**
    @brief  Decodes the mini-beacon payload
    @param[in] buf Pointer to the received payload.
    @param[in] length length of the received payload buffer
    @param[out] mbcn pointer to a poslib_mbcn_data_t structure where decoded data will be stored
    @return true: decoding sucesfull,  false: decoding error, data is invalid
*/
bool PosLibMbcn_decode(uint8_t * buf, uint8_t length, poslib_mbcn_data_t * mbcn);

#endif /* _POSLIB_MBCN_H_ */
