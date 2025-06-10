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

// Netconf Status
int _netconf_status = UTILITY_NETCONF_STATUS_NONE;

/**
 * Connect to Adhoc Network via Kernel Utility Wrapper
 * @param param Netconf Parameter (Groupname and other Information)
 * @return 0 on success or... -1
 */
int proUtilityNetconfInitStart(SceUtilityNetconfParam * param)
{
	// Valid Utility State
	if(_netconf_status == UTILITY_NETCONF_STATUS_NONE)
	{
		// Valid Parameter
		if(param != NULL)
		{
			// Valid Adhoc Types
			if(param->type == UTILITY_NETCONF_TYPE_CONNECT_ADHOC || param->type == UTILITY_NETCONF_TYPE_CREATE_ADHOC || param->type == UTILITY_NETCONF_TYPE_JOIN_ADHOC)
			{
				// Valid Adhoc Parameter
				if(param->adhoc_param != NULL)
				{
					// Disconnect from Network (if any)
					// On second thought not such a good idea... produces invalid events...
					// proNetAdhocctlDisconnect();

					int join_status = 0;
					// Join network
					if (param->type == UTILITY_NETCONF_TYPE_JOIN_ADHOC)
					{
						join_status = search_and_join(&param->adhoc_param->group_name, 10000000);
					}
					else
					{
						join_status = proNetAdhocctlCreate(&param->adhoc_param->group_name);
					}

					if(join_status == 0)
					{
						// Set Library Status
						// _netconf_status = UTILITY_NETCONF_STATUS_INITIALIZE;

						printk("%s: joined network %s\n", __func__, &param->adhoc_param->group_name);
						// Return Success
						return 0;
					}
					printk("%s: failed joining network %s\n", __func__, &param->adhoc_param->group_name);
				}
			}
		}
	}
	
	// Generic Error
	return -1;
}
