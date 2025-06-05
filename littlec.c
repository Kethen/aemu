#include <pspsdk.h>
#include <pspsysmem.h>
#include <pspthreadman.h>
#include <pspintrman.h>

#include "tinyalloc-master/tinyalloc.h"
#include <stddef.h>

#include "atpro/logs.h"

#define USE_TINYALLOC 1
#define TINYALLOC_USE_PARTITION_MEM 1
#define LOG_MALLOC 0
#define LOG_CSTRING 0

#if USE_TINYALLOC

#if TINYALLOC_USE_PARTITION_MEM
static SceUID memblockid = -1;
#else
static uint8_t heap[HEAP_SIZE * 1024];
#endif

static void tinyalloc_allocate_partition_memory()
{
	int interrupts = sceKernelCpuSuspendIntr();
	#if TINYALLOC_USE_PARTITION_MEM
	static const SceSize size = HEAP_SIZE * 1024;
	memblockid = sceKernelAllocPartitionMemory(2, "tinyalloc heap", PSP_SMEM_High, size, NULL);

	if (memblockid < 0)
	{
		sceKernelCpuResumeIntrWithSync(interrupts);
		printk("%s: sceKernelAllocPartitionMemory failed with 0x%x\n", __func__, memblockid);
		return;
	}

	void *block = sceKernelGetBlockHeadAddr(memblockid);

	ta_init(block, block + size, 256, 16, 8);
	#else
	ta_init(heap, heap + sizeof(heap), 256, 16, 8);
	#endif
	sceKernelCpuResumeIntrWithSync(interrupts);

	#if TINYALLOC_USE_PARTITION_MEM
	printk("%s: allocated %d, id 0x%x, 0x%x\n", __func__, size, memblockid, block);
	#else
	printk("%s: done\n", __func__);
	#endif
}

static void *malloc_tinyalloc(uint32_t size)
{
	int interrupts = sceKernelCpuSuspendIntr();
	#if TINYALLOC_USE_PARTITION_MEM
	if (__builtin_expect(memblockid < 0, 0))
	{
		sceKernelCpuResumeIntrWithSync(interrupts);
		return NULL;
	}
	#endif

	void *buf = ta_alloc(size);
	sceKernelCpuResumeIntrWithSync(interrupts);

	return buf;
}

static void free_tinyalloc(void *buf)
{
	int interrupts = sceKernelCpuSuspendIntr();

	#if TINYALLOC_USE_PARTITION_MEM
	if (__builtin_expect(memblockid < 0, 0))
	{
		sceKernelCpuResumeIntrWithSync(interrupts);
		return;
	}
	#endif

	ta_free(buf);

	sceKernelCpuResumeIntrWithSync(interrupts);
}

#else

static void *malloc_partition(uint32_t size)
{
	SceUID blockid = sceKernelAllocPartitionMemory(2, "littlec malloc", PSP_SMEM_Low, size + sizeof(SceUID), NULL);
	if (blockid < 0)
	{
		return NULL;
	}
	void *block = sceKernelGetBlockHeadAddr(blockid);
	((SceUID *)block)[0] = blockid;

	printk("%s: allocated %d 0x%x 0x%x", size, blockid, block);

	return block + sizeof(SceUID);
}

static void free_partition(void *buf)
{
	SceUID blockid = ((SceUID *)buf)[-1];
	printk("%s: freeing 0x%x 0x%x", blockid, buf);
	sceKernelFreePartitionMemory(blockid);
	return;
}

#endif

void init_littlec()
{
	#if USE_TINYALLOC
	tinyalloc_allocate_partition_memory();
	#endif
}

void *malloc(uint32_t size)
{
	#if LOG_MALLOC
	printk("%s: allocating %d\n", __func__, size);
	#endif

	#if USE_TINYALLOC
	void *buf = malloc_tinyalloc(size);
	#else
	void *buf = malloc_partition(size);
	#endif

	#if LOG_MALLOC
	if (__builtin_expect(buf != NULL, 1))
	{
		printk("%s: allocated %d, 0x%x\n", __func__, size, buf);
	}
	else
	{
		printk("%s: failed alocating %d\n", __func__, size);
	}
	#endif

	return buf;
}

void free(void * buf)
{
	#if LOG_MALLOC
	printk("%s: freeing 0x%x\n", __func__, buf);
	#endif

	#if USE_TINYALLOC
	free_tinyalloc(buf);
	#else
	free_partition(buf);
	#endif

	#if LOG_MALLOC
	printk("%s: freed 0x%x\n", __func__, buf);
	#endif
}

void *memcpy(void *dst, const void *src, size_t len)
{
	#if LOG_CSTRING
	printk("%s: 0x%x 0x%x %d\n", __func__, dst, src, len);
	#endif

	for (size_t i = 0;i < len; i++)
	{
		((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
	}

	return dst;
}

void *memset(void *dst, int val, size_t len)
{
	#if LOG_CSTRING
	printk("%s: 0x%x 0x%x %d\n", __func__, dst, val, len);
	#endif

	for (size_t i = 0;i < len;i++)
	{
		((uint8_t *)dst)[i] = (uint8_t)val;
	}
	return dst;
}

int memcmp(const void *lhs, const void *rhs, size_t len)
{
	#if LOG_CSTRING
	printk("%s: 0x%x 0x%x %d\n", __func__, lhs, rhs, len);
	#endif

	for (size_t i = 0;i < len;i++)
	{
		if (((uint8_t *)lhs)[i] < ((uint8_t *)rhs)[i])
		{
			return -1;
		}
		else if (((uint8_t *)lhs)[i] > ((uint8_t *)rhs)[i])
		{
			return 1;
		}
	}
	return 0;
}

size_t strlen(const char *s)
{
	#if LOG_CSTRING
	printk("%s: 0x%x\n", __func__, s);
	#endif

	size_t offset = 0;
	while(s[offset] != '\0')
	{
		offset++;
	}

	return offset;
}

int strcmp(const char *lhs, const char *rhs)
{
	#if LOG_CSTRING
	printk("%s: 0x%x 0x%x\n", __func__, lhs, rhs);
	#endif
	int lhs_len = strlen(lhs);
	int rhs_len = strlen(rhs);
	if (lhs_len < rhs_len)
	{
		return -1;
	}
	else if (lhs_len > rhs_len)
	{
		return 1;
	}

	return memcmp(lhs, rhs, lhs_len);
}

void *memmove(void *dst, const void *src, size_t len)
{
	#if LOG_CSTRING
	printk("%s: 0x%x 0x%x %d\n", __func__, dst, src, len);
	#endif

	return memcpy(dst, src, len);
}
