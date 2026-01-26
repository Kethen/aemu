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

#ifndef _ADHOC_MATCHING_FUNCTIONS_H_
#define _ADHOC_MATCHING_FUNCTIONS_H_

/**
 * Allocate Buffer from Heap
 * @param size Buffer Size
 * @return Buffer Address or... NULL
 */
void * _malloc(uint32_t size);

/**
 * Free Buffer
 * @param buffer To-be-freed Buffer
 */
#define _free(b) __free(b, __func__)
void __free(void * buffer, const char *caller);

/**
 * Get Free Heap Size
 * @return Free Heap Size in Bytes
 */
uint32_t _getFreeHeapSize(void);

/**
 * Find Free Matching ID
 * @return First unoccupied Matching ID
 */
int _findFreeMatchingID(void);

/**
 * Find Internal Matching Context for Matching ID
 * @param id Matching ID
 * @return Matching Context Pointer or... NULL
 */
SceNetAdhocMatchingContext * _findMatchingContext(int id);

/**
 * Find Peer in Context by MAC
 * @param context Matching Context Pointer
 * @param mac Peer MAC Address
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findPeer(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac);

/**
 * Find Parent Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findParent(SceNetAdhocMatchingContext * context);

/**
 * Find P2P Buddy Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findP2P(SceNetAdhocMatchingContext * context);

/**
 * Find Outgoing Request Target Peer
 * @param context Matching Context Pointer
 * @return Internal Peer Reference or... NULL
 */
SceNetAdhocMatchingMemberInternal * _findOutgoingRequest(SceNetAdhocMatchingContext * context);

/**
 * Count Children Peers (for Parent)
 * @param context Matching Context Pointer
 * @return Number of Children
 */
uint32_t _countChildren(SceNetAdhocMatchingContext * context);

/**
 * Delete Peer from List
 * @param context Matching Context Pointer
 * @param peer Internal Peer Reference
 */
void _deletePeer(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer);

/**
 * Safely Link Thread Message to IO Thread Stack
 * @param context Matching Context Pointer
 * @param message Thread Message Pointer
 */
void _linkIOMessage(SceNetAdhocMatchingContext * context, ThreadMessage * message);

/**
 * Safely Link Thread Message to Event Thread Stack
 * @param context Matching Context Pointer
 * @param message Thread Message Pointer
 */
void _linkEVMessage(SceNetAdhocMatchingContext * context, ThreadMessage * message);

/**
 * Send Generic Thread Message
 * @param context Matching Context Pointer
 * @param stack ADHOC_MATCHING_EVENT_STACK or ADHOC_MATCHING_INPUT_STACK
 * @param mac Target MAC
 * @param opcode Message Opcode
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendGenericMessage(SceNetAdhocMatchingContext * context, int stack, SceNetEtherAddr * mac, int opcode, int optlen, const void * opt);

/**
 * Clear Thread Stack
 * @param context Matching Context Pointer
 * @param stack ADHOC_MATCHING_EVENT_STACK or ADHOC_MATCHING_INPUT_STACK
 */
void _clearStack(SceNetAdhocMatchingContext * context, int stack);

/**
 * Clear Peer List
 * @param context Matching Context Pointer
 */
void _clearPeerList(SceNetAdhocMatchingContext * context);

/**
 * Send Accept Message from P2P -> P2P or Parent -> Children
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendAcceptMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt);

/**
 * Send Join Request from P2P -> P2P or Children -> Parent
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendJoinRequest(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt);

/**
 * Send Cancel Message to Peer (has various effects)
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendCancelMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int optlen, const void * opt);

/**
 * Send Bulk Data to Peer
 * @param context Matching Context Pointer
 * @param peer Target Peer
 * @param datalen Data Length
 * @param data Data
 */
void _sendBulkData(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer, int datalen, const void * data);

/**
 * Abort Bulk Data Transfer (if in progress)
 * @param context Matching Context Pointer
 * @param peer Target Peer
 */
void _abortBulkTransfer(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer);

/**
 * Notify all established Peers about new Kid in the Neighborhood
 * @param context Matching Context Pointer
 * @param peer New Kid
 */
void _sendBirthMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer);

/**
 * Notify all established Peers about abandoned Child
 * @param context Matching Context Pointer
 * @param peer Abandoned Child
 */
void _sendDeathMessage(SceNetAdhocMatchingContext * context, SceNetAdhocMatchingMemberInternal * peer);

/**
 * Return Number of Connected Peers
 * @param context Matching Context Pointer
 * @return Number of Connected Peers
 */
uint32_t _countConnectedPeers(SceNetAdhocMatchingContext * context);

/**
 * Spawn Local Event for Event Thread
 * @param context Matching Context Pointer
 * @param event Event ID
 * @param mac Event Source MAC
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _spawnLocalEvent(SceNetAdhocMatchingContext * context, int event, SceNetEtherAddr * mac, int optlen, void * opt);

int _isMacMatch(const void *lhs, const void *rhs);

#endif
