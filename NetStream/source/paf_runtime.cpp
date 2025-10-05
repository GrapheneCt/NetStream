#include <kernel.h>
#include <paf.h>
#include <libsysmodule.h>

extern "C" {

	typedef struct SceSysmoduleOpt {
		int flags;
		int *result;
		int unused[2];
	} SceSysmoduleOpt;

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
	
	void user_malloc_init(void)
	{

	}

	void user_malloc_finalize(void)
	{

	}

	void *user_malloc(size_t size)
	{
		return sce_paf_malloc(size);
	}

	void user_free(void *ptr)
	{
		sce_paf_free(ptr);
	}

	void *user_calloc(size_t nelem, size_t size)
	{
		return sce_paf_calloc(nelem, size);
	}

	void *user_realloc(void *ptr, size_t size)
	{
		return sce_paf_realloc(ptr, size);
	}

	void *user_memalign(size_t boundary, size_t size)
	{
		return sce_paf_memalign(boundary, size);
	}

	void *user_reallocalign(void *ptr, size_t size, size_t boundary)
	{
		sceClibPrintf("[PAF2LIBC] reallocalign is not supported\n");
		abort();
		return NULL;
	}

	int user_malloc_stats(struct malloc_managed_size *mmsize)
	{
		sceClibPrintf("[PAF2LIBC] malloc_stats is not supported\n");
		abort();
		return 0;
	}

	int user_malloc_stats_fast(struct malloc_managed_size *mmsize)
	{
		sceClibPrintf("[PAF2LIBC] malloc_stats_fast is not supported\n");
		abort();
		return 0;
	}

	size_t user_malloc_usable_size(void *ptr)
	{
		return sce_paf_musable_size(ptr);
	}
}

void *user_new(std::size_t size)
{
	void *ret = sce_paf_malloc(size);
	/*
		SceAvPlayer bugfix: yucca::DashManifestParser::FinishParse() tries to use uninitialized pointers in MediaLookUpMmap member
		This bug is not visible with SceLibc because it always memsets heap with 0's and yucca probably checks: if(ptr){...}
	*/
	/*
	if (size == 112)
	{
		sce_paf_memset(ret, 0, size);
	}
	*/
	return ret;
}

void *user_new(std::size_t size, const std::nothrow_t& x)
{
	return sce_paf_malloc(size);
}

void *user_new_array(std::size_t size)
{
	return sce_paf_malloc(size);
}

void *user_new_array(std::size_t size, const std::nothrow_t& x)
{
	return sce_paf_malloc(size);
}

void user_delete(void *ptr)
{
	sce_paf_free(ptr);
}

void user_delete(void *ptr, const std::nothrow_t& x)
{
	sce_paf_free(ptr);
}

void user_delete_array(void *ptr)
{
	sce_paf_free(ptr);
}

void user_delete_array(void *ptr, const std::nothrow_t& x)
{
	sce_paf_free(ptr);
}

__attribute__((constructor(101))) void preloadPaf()
{
	int32_t ret = -1, load_res;

	ScePafInit init_param;
	SceSysmoduleOpt sysmodule_opt;

	init_param.global_heap_size = SCE_KERNEL_128MiB;
	init_param.cdlg_mode = 0;
	init_param.global_heap_alignment = 0;
	init_param.global_heap_disable_assert_on_alloc_failure = 0;

	sysmodule_opt.flags = 0;
	sysmodule_opt.result = &load_res;

	ret = sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(init_param), &init_param, &sysmodule_opt);

	if (ret < 0) {
		sceClibPrintf("[PAF PRX] Loader: 0x%x\n", ret);
		sceClibPrintf("[PAF PRX] Loader result: 0x%x\n", load_res);
	}
}