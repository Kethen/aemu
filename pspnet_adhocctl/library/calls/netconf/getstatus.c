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
 * Return Netconf Utility Status
 * @return Netconf Status
 */
int proUtilityNetconfGetStatus(void)
{
	#if 0
	if (_netconf_status == UTILITY_NETCONF_STATUS_NONE)
	{
		return SCE_ERROR_UTILITY_WRONG_TYPE;
	}
	#endif

	if (_netconf_param.type != UTILITY_NETCONF_TYPE_CONNECT_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_CREATE_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_JOIN_ADHOC)
	{
		// passthrough
		return sceUtilityNetconfGetStatus();
	}

	// State Transition
	if (_netconf_status == UTILITY_NETCONF_STATUS_INITIALIZE)
	{
		_netconf_status = UTILITY_NETCONF_STATUS_RUNNING;
		return UTILITY_NETCONF_STATUS_INITIALIZE;
	}

	if (_netconf_status == UTILITY_NETCONF_STATUS_SHUTDOWN)
	{
		_netconf_status = UTILITY_NETCONF_STATUS_NONE;
		return UTILITY_NETCONF_STATUS_SHUTDOWN;
	}

	// Return State
	return _netconf_status;
}
