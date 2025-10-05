#ifndef _MAIN_H_
#define _MAIN_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

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

			int32_t Run();

			void Finish(int32_t result) {}

		private:

			Type m_type;
		};

		int32_t OnNpDialogComplete(void *data);
	}
}


#endif
