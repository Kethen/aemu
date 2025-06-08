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

/**
 * Mersenne Twister Number Generator
 * @param max Maximum Range of Number
 * @return Random unsigned 32bit Number
 */
uint32_t _getRandomNumber(uint32_t max)
{
	// Initialization Switch
	static int randinit = 0;
	
	// Mersenne Twister Context
	static SceKernelUtilsMt19937Context ctx;
	
	// First Time Initialization
	if(randinit == 0)
	{
		// Initialize Mersenne Twister
		#if 0
		sceKernelUtilsMt19937Init(&ctx, time(NULL));
		#else
		// It does not really need to be different every boot...
		sceKernelUtilsMt19937Init(&ctx, 0xBEEFBEEF);
		#endif
		
		// Prevent Duplicate Initialization
		randinit = 1;
	}
	
	// Calculate Number
	uint32_t number = sceKernelUtilsMt19937UInt(&ctx);
	
	// Trim Number
	number %= max + 1;
	
	// Return Number
	return number;
}

int _isZeroMac(const void *addr)
{
	static const uint8_t zero_mac[ETHER_ADDR_LEN] = {0};
	return memcmp(addr, zero_mac, sizeof(zero_mac)) == 0;
}
