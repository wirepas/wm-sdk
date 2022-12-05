#ifndef _IPV6_CONFIG_H_
#define _IPV6_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

void Ipv6_initConfig(void);

/**
 * \brief   Get offmesh ipv6 address
 * \param   ipv6_offmesh_address
 *          Pointer to store the ipv6 offmesh ipv6 address
 * \return  True if ipv6_offmesh_address was updated, false if
 *          network doesn't have an offmesh ipv6 address set yet
 */
bool Ipv6_config_getOffMeshIpv6Address(uint8_t ipv6_offmesh_address[16]);

#endif