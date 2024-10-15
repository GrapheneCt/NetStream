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

			enum Type
			{
				Type_Initial,
				Type_NpOnly
			};

			NetcheckJob(Type type) : job::JobItem("NS::NetcheckJob", NULL), m_type(type)
			{

			}

			~NetcheckJob() {}

			void Run();

			void Finish() {}

		private:

			Type m_type;
		};

		int32_t OnNpDialogComplete(void *data);
	}
}


#endif
