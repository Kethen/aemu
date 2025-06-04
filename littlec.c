#include <pspsdk.h>
#include <pspsysmem.h>

#include "tinyalloc-master/tinyalloc.h"
#include <stddef.h>

// Available Heap Memory (in Bytes)
static SceUID memblockid = -1;

void * malloc(uint32_t size)
{
	if (__builtin_expect(memblockid < 0, 0))
	{
		static const SceSize size = HEAP_SIZE * 1024;
		memblockid = sceKernelAllocPartitionMemory(2, "tinyalloc", PSP_SMEM_Low, size, NULL);
		if (memblockid < 0)
		{
			return NULL;
		}
		void *block = sceKernelGetBlockHeadAddr(memblockid);
		ta_init(block, block + size, 256, 16, 8);
	}

	return ta_alloc(size + sizeof(uint32_t));
}

void free(void * buffer)
{
	if (__builtin_expect(memblockid < 0, 0))
	{
		return;
	}

	ta_free(buffer);
}

void *memcpy(void *dst, const void *src, size_t len)
{
	return __builtin_memcpy(dst, src, len);
}

void *memset(void *dst, int val, size_t len)
{
	return __builtin_memset(dst, val, len);
}

int memcmp(const void *lhs, const void *rhs, size_t len)
{
	return __builtin_memcmp(lhs, rhs, len);
}

int strcmp(const char *lhs, const char *rhs)
{
	return __builtin_strcmp(lhs, rhs);
}

size_t strlen(const char *s)
{
	return __builtin_strlen(s);
}

void *memmove(void *dst, const void *src, size_t len)
{
	return __builtin_memmove(dst, src, len);
}

