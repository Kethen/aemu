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

#include <stdio.h>
#include <string.h>
#include <user.h>
#include <status.h>
#include <config.h>
#include <sqlite3.h>

// Function Prototypes
const char * strcpyxml(char * out, const char * in, uint32_t size);

/**
 * Update Status Logfile
 */
void update_status(void)
{
	// Open Logfile
	FILE * log = fopen(SERVER_STATUS_XMLOUT, "w");
	
	// Opened Logfile
	if(log != NULL)
	{
		// Write XML Header
		fprintf(log, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		
		// Write XSL Processor Information
		fprintf(log, "<?xml-stylesheet type=\"text/xsl\" href=\"status.xsl\"?>\n");
		
		// Output Root Tag + User Count
		fprintf(log, "<prometheus usercount=\"%u\">\n", _db_user_count);
		
		// Database Handle
		sqlite3 * db = NULL;
		
		// Open Database
		if(sqlite3_open(SERVER_DATABASE, &db) == SQLITE_OK)
		{
			// Iterate Games
			SceNetAdhocctlGameNode * game = _db_game; for(; game != NULL; game = game->next)
			{
				// Safe Product ID
				char productid[PRODUCT_CODE_LENGTH + 1];
				strncpy(productid, game->game.data, PRODUCT_CODE_LENGTH);
				productid[PRODUCT_CODE_LENGTH] = 0;
				
				// Display Name
				char displayname[128];
				memset(displayname, 0, sizeof(displayname));
				
				// SQL Statement
				const char * sql = "SELECT name FROM productids WHERE id=?;";
				
				// Prepared SQL Statement
				sqlite3_stmt * statement = NULL;
				
				// Prepare SQL Statement
				if(sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &statement, NULL) == SQLITE_OK)
				{
					// Bind SQL Statement Data
					if(sqlite3_bind_text(statement, 1, productid, strlen(productid), SQLITE_STATIC) == SQLITE_OK)
					{
						// Found Matching Row
						if(sqlite3_step(statement) == SQLITE_ROW)
						{
							// Fetch Game Name from Database
							const char * gamename = (const char *)sqlite3_column_text(statement, 0);
							
							// Copy Game Name
							strcpyxml(displayname, gamename, sizeof(displayname));
						}
						
						// Game not in Database
						else
						{
							// Use Product Code as Name
							strcpyxml(displayname, productid, sizeof(displayname));
						}
					}
					
					// Destroy Prepared SQL Statement
					sqlite3_finalize(statement);
				}
				
				// Output Game Tag + Game Name
				fprintf(log, "\t<game name=\"%s\" usercount=\"%u\">\n", displayname, game->playercount);
				
				// Activate User Count
				uint32_t activecount = 0;
				
				// Iterate Game Groups
				SceNetAdhocctlGroupNode * group = game->group; for(; group != NULL; group = group->next)
				{
					// Safe Group Name
					char groupname[ADHOCCTL_GROUPNAME_LEN + 1];
					strncpy(groupname, (const char *)group->group.data, ADHOCCTL_GROUPNAME_LEN);
					groupname[ADHOCCTL_GROUPNAME_LEN] = 0;
					
					// Output Group Tag + Group Name + User Count
					fprintf(log, "\t\t<group name=\"%s\" usercount=\"%u\">\n", strcpyxml(displayname, groupname, sizeof(displayname)), group->playercount);
					
					// Iterate Users
					SceNetAdhocctlUserNode * user = group->player; for(; user != NULL; user = user->group_next)
					{
						// Output User Tag + Username
						char mac_address[20] = {0};
						sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int)user->resolver.mac.data[0], (unsigned int)user->resolver.mac.data[1], (unsigned int)user->resolver.mac.data[2], (unsigned int)user->resolver.mac.data[3], (unsigned int)user->resolver.mac.data[4], (unsigned int)user->resolver.mac.data[5]);
						fprintf(log, "\t\t\t<user mac_address=\"%s\">%s</user>\n", mac_address, strcpyxml(displayname, (const char *)user->resolver.name.data, sizeof(displayname)));
					}
					
					// Output Closing Group Tag
					fprintf(log, "\t\t</group>\n");
					
					// Increase Active Game User Count
					activecount += group->playercount;
				}
				
				// Output Idle Game Group
				if(game->playercount > activecount)
				{
					// Output Group Tag + Group Name + Idle User Count
					fprintf(log, "\t\t<group name=\"Groupless\" usercount=\"%u\" />\n", game->playercount - activecount);
				}
				
				// Output Closing Game Tag
				fprintf(log, "\t</game>\n");
			}
			
			// Close Database
			sqlite3_close(db);
		}
		
		// Output Closing Root Tag
		fprintf(log, "</prometheus>");
		
		// Close Logfile
		fclose(log);
	}
}

/**
 * Escape XML Sequences to avoid malformed XML files.
 * @param out Out Buffer
 * @param in In Buffer
 * @param size Size of Out Buffer
 * @return Reference to Out Buffer
 */
const char * strcpyxml(char * out, const char * in, uint32_t size)
{
	// Valid Arguments
	if(out != NULL && in != NULL && size > 0)
	{
		// Clear Memory
		memset(out, 0, size);
		
		// Written Size Pointer
		uint32_t written = 0;
		
		// Iterate In-Buffer Symbols
		uint32_t i = 0; for(; i < strlen(in); i++)
		{
			// " Symbol
			if(in[i] == '"')
			{
				// Enough Space in Out-Buffer (6B for &quot;)
				if((size - written) > 6)
				{
					// Write Escaped Sequence
					strcpy(out + written, "&quot;");
					
					// Move Pointer
					written += 6;
				}
				
				// Truncate required
				else break;
			}
			
			// < Symbol
			else if(in[i] == '<')
			{
				// Enough Space in Out-Buffer (4B for &lt;)
				if((size - written) > 4)
				{
					// Write Escaped Sequence
					strcpy(out + written, "&lt;");
					
					// Move Pointer
					written += 4;
				}
				
				// Truncate required
				else break;
			}
			
			// > Symbol
			else if(in[i] == '>')
			{
				// Enough Space in Out-Buffer (4B for &gt;)
				if((size - written) > 4)
				{
					// Write Escaped Sequence
					strcpy(out + written, "&gt;");
					
					// Move Pointer
					written += 4;
				}
				
				// Truncate required
				else break;
			}
			
			// & Symbol
			else if(in[i] == '&')
			{
				// Enough Space in Out-Buffer (5B for &amp;)
				if((size - written) > 5)
				{
					// Write Escaped Sequence
					strcpy(out + written, "&amp;");
					
					// Move Pointer
					written += 5;
				}
				
				// Truncate required
				else break;
			}
			
			// Normal Character
			else
			{
				// Enough Space in Out-Buffer (1B)
				if((size - written) > 1)
				{
					// Write Character
					out[written++] = in[i];
				}
			}
		}
		
		// Return Reference
		return out;
	}
	
	// Invalid Arguments
	return NULL;
}

