#ifndef _MAIN_H_
#define _MAIN_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

extern "C"
{
	SceUID _vshKernelSearchModuleByName(const char *, int *);
}

namespace menu {
	namespace main {
		class NetcheckJob : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~NetcheckJob() {}

			void Run();

			void Finish() {}
		};
	}
}


#endif
