#include <kernel.h>
#include <libsysmodule.h>

int sceAppMgrGrowMemory3(unsigned int a1, int a2);

//#define GROW_MEMORY

typedef struct SceSysmoduleOpt {
	int flags;
	int *result;
	int unused[2];
} SceSysmoduleOpt;

typedef struct ScePafInit {
	SceSize global_heap_size;
	int a2;
	int a3;
	int cdlg_mode;
	int heap_opt_param1;
	int heap_opt_param2;
} ScePafInit; // size is 0x18

int __module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

int __module_exit()
{
	return SCE_KERNEL_STOP_SUCCESS;
}

int __module_start(SceSize argc, void *args)
{
	SceInt32 ret = -1, load_res;

	ScePafInit init_param;
	SceSysmoduleOpt sysmodule_opt;

#ifdef GROW_MEMORY

	init_param.global_heap_size = 4 * 1024 * 1024;

	//Grow memory if possible
	ret = sceAppMgrGrowMemory3(41 * 1024 * 1024, 1); // 57 MB
	if (ret < 0) {
		ret = sceAppMgrGrowMemory3(16 * 1024 * 1024, 1); // 32 MB
		if (ret == 0)
			init_param.global_heap_size = 14 * 1024 * 1024;
	}
	else
		init_param.global_heap_size = 25 * 1024 * 1024;
#else
	init_param.global_heap_size = 128 * 1024 * 1024;
#endif
	init_param.a2 = 0x0000EA60;
	init_param.a3 = 0x00040000;
	init_param.cdlg_mode = 0;
	init_param.heap_opt_param1 = 0;
	init_param.heap_opt_param2 = 0;

	sysmodule_opt.flags = 0; // with arg
	sysmodule_opt.result = &load_res;

	ret = sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(init_param), &init_param, &sysmodule_opt);

	if (ret < 0) {
		sceClibPrintf("[PAF PRX] Loader: 0x%x\n", ret);
		sceClibPrintf("[PAF PRX] Loader result: 0x%x\n", load_res);
	}

	return SCE_KERNEL_START_NO_RESIDENT;
}