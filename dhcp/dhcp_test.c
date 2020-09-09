#include <stdio.h>

#include "dhcp.h"

#define TEST(real,expected) (real == expected ? printf("\033[1;32m%d, PASS\033[0m\n", expected) : printf("\033[1;31m%d, FAIL\033[0m\n",expected))

#define RED	printf("\033[1;31m")
#define BLUE printf("\033[1;36m")
#define MAGENTA printf("\033[1;35m")
#define YELLOW printf("\033[1;33m")
#define GREEN printf("\033[1;32m")
#define DEFAULT	printf("\033[0m")

int main()
{
	ip_t subnet_id = {198, 10, 255, 32};
	ip_t wanted = {198, 10, 255, 31};
	size_t subnet_mask = 23;
	ip_t output;
	dhcp_t *dhcp = NULL;
	status_t status;

	dhcp = DHCPCreate(subnet_id , subnet_mask);

	status = DHCPAllocateIP(dhcp, NULL , output);
	if(status != DHCP_SUCCESS)
	{
		printf("fail\n");
	}
	
	status = DHCPAllocateIP(dhcp, NULL , output);
	if(status != DHCP_SUCCESS)
	{
		printf("fail\n");
	}

	status = DHCPAllocateIP(dhcp, wanted , output);
	if(status != DHCP_SUCCESS)
	{
		printf("fail\n");
	}

	status = DHCPAllocateIP(dhcp, wanted , output);
	if(status != DHCP_SUCCESS)
	{
		printf("fail to alloc\n");
	}

	status = DHCPFreeIP(dhcp, wanted);
	if(status != DHCP_SUCCESS)
	{
		printf("fail to free\n");
	}

	status = DHCPAllocateIP(dhcp, wanted , output);
	if(status != DHCP_SUCCESS)
	{
		printf("fail\n");
	}

 	DHCPDestroy(dhcp);


	return 0;
}