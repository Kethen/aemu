#include <pspsdk.h>
#include <pspsysmem.h>
#include <pspthreadman.h>
#include <pspintrman.h>

#include "tinyalloc-master/tinyalloc.h"
#include <stddef.h>

//#define DEBUG 1
#include "atpro/logs.h"

#define USE_TINYALLOC 1
#define TINYALLOC_USE_PARTITION_MEM 1

#define LOG_MALLOC 0
#define DEBUG_MALLOC 0
#define LOG_CSTRING 0
#define LOG_CSTDLIB 0

#if USE_TINYALLOC

#if TINYALLOC_USE_PARTITION_MEM
static SceUID memblockid = -1;
#elif HEAP_SIZE
static uint8_t heap[HEAP_SIZE * 1024];
#endif

static void tinyalloc_allocate_partition_memory()
{
	int interrupts = sceKernelCpuSuspendIntr();
	#if TINYALLOC_USE_PARTITION_MEM && HEAP_SIZE
	static const SceSize size = HEAP_SIZE * 1024;
	memblockid = sceKernelAllocPartitionMemory(2, "tinyalloc heap", PSP_SMEM_Low, size, NULL);

	if (memblockid < 0)
	{
		sceKernelCpuResumeIntrWithSync(interrupts);
		printk("%s: sceKernelAllocPartitionMemory failed with 0x%x\n", __func__, memblockid);
		return;
	}

	void *block = sceKernelGetBlockHeadAddr(memblockid);

	ta_init(block, block + size, 256, 16, 8);
	#elif HEAP_SIZE
	ta_init(heap, heap + sizeof(heap), 256, 16, 8);
	#endif
	sceKernelCpuResumeIntrWithSync(interrupts);

	#if TINYALLOC_USE_PARTITION_MEM && HEAP_SIZE
	printk("%s: allocated %d, id 0x%x, 0x%x\n", __func__, size, memblockid, block);
	#else
	printk("%s: done\n", __func__);
	#endif
}

static void tinyalloc_free_partition_memory()
{
	#if TINYALLOC_USE_PARTITION_MEM
	int interrupts = sceKernelCpuSuspendIntr();
	SceUID old_memblockid = memblockid;
	if (memblockid < 0)
	{
		sceKernelCpuResumeIntrWithSync(interrupts);
		printk("%s: no partition memory was allocated\n", __func__);
		return;
	}
	sceKernelFreePartitionMemory(memblockid);
	memblockid = -1;
	sceKernelCpuResumeIntrWithSync(interrupts);
	#endif

	#if TINYALLOC_USE_PARTITION_MEM
	printk("%s: freed partition memory 0x%x\n", __func__, old_memblockid);
	#else
	printk("%s: nothing to do in global heap mode\n", __func__);
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

void clean_littlec()
{
	#if USE_TINYALLOC
	tinyalloc_free_partition_memory();
	#endif
}

#if DEBUG_MALLOC
static uint32_t allocated_heap = 0;
#endif

void *malloc(uint32_t size)
{
	#if LOG_MALLOC
	printk("%s: allocating %d\n", __func__, size);
	#endif

	#if DEBUG_MALLOC
	size += sizeof(uint32_t);
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
		return NULL;
	}
	#endif

	#if DEBUG_MALLOC
	((uint32_t *)buf)[0] = size;
	allocated_heap += size;
	printk("%s: allocated heap %d\n", __func__, allocated_heap);
	return buf + sizeof(uint32_t);
	#else
	return buf;
	#endif
}

void free(void *buf)
{
	#if LOG_MALLOC
	printk("%s: freeing 0x%x\n", __func__, buf);
	#endif

	#if DEBUG_MALLOC
	if (buf == 0)
	{
		printk("%s: freeing NULL from 0x%x\n", __func__, __builtin_return_address(0));
		return;
	}
	#endif

	#if DEBUG_MALLOC
	buf -= sizeof(uint32_t);
	uint32_t size = ((uint32_t *)buf)[0];
	allocated_heap -= size;
	printk("%s: allocated heap %d\n", __func__, allocated_heap);
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

int atoi(const char *s)
{
	int result = 0;
	size_t offset = 0;
	while(s[offset] != '\0' && s[offset] != '\r' && s[offset] != '\n')
	{
		int new_digit = s[offset] - '0';
		result *= 10;
		result += new_digit;
		offset++;
	}

	#if LOG_CSTDLIB
	printk("%s: %s %d\n", __func__, s, result);
	#endif

	return result;
}
