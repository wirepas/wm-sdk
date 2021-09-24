/**
* @file       poslib_da.h
* @brief      Header file for poslib_da.c
* @copyright  Wirepas Ltd 2021
*/

#ifndef _POSLIB_DA_H_
#define _POSLIB_DA_H_

#define POSLIB_DA_SRC_EP POS_SOURCE_ENDPOINT
#define POSLIB_DA_DEST_EP POS_DESTINATION_ENDPOINT

/**
 * @brief   Starts the DA module with functionality required by node positioning mode
 * @param   void
 * @return  bool
 *          true: DA started succesfully ; false: error occured  
 */
bool PosLibDa_start(poslib_settings_t * settings);

/**
 * @brief   Stops the DA module
 * @param   void
 * @return  void
 *           
 */
void PosLibDa_stop();

/**
 * @brief   Sends data for DA tags
 * @param   data pointer to data to sent \ref app_lib_data_to_send_t
 *          sent_cb data sent callback
 * @return  sent result code. see \ref app_lib_data_send_res_e 
 */
app_lib_data_send_res_e PosLibDa_sendData(app_lib_data_to_send_t * data,
                                            app_lib_data_data_sent_cb_f sent_cb);
#endif