#include <kernel.h>
#include <libsysmodule.h>

extern "C" {

	typedef struct SceSysmoduleOpt {
		int flags;
		int *result;
		int unused[2];
	} SceSysmoduleOpt;

	typedef struct ScePafInit {
		size_t global_heap_size;
		int a2;
		int a3;
		int cdlg_mode;
		int heap_opt_param1;
		int heap_opt_param2;
	} ScePafInit;

	int sceAppMgrGrowMemory3(unsigned int a1, int a2);

	void __cxa_set_dso_handle_main(void *dso)
	{

	}

	int _sceLdTlsRegisterModuleInfo()
	{
		return 0;
	}

	int __at_quick_exit()
	{
		return 0;
	}
}

__attribute__((constructor(101))) void preloadPaf()
{
	int32_t ret = -1, load_res;
	void* pRet = 0;

	ScePafInit init_param;
	SceSysmoduleOpt sysmodule_opt;

	init_param.global_heap_size = SCE_KERNEL_128MiB;
	init_param.a2 = 0x0000EA60;
	init_param.a3 = 0x00040000;
	init_param.cdlg_mode = 0;
	init_param.heap_opt_param1 = 0;
	init_param.heap_opt_param2 = 0;

	sysmodule_opt.flags = 0;
	sysmodule_opt.result = &load_res;

	ret = sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(init_param), &init_param, &sysmodule_opt);

	if (ret < 0) {
		sceClibPrintf("[PAF PRX] Loader: 0x%x\n", ret);
		sceClibPrintf("[PAF PRX] Loader result: 0x%x\n", load_res);
	}
}