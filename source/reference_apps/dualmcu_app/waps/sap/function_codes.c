/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "function_codes.h"

static const uint8_t m_dsap_req[] = DSAP_REQUESTS;
static const uint8_t m_msap_req[] = MSAP_REQUESTS;
static const uint8_t m_csap_req[] = CSAP_REQUESTS;
static const uint8_t m_cnf[] = WAPS_CONFIRMATIONS;
static const uint8_t m_ind[] = WAPS_INDICATIONS;
static const uint8_t m_rsp[] = WAPS_RESPONSES;

#define DSAP_REQ_COUNT  (sizeof(m_dsap_req)/sizeof(m_dsap_req[0]))
#define MSAP_REQ_COUNT  (sizeof(m_msap_req)/sizeof(m_msap_req[0]))
#define CSAP_REQ_COUNT  (sizeof(m_csap_req)/sizeof(m_csap_req[0]))
#define CNF_COUNT       (sizeof(m_cnf)/sizeof(m_cnf[0]))
#define IND_COUNT       (sizeof(m_ind)/sizeof(m_ind[0]))
#define RSP_COUNT       (sizeof(m_rsp)/sizeof(m_rsp[0]))

/**
 * \brief   Helper function for finding an entry in an array
 * \param   array
 *          Search array
 * \param   size
 *          Elements in search array
 * \param   value
 *          The value to search
 */
static bool find(const uint8_t * array,
                 const uint8_t size,
                 const uint8_t value)
{
    for (uint32_t i = 0; i < size; i++)
    {
        if (array[i] == value)
        {
            return true;
        }
    }
    return false;
}

bool WapsFunc_isDsapRequest(uint8_t func)
{
    return find(m_dsap_req, DSAP_REQ_COUNT, func);
}

bool WapsFunc_isMsapRequest(uint8_t func)
{
    return find(m_msap_req, MSAP_REQ_COUNT, func);
}

bool WapsFunc_isCsapRequest(uint8_t func)
{
    return find(m_csap_req, CSAP_REQ_COUNT, func);
}

bool WapsFunc_isRequest(const uint8_t func)
{

    return WapsFunc_isDsapRequest(func) ||
           WapsFunc_isMsapRequest(func) ||
           WapsFunc_isCsapRequest(func);
}

bool WapsFunc_isConfirmation(const uint8_t func)
{
    return find(m_cnf, CNF_COUNT, func);
}

bool WapsFunc_isIndication(const uint8_t func)
{
    return find(m_ind, IND_COUNT, func);
}

bool WapsFunc_isResponse(const uint8_t func)
{
    return find(m_rsp, RSP_COUNT, func);
}
