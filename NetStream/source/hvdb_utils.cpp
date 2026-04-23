#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "utils.h"
#include "hvdb_utils.h"
#include "hvdb.h"

using namespace paf;
using namespace sce;

void hvdbutils::Init()
{
	hvdbInit(sce_paf_malloc, sce_paf_free, sce_paf_realloc, utils::DoGETRequest, utils::DoPOSTRequest);
}

void hvdbutils::Flush()
{

}

void hvdbutils::Term()
{
	hvdbTerm();
}

int32_t hvdbutils::EnqueueDownload(const char *url, const char *name)
{
	return ytutils::EnqueueDownload(url, name);
}

int32_t hvdbutils::EnqueueDownloadAsync(const char *url, const char *name)
{
	return ytutils::EnqueueDownloadAsync(url, name);
}