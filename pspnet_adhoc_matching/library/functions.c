/*
 * This file is part of PRO ONLINE.

 * PRO ONLINE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO ONLINE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO ONLINE. If not, see <http://www.gnu.org/licenses/ .
 */

#include "common.h"

#include <pspsysmem.h>
#include "../../tinyalloc-master/tinyalloc.h"
// Available Heap Memory (in Bytes)
SceUID memblockid = -1;

/**
 * Allocate Buffer from Heap
 * @param size Buffer Size
 * @return Buffer Address or... NULL
 */
void * _malloc(uint32_t size)
{
	if (__builtin_expect(memblockid < 0, 0))
	{
		static const SceSize size = HEAP_SIZE * 1024;
		memblockid = sceKernelAllocPartitionMemory(2, "pspnet_adhoc_matching", PSP_SMEM_Low, size, NULL);
		if (memblockid < 0)
		{
			return NULL;
		}
		void *block = sceKernelGetBlockHeadAddr(memblockid);
		ta_init(block, block + size, 256, 16, 8);
	}

	return ta_alloc(size + sizeof(uint32_t));
}

/**
 * Free Buffer
 * @param buffer To-be-freed Buffer
 */
void _free(void * buffer)
{
	if (__builtin_expect(memblockid < 0, 0))
	{
		return;
	}

	ta_free(buffer);
}

#if 0
/**
 * Get Free Heap Size
 * @return Free Heap Size in Bytes
 */
uint32_t _getFreeHeapSize(void)
{
	// Return Variable
	return _available_heap - sizeof(uint32_t);
}
#endif

/**
 * Find Free Matching ID
 * @return First unoccupied Matching ID
 */
int _findFreeMatchingID(void)
{
	// Minimum Matching ID
	int min = 1;
	
	// Maximum Matching ID
	int max = 0;
	
	// Find highest Matching ID
	SceNetAdhocMatchingContext * item = _contexts; for(; item != NULL; item = item->next)
	{
		// New Maximum
		if(max < item->id) max = item->id;
	}
	
	// Find unoccupied ID
	int i = min; for(; i < max; i++)
	{
		// Found unoccupied ID
		if(_findMatchingContext(i) == NULL) return i;
	}
	
	// Append at virtual end
	return max + 1;
}

/**
 * Find Internal Matching Context for Matching ID
 * @param id Matching ID
 * @return Matching Context Pointer or... NULL
 */
SceNetAdhocMatchingContext * _findMatchingContext(int id)
{
	// Iterate Matching Context List
	SceNetAdhocMatchingContext * item = _contexts; for(; item != NULL; item = item->next)
	{
		// Found Matching ID
		if(item->id == id) return item;
	}
	
	// Context not found
	return NULL;
}

/**
 * Find Peer in Context by MAC
 * @param context Matching Context Pointer
 * @param mac Peer MAC Address
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findPeer(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac)
{
	// Iterate Peer List for Matching Target
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Found Peer in List
		if(memcmp(&peer->mac, mac, sizeof(SceNetEtherAddr)) == 0)
		{
			// Return Peer Pointer
			return peer;
		}
	}
	
	// Peer not found
	return NULL;
}

/**
 * Find Parent Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findParent(SceNetAdhocMatchingContext * context)
{
	// Iterate Peer List for Matching Target
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Found Peer in List
		if(peer->state == ADHOC_MATCHING_PEER_PARENT) return peer;
	}
	
	// Peer not found
	return NULL;
}

/**
 * Find P2P Buddy Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findP2P(SceNetAdhocMatchingContext * context)
{
	// Iterate Peer List for Matching Target
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Found Peer in List
		if(peer->state == ADHOC_MATCHING_PEER_P2P) return peer;
	}
	
	// Peer not found
	return NULL;
}

/**
 * Find Outgoing Request Target Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findOutgoingRequest(SceNetAdhocMatchingContext * context)
{
	// Iterate Peer List for Matching Target
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Found Peer in List
		if(peer->state == ADHOC_MATCHING_PEER_OUTGOING_REQUEST) return peer;
	}
	
	// Peer not found
	return NULL;
}

/**
 * Count Children Peers (for Parent)
 * @param context Matching Context Pointer
 * @return Number of Children
 */
uint32_t _countChildren(SceNetAdhocMatchingContext * context)
{
	// Children Counter
	uint32_t count = 0;
	
	// Iterate Peer List for Matching Target
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Increase Children Counter
		if(peer->state == ADHOC_MATCHING_PEER_CHILD) count++;
	}
	
	// Return Children Count
	return count;
}

/**
 * Delete Peer from List
 * @param context Matching Context Pointer
 * @param peer Internal Peer Reference
 */
void _deletePeer(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer)
{
	// Valid Arguments
	if(context != NULL && peer != NULL)
	{
		// Previous Peer Reference
		SceNetAdhocMatchingMemberInternal * previous = NULL;
		
		// Iterate Peer List
		SceNetAdhocMatchingMemberInternal * item = context->peerlist; for(; item != NULL; item = item->next)
		{
			// Found Peer Match
			if(item == peer) break;
			
			// Set Previous Peer
			previous = item;
		}
		
		// Middle Item
		if(previous != NULL) previous->next = peer->next;
		
		// Beginning Item
		else context->peerlist = peer->next;
		
		// Free Peer Memory
		_free(peer);
	}
}

/**
 * Safely Link Thread Message to IO Thread Stack
 * @param context Matching Context Pointer
 * @param message Thread Message Pointer
 */
void _linkIOMessage(SceNetAdhocMatchingContext * context, ThreadMessage * message)
{
	// Wait for Unlock
	while(context->input_stack_lock) sceKernelDelayThread(1000);
	
	// Lock Access
	context->input_stack_lock = 1;
	
	// Link Message
	message->next = context->input_stack;
	context->input_stack = message;
	
	// Unlock Access
	context->input_stack_lock = 0;
}

/**
 * Safely Link Thread Message to Event Thread Stack
 * @param context Matching Context Pointer
 * @param message Thread Message Pointer
 */
void _linkEVMessage(SceNetAdhocMatchingContext * context, ThreadMessage * message)
{
	// Wait for Unlock
	while(context->event_stack_lock) sceKernelDelayThread(1000);
	
	// Lock Access
	context->event_stack_lock = 1;
	
	// Link Message
	message->next = context->event_stack;
	context->event_stack = message;
	
	// Unlock Access
	context->event_stack_lock = 0;
}

/**
 * Send Generic Thread Message
 * @param context Matching Context Pointer
 * @param stack ADHOC_MATCHING_EVENT_STACK or ADHOC_MATCHING_INPUT_STACK
 * @param mac Target MAC
 * @param opcode Message Opcode
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendGenericMessage(SceNetAdhocMatchingContext * context, int stack, SceNetEtherAddr * mac, int opcode, int optlen, const void * opt)
{
	// Calculate Required Memory Size
	uint32_t size = sizeof(ThreadMessage) + optlen;
	
	// Allocate Memory
	uint8_t * memory = (uint8_t *)_malloc(size);
	
	// Allocated Memory
	if(memory != NULL)
	{
		// Clear Memory
		memset(memory, 0, size);
		
		// Cast Header
		ThreadMessage * header = (ThreadMessage *)memory;
		
		// Set Message Opcode
		header->opcode = opcode;
		
		// Set Peer MAC Address
		header->mac = *mac;
		
		// Set Optional Data Length
		header->optlen = optlen;
		
		// Set Optional Data
		memcpy(memory + sizeof(ThreadMessage), opt, optlen);
		
		// Link Thread Message
		if(stack == ADHOC_MATCHING_EVENT_STACK) _linkEVMessage(context, header);
		
		// Link Thread Message to Input Stack
		else _linkIOMessage(context, header);
		
		// Exit Function
		return;
	}
	
	// Out of Memory Emergency Delete
	_deletePeer(context, _findPeer(context, mac));
}

/**
 * Recursive Stack Cleaner
 * @param node Current Thread Message Node
 */
void _clearStackRecursive(ThreadMessage * node)
{
	// Not End of List
	if(node != NULL) _clearStackRecursive(node->next);
	
	// Free Last Existing Node of List (NULL is handled in _free)
	_free(node);
}

/**
 * Clear Thread Stack
 * @param context Matching Context Pointer
 * @param stack ADHOC_MATCHING_EVENT_STACK or ADHOC_MATCHING_INPUT_STACK
 */
void _clearStack(SceNetAdhocMatchingContext * context, int stack)
{
	// Clear Event Stack
	if(stack == ADHOC_MATCHING_EVENT_STACK)
	{
		// Free Memory Recursively
		_clearStackRecursive(context->event_stack);
		
		// Destroy Reference
		context->event_stack = NULL;
	}
	
	// Clear IO Stack
	else
	{
		// Free Memory Recursively
		_clearStackRecursive(context->input_stack);
		
		// Destroy Reference
		context->input_stack = NULL;
	}
}

/**
 * Clear Peer List
 * @param context Matching Context Pointer
 */
void _clearPeerList(SceNetAdhocMatchingContext * context)
{
	// Iterate Peer List
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; while(peer != NULL)
	{
		// Grab Next Pointer
		SceNetAdhocMatchingMemberInternal * next = peer->next;
		
		// Delete Peer
		_deletePeer(context, peer);
		
		// Move Pointer
		peer = next;
	}
}

/**
 * Send Accept Message from P2P -> P2P or Parent -> Children
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendAcceptMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt)
{
	// Send Accept Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_ACCEPT, optlen, opt);
}

/**
 * Send Join Request from P2P -> P2P or Children -> Parent
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendJoinRequest(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt)
{
	// Send Join Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_JOIN, optlen, opt);
}

/**
 * Send Cancel Message to Peer (has various effects)
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendCancelMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt)
{
	// Send Cancel Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_CANCEL, optlen, opt);
}

/**
 * Send Bulk Data to Peer
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param datalen Data Length
 * @param data Data
 */
void _sendBulkData(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int datalen, const void * data)
{
	// Send Bulk Data Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_BULK, datalen, data);
}

/**
 * Abort Bulk Data Transfer (if in progress)
 * @param context Matching Context Pointer
 * @param peer Target Peer
 */
void _abortBulkTransfer(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer)
{
	// Send Bulk Data Abort Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_BULK_ABORT, 0, NULL);
}

/**
 * Notify all established Peers about new Kid in the Neighborhood
 * @param context Matching Context Pointer
 * @param peer New Kid
 */
void _sendBirthMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer)
{
	// Send Birth Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_BIRTH, 0, NULL);
}

/**
 * Notify all established Peers about abandoned Child
 * @param context Matching Context Pointer
 * @param peer Abandoned Child
 */
void _sendDeathMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer)
{
	// Send Death Message
	_sendGenericMessage(context, ADHOC_MATCHING_INPUT_STACK, &peer->mac, ADHOC_MATCHING_PACKET_DEATH, 0, NULL);
}

/**
 * Return Number of Connected Peers
 * @param context Matching Context Pointer
 * @return Number of Connected Peers
 */
uint32_t _countConnectedPeers(SceNetAdhocMatchingContext * context)
{
	// Peer Count
	uint32_t count = 0;
	
	// Parent Mode
	if(context->mode == ADHOC_MATCHING_MODE_PARENT)
	{
		// Number of Children + 1 Parent (Self)
		count = _countChildren(context) + 1;
	}
	
	// Child Mode
	else if(context->mode == ADHOC_MATCHING_MODE_CHILD)
	{
		// Default to 1 Child (Self)
		count = 1;
		
		// Connected to Parent
		if(_findParent(context) != NULL)
		{
			// Add Number of Siblings + 1 Parents
			count += _countChildren(context) + 1;
		}
	}
	
	// P2P Mode
	else
	{
		// Default to 1 P2P Client (Self)
		count = 1;
		
		// Connected to another P2P Client
		if(_findP2P(context) != NULL)
		{
			// Add P2P Brother
			count++;
		}
	}
	
	// Return Peer Count
	return count;
}

/**
 * Spawn Local Event for Event Thread
 * @param context Matching Context Pointer
 * @param event Event ID
 * @param mac Event Source MAC
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _spawnLocalEvent(SceNetAdhocMatchingContext * context, int event, SceNetEtherAddr * mac, int optlen, void * opt)
{
	// Spawn Local Event
	_sendGenericMessage(context, ADHOC_MATCHING_EVENT_STACK, mac, event, optlen, opt);
}

