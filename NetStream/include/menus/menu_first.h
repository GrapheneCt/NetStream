#ifndef _MENU_FIRST_H_
#define _MENU_FIRST_H_

#include <kernel.h>
#include <paf.h>

#include "menu_generic.h"

using namespace paf;

namespace menu {

	class First : public GenericMenu
	{
	public:

		static SceVoid ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

		class ListViewCb : public ui::ListView::ItemCallback
		{
		public:

			~ListViewCb()
			{

			}

			ui::ListItem *Create(Param *info);
		};

		First();

		~First();

		MenuType GetMenuType()
		{
			return MenuType_First;
		}

		const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count)
		{
			*count = 0;
			return SCE_NULL;
		}
	};
}

#endif
