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

#include "../../common.h"

/**
 * Shutdown Netconf Utility
 * @return 0 on success or... -1
 */
int proUtilityNetconfShutdownStart(void)
{
	// Valid Utility State
	if(_netconf_status != UTILITY_NETCONF_STATUS_FINISHED)
	{
		printk("%s: bad state %d\n", __func__, _netconf_status);
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}

	// Set Library Status
	_netconf_status = UTILITY_NETCONF_STATUS_SHUTDOWN;
	
	// Return Success
	return 0;
}
