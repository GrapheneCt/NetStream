#include <kernel.h>

extern "C" {

	SCE_USER_MODULE_LIST("app0:module/libScePafPreload.suprx");

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