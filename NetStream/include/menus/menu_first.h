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

		static void ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

		class ListViewFactory : public ui::listview::ItemFactory
		{
		public:

			~ListViewFactory()
			{

			}

			ui::ListItem* Create(CreateParam& param);
		};

		First();

		~First();

		MenuType GetMenuType()
		{
			return MenuType_First;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
		{
			*count = 0;
			return NULL;
		}
	};
}

#endif
