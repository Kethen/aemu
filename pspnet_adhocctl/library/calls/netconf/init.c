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
SceUtilityNetconfParam _netconf_param = {0};
SceUtilityNetconfAdhocParam _netconf_adhoc_param = {0};

/**
 * Connect to Adhoc Network via Kernel Utility Wrapper
 * @param param Netconf Parameter (Groupname and other Information)
 * @return 0 on success or... -1
 */
int proUtilityNetconfInitStart(SceUtilityNetconfParam * param)
{
	if (param == NULL)
	{
		// Panic
		printk("%s: param is NULL, this is very bad\n", __func__);
		return -1;
	}

	_netconf_param = *param;
	if(param->type == UTILITY_NETCONF_TYPE_CONNECT_ADHOC || param->type == UTILITY_NETCONF_TYPE_CREATE_ADHOC || param->type == UTILITY_NETCONF_TYPE_JOIN_ADHOC)
	{
		if (param->adhoc_param != NULL)
		{
			_netconf_adhoc_param = *(param->adhoc_param);
		}
	}else{
		// Passthrough
		printk("%s: passthrough\n", __func__);
		return sceUtilityNetconfInitStart((pspUtilityNetconfData *)param);
	}

	if (_netconf_status != UTILITY_NETCONF_STATUS_NONE)
	{
		printk("%s: bad state %d\n", __func__, _netconf_status);
		return SCE_ERROR_UTILITY_INVALID_STATUS;
	}

	_netconf_status = UTILITY_NETCONF_STATUS_INITIALIZE;

	return 0;
}
