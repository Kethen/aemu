#include <pspkernel.h>
#include <pspdebug.h>
#include <string.h>
#include <pspiofilemgr.h>
#include <stdio.h>

#include <pspnet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <pspnet_adhocmatching.h>

PSP_MODULE_INFO("adhoc test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(8192);
PSP_HEAP_SIZE_KB(0);
#define LOG_PATH "ms0:/adhoc_test.log"

#define LOG(...) { \
	int _fd = sceIoOpen(LOG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777); \
	if (_fd > 0){ \
		char _log_buf[2048] = {0}; \
		int _len = sprintf(_log_buf, __VA_ARGS__); \
		sceIoWrite(_fd, _log_buf, _len); \
		sceIoClose(_fd); \
	} \
	pspDebugScreenPrintf(__VA_ARGS__); \
}

static void init_logging(){
	int fd = sceIoOpen(LOG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (fd > 0){
		sceIoClose(fd);
	}
	pspDebugScreenInit();
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

int main(int argc, char *argv[])
{
	init_logging();
	LOG("adhoc test\n");

	const char *module_list[] = {
		"flash0:/kd/ifhandle.prx",
		"flash0:/kd/pspnet.prx",
		"flash0:/kd/pspnet_adhoc.prx",
		"flash0:/kd/pspnet_adhocctl.prx",
		"flash0:/kd/pspnet_adhoc_matching.prx",	
	};

	for (int i = 0;i < ARRAY_SIZE(module_list);i++){
		int uid = sceKernelLoadModule(module_list[i], 0, NULL);
		if (uid < 0){
			LOG("%s: failed loading %s, 0x%x\n", __func__, module_list[i], uid);
			goto exit;
		}
		int start_ret;
		int start_result = sceKernelStartModule(uid, 0, NULL, &start_ret, NULL);
		if (start_result < 0){
			LOG("%s: failed starting %s, 0x%x\n", __func__, module_list[i], start_result);
			goto exit;
		}else{
			LOG("%s: loaded and started %s\n", __func__, module_list[i]);
		}
	}

	int pspnet_init_status = sceNetInit(0x20000, 0x20, 0, 0x20, 0);
	if (pspnet_init_status < 0){
		LOG("%s: sceNetInit failed, 0x%x\n", __func__, pspnet_init_status);
		goto exit;
	}
	LOG("%s: sceNetInit ok\n", __func__);

	int adhoc_init_status = sceNetAdhocInit();
	if (adhoc_init_status < 0){
		LOG("%s: sceNetAdhocInit failed, 0x%x\n", __func__, adhoc_init_status);
		goto exit;
	}
	LOG("%s: sceNetAdhocInit ok\n", __func__);

	struct productStruct prod = {
		.unknown = 0,
		.product = "ABCD12345",
		.unk = {0, 0, 0}
	};

	int adhocctl_init_status = sceNetAdhocctlInit(3072, 0x20, &prod);
	if (adhocctl_init_status < 0){
		LOG("%s: sceNetAdhocctlInit failed, 0x%x\n", __func__, adhocctl_init_status);
		goto exit;
	}
	LOG("%s: sceNetAdhocctlInit ok\n", __func__);

	int matching_init_status = sceNetAdhocMatchingInit(8192);
	if (matching_init_status < 0){
		LOG("%s: sceNetAdhocMatchingInit failed, 0x%x\n", __func__, matching_init_status);
		goto exit;
	}
	LOG("%s: sceNetAdhocMatchingInit ok\n", __func__);

	exit:

	#if 0
	LOG("Exiting in 5 seconds...\n");
	sceKernelDelayThread(5000000);
	sceKernelExitGame();
	#else
	LOG("Test finished\n");
	sceKernelSleepThread();
	#endif
    return 0;
}

