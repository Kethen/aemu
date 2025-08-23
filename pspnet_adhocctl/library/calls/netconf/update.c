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
 * Set Library in Motion
 * @return 0 on success or... -1
 */
int proUtilityNetconfUpdate(int speed)
{
	if (_netconf_param.type != UTILITY_NETCONF_TYPE_CONNECT_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_CREATE_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_JOIN_ADHOC)
	{
		// Passthrough
		printk("%s: passthrough\n", __func__);
		return sceUtilityNetconfUpdate(speed);
	}

	// We should get called once here
	if (_netconf_status != UTILITY_NETCONF_STATUS_INITIALIZE && _netconf_status != UTILITY_NETCONF_STATUS_RUNNING)
	{
		printk("%s: bad state %d\n", __func__, _netconf_status);
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}

	// Transition into running state
	_netconf_status = UTILITY_NETCONF_STATUS_RUNNING;

	if (_netconf_param.type != UTILITY_NETCONF_TYPE_CONNECT_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_CREATE_ADHOC && _netconf_param.type != UTILITY_NETCONF_TYPE_JOIN_ADHOC)
	{
		// Nothing to do here, or rather, there's no real netconf passthrough here
		_netconf_status = UTILITY_NETCONF_STATUS_FINISHED;
		return 0;
	}

	if (_netconf_param.adhoc_param == NULL)
	{
		// Something is wrong, just bail
		_netconf_status = UTILITY_NETCONF_STATUS_FINISHED;
		return 0;
	}

	// Looks like we can block here
	int join_status = 0;
	if (_netconf_param.type == UTILITY_NETCONF_TYPE_JOIN_ADHOC)
	{
		join_status = search_and_join(&_netconf_adhoc_param.group_name, 10000000);
	}
	else
	{
		join_status = proNetAdhocctlCreate(&_netconf_adhoc_param.group_name);

		if (join_status >= 0){
			// wait for 5 seconds or till we have been connected
			uint64_t begin = sceKernelGetSystemTimeWide();
			while(_thread_status != ADHOCCTL_STATE_CONNECTED && sceKernelGetSystemTimeWide() - begin < 5000000){
				sceKernelDelayThread(1000);
			}
		}
	}

	if (join_status == 0)
	{
		printk("%s: joined network %s\n", __func__, &_netconf_adhoc_param.group_name);
	}
	else
	{
		printk("%s: failed joining network %s, 0x%x\n", __func__, &_netconf_adhoc_param.group_name, join_status);
	}

	_netconf_status = UTILITY_NETCONF_STATUS_FINISHED;

	return 0;
}
