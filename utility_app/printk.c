#include <stdio.h>
#include <stdarg.h>

#include <pspstdio.h>
#include <pspiofilemgr.h>

void printk(const char *fmt, ...){
	char buf[256];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, args);
	sceIoWrite(sceKernelStdout(), buf, len);
}
