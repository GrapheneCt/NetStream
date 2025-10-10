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

		class ListViewFactory : public ui::listview::ItemFactory
		{
		public:

			ListViewFactory(menu::First *parent) : m_parent(parent)
			{

			}

			~ListViewFactory()
			{

			}

			ui::ListItem* Create(CreateParam& param)
			{
				return m_parent->CreateListItem(param);
			}

		private:

			menu::First *m_parent;
		};

		First();

		~First() override;

		ui::ListItem* CreateListItem(ui::listview::ItemFactory::CreateParam& param);

		void OnListButton(ui::Widget *self);

		void SetOfflineMode();

		MenuType GetMenuType() override
		{
			return MenuType_First;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count) override
		{
			*count = 0;
			return nullptr;
		}

	private:

		bool m_showHvdb;
		ui::ListView *m_rootList;
	};
}

#endif
