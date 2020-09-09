#ifndef __DHCP_H__ 
#define __DHCP_H__

#include <stddef.h> /* size_t */

typedef unsigned char ip_t[4];
typedef struct dhcp dhcp_t;

typedef enum status
{
	DHCP_SUCCESS,
    DHCP_NO_FREE_IP_LEFT,
    DHCP_NO_SUCH_IP_EXISTS,
    DHCP_MEMORY_ALLOCATION_FAIL
} status_t;

/* O(1) */
dhcp_t *DHCPCreate(ip_t subnet_id , size_t subnet_mask);

void DHCPDestroy(dhcp_t *dhcp);

status_t DHCPAllocateIP(dhcp_t *dhcp, const ip_t requested , ip_t output);

status_t DHCPFreeIP(dhcp_t *dhcp, const ip_t ip);

size_t DHCPCountFree(const dhcp_t *dhcp);

#endif