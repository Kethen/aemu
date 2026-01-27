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

#ifndef _LOGS_H_
#define _LOGS_H_

// Check CPU Interrupt State
int is_cpu_intr_enable(void);

// Dump Binary Buffer to Logfile
void hexdump(void * addr, int size);

// Logfile API
#ifdef DEBUG
int printk_init(const char* filename);
int printk(char *fmt, ...)__attribute__((format (printf, 1, 2)));
int printk_sync(void);
void printk_lock(void);
void printk_unlock(void);
#else
#define printk_init(...)
#define printk(...)
#define printk_sync()
#define printk_lock()
#define printk_unlock()
#endif

// for debugging release mode only issues
int printk2(char *fmt, ...)__attribute__((format (printf, 1, 2)));

// Unimplemented Stub Log
#define THROW_UNIMPLEMENTED(func) printk("UNIMPLEMENTED: %s\n", func)

#endif
