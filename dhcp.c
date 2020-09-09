/**************************************/
/* author: Shahar maimon              */
/* date: 03.18.20                     */
/**************************************/

#include <assert.h> /* assert */
#include <stdlib.h> /* malloc */
#include <stdio.h> /* fprintf */
#include <string.h> /* memcpy */
#include <math.h> /*pow */

#include "dhcp.h"

typedef enum 
{
	LEFT, 
	RIGHT, 
	NUM_OF_CHILDREN
}children_t;

typedef struct node
{
	struct node *child[NUM_OF_CHILDREN];
	int is_full;
}trie_node_t;

struct dhcp
{
	trie_node_t root;
	size_t subnet_mask;
	ip_t network_id;
	size_t free_add;
};

static char SetBitOn(char cha, unsigned int bit_index);
static char SetBitOff(char cha, unsigned int bit_index);
static status_t ReserveAdd(dhcp_t *dhcp);
static int GetBit(char cha, unsigned int bit_index);
static int isValidNetwork(dhcp_t *dhcp, const ip_t requested);
static status_t AllocRec(dhcp_t *dhcp, trie_node_t *node, ip_t ip, size_t curr_bit);
static children_t GetWantedSide(const ip_t ip, size_t curr_bit);
static trie_node_t *CreateNode();
static void UpdateIP(ip_t output, size_t curr_bit);
static void checkandMarkFull(trie_node_t *node);
static status_t FreeRec(dhcp_t *dhcp, trie_node_t *node, const ip_t output, size_t curr_bit);
static void DestroyRec(trie_node_t *node);

#define CHAR_BIT 8
#define IP_CHARS (sizeof(ip_t))
#define IP_BITS 32

/******************** DHCP ************************/
dhcp_t *DHCPCreate(ip_t subnet_id , size_t subnet_mask)
{
	dhcp_t *dhcp = NULL;
	size_t i = 0;

	assert(subnet_mask < IP_BITS);

	dhcp = (dhcp_t *)malloc(sizeof(dhcp_t));
	if (NULL == dhcp)
	{
		fprintf(stderr, "allocation failed\n");
		return NULL;
	}

	dhcp->subnet_mask = subnet_mask;

	for(i = 0; i < IP_CHARS; i++)
	{
		dhcp->network_id[i] = subnet_id[i];
	}

	dhcp->root.child[LEFT] = NULL;
	dhcp->root.child[RIGHT] = NULL;
	dhcp->root.is_full = 0;
	dhcp->free_add = pow(2, IP_BITS - subnet_mask);

	if(ReserveAdd(dhcp) == DHCP_MEMORY_ALLOCATION_FAIL)
	{
		printf("DHCP_MEMORY_ALLOCATION_FAIL");
		return NULL;
	}
	return dhcp;
}

void DHCPDestroy(dhcp_t *dhcp)
{
	DestroyRec(&dhcp->root);
}

static void DestroyRec(trie_node_t *node)
{
	if(node->child[LEFT] != NULL)
	{	
		DestroyRec(node->child[LEFT]);
	}
	if(node->child[RIGHT] != NULL)
	{	
		DestroyRec(node->child[RIGHT]);
	}

	free(node);
	node = NULL;
}

/******************** ALLOC ************************/
status_t DHCPAllocateIP(dhcp_t *dhcp, const ip_t requested , ip_t output)
{
	if(requested == NULL)
	{
		memcpy(output, dhcp->network_id, IP_CHARS);
	}
	else
	{
		memcpy(output, requested, IP_CHARS);
	}
	
	if(!isValidNetwork(dhcp, output))
	{
		printf("not valid\n");
		return DHCP_NO_SUCH_IP_EXISTS;
	}

	if(dhcp->free_add == 0)
	{
		return DHCP_NO_FREE_IP_LEFT;
	}

	return AllocRec(dhcp, &dhcp->root, output, dhcp->subnet_mask);
}

static status_t AllocRec(dhcp_t *dhcp, trie_node_t *node, ip_t output, size_t curr_bit)
{
	children_t side = LEFT;
	status_t status = DHCP_SUCCESS;

	if(curr_bit == IP_BITS)
	{
		dhcp->free_add--;
		node->is_full = 1;
		return DHCP_SUCCESS;
	}

	side = GetWantedSide(output, curr_bit);

	if(node->child[side] == NULL)
	{
		node->child[side] = CreateNode();
		if(node->child[side] == NULL)
		{
			return DHCP_MEMORY_ALLOCATION_FAIL;
		}
		status = AllocRec(dhcp, node->child[side], output, curr_bit + 1);
	}
	else if(side == RIGHT && node->child[side]->is_full)
	{
		return DHCP_NO_FREE_IP_LEFT;
	}
	else if(side == LEFT && node->child[side]->is_full)
	{
		UpdateIP(output, curr_bit);
		status = AllocRec(dhcp, node, output, curr_bit);
	}
	else
	{
		status = AllocRec(dhcp, node->child[side], output, curr_bit + 1);
	}

	
	switch (status)
	{
		case DHCP_MEMORY_ALLOCATION_FAIL:
		{
			return DHCP_MEMORY_ALLOCATION_FAIL;
		}
		case DHCP_NO_FREE_IP_LEFT:
		{
			if(side == RIGHT)
			{
				if(curr_bit == dhcp->subnet_mask)
				{
					return DHCP_NO_SUCH_IP_EXISTS;
				}
				return DHCP_NO_FREE_IP_LEFT;
			}
			else
			{
				UpdateIP(output, curr_bit);
				status = AllocRec(dhcp, node, output, curr_bit);
				break;
			}
		}
		case DHCP_NO_SUCH_IP_EXISTS:
		{
			return DHCP_NO_SUCH_IP_EXISTS;
		}
		default:
		{
			checkandMarkFull(node);
			return DHCP_SUCCESS;
		}
	}
}


/******************** FREE ************************/
status_t DHCPFreeIP(dhcp_t *dhcp, const ip_t ip)
{
	assert(dhcp && ip);

	return FreeRec(dhcp, &dhcp->root, ip , dhcp->subnet_mask);
}

static status_t FreeRec(dhcp_t *dhcp, trie_node_t *node, const ip_t output, size_t curr_bit)
{
	children_t side = LEFT;
	status_t status = DHCP_SUCCESS;

	if(curr_bit == IP_BITS)
	{
		free(node);
		node = NULL;
		return DHCP_NO_FREE_IP_LEFT;
	}

	side = GetWantedSide(output, curr_bit);

	if(node->child[side] == NULL)
	{
		return DHCP_NO_SUCH_IP_EXISTS;
	}

	status = FreeRec(dhcp, node->child[side], output, curr_bit + 1);
	
	switch(status)
	{
		case(DHCP_NO_FREE_IP_LEFT):
		{
			if((status == DHCP_NO_FREE_IP_LEFT) && (node->child[!side] == NULL))
			{
				free(node);
				node = NULL;
				return DHCP_NO_FREE_IP_LEFT;
			}
			node->child[side] = NULL;
			break;	
		}
		case(DHCP_NO_SUCH_IP_EXISTS):
		{
			return DHCP_NO_SUCH_IP_EXISTS;
		}

	}
 	dhcp->free_add--;
	node->is_full = 0;
	return DHCP_SUCCESS;
}

size_t DHCPCountFree(const dhcp_t *dhcp)
{
	assert(dhcp);

	return dhcp->free_add;
}

/******************** Utility funcs ************************/
static void UpdateIP(ip_t output, size_t curr_bit)
{
	output[curr_bit/CHAR_BIT] = SetBitOn(output[curr_bit/CHAR_BIT],  CHAR_BIT - (curr_bit % CHAR_BIT) -1);

	for(curr_bit++; curr_bit<IP_BITS; curr_bit++)
	{
		output[curr_bit/CHAR_BIT] = SetBitOff(output[curr_bit/CHAR_BIT], CHAR_BIT - (curr_bit % CHAR_BIT) -1);
	}

}

static int isValidNetwork(dhcp_t *dhcp, const ip_t requested)
{
	unsigned int bit_index = 0;
	size_t i = 0;

	for(i=0; i < dhcp->subnet_mask/CHAR_BIT; i++)
	{
		if(requested[i] != dhcp->network_id[i])
		{
			return 0;
		}
	}

	for(bit_index = CHAR_BIT - (dhcp->subnet_mask % CHAR_BIT ); 
		bit_index < CHAR_BIT; 
		bit_index++)
	{
		if(GetBit(requested[i], bit_index) != GetBit(dhcp->network_id[i], bit_index))
		{
			return 0;
		}
	}

	return 1;
}




static void checkandMarkFull(trie_node_t *node)
{
	if((node->child[LEFT] != NULL) && (node->child[RIGHT] != NULL))
	{
		if(node->child[LEFT]->is_full && node->child[RIGHT]->is_full)
		{
			node->is_full =1;
		}
	}
}


static char SetBitOn(char cha, unsigned int bit_index)
{
	char one = 1;

	return cha | (one<<bit_index);
}




static char SetBitOff(char cha, unsigned int bit_index)
{
	char one = 1;

	return cha & ~(one<<bit_index);
}



static int GetBit(char cha, unsigned int bit_index)
{
	char one = 1;

	return ((cha & (one << bit_index )) == 0) ? 0 : 1;
} 



static children_t GetWantedSide(const ip_t ip, size_t curr_bit)
{
	if(GetBit(ip[curr_bit/CHAR_BIT], CHAR_BIT - (curr_bit % CHAR_BIT) -1) == 0)
	{
		return LEFT;
	}
	else
	{
		return RIGHT;
	}

} 

static status_t ReserveAdd(dhcp_t *dhcp)
{
	int bit_index = 0;
	ip_t server_add, broadcast_add;
	size_t subnet_mask = 0;
	size_t i = 0;
	status_t status = DHCP_SUCCESS;

	subnet_mask = dhcp->subnet_mask;
	
	memcpy(server_add, dhcp->network_id, IP_CHARS);
	memcpy(broadcast_add, dhcp->network_id, IP_CHARS);

	for(i = subnet_mask/CHAR_BIT , bit_index = CHAR_BIT - (subnet_mask % CHAR_BIT) -1;
	 i < IP_CHARS;
	 bit_index--)
	{
		dhcp->network_id[i] = SetBitOff(dhcp->network_id[i], bit_index);
		broadcast_add[i] = SetBitOn(broadcast_add[i], bit_index);
		server_add[i]  = SetBitOn(server_add[i], bit_index);

		if(bit_index == 0)
		{
			bit_index = CHAR_BIT;
			i++;
		}
	}

	server_add[i-1] = SetBitOff(server_add[IP_CHARS-1], 0);

	status = DHCPAllocateIP(dhcp, dhcp->network_id, dhcp->network_id);
	if (DHCP_MEMORY_ALLOCATION_FAIL == status)
	{
		fprintf(stderr, "allocation failed\n");
		DHCPDestroy(dhcp);
		return status;
	}
	status = DHCPAllocateIP(dhcp, broadcast_add, broadcast_add);
	if (DHCP_MEMORY_ALLOCATION_FAIL == status)
	{
		fprintf(stderr, "allocation failed\n");
		DHCPDestroy(dhcp);
		return status;
	}
	status = DHCPAllocateIP(dhcp, server_add, server_add);
	if (DHCP_MEMORY_ALLOCATION_FAIL == status)
	{
		fprintf(stderr, "allocation failed\n");
		DHCPDestroy(dhcp);
		return status;
	}

	return status;
}




static trie_node_t *CreateNode()
{
	trie_node_t *new_node = NULL;

	new_node = (trie_node_t *)malloc(sizeof(trie_node_t));
	if (NULL == new_node)
	{
		fprintf(stderr, "allocation failed\n");
		return NULL;
	}

	new_node ->child[LEFT] = NULL;
	new_node ->child[RIGHT] = NULL;
	new_node->is_full = 0;

	return new_node;
}
