#ifndef _MENU_HVDB_H_
#define _MENU_HVDB_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "tex_pool.h"
#include "hvdb.h"
#include "menu_generic.h"
#include "menus/menu_player_simple.h"

using namespace paf;

namespace menu {
	class HVDB : public GenericMenu
	{
	public:

		class AudioItem;

		class EntryAddJob : public job::JobItem
		{
		public:

			EntryAddJob(HVDB *parent) : job::JobItem("HVDB::EntryAddJob", NULL), m_parent(parent)
			{

			}

			~EntryAddJob() {}

			int32_t Run()
			{
				m_parent->AddEntry();

				return SCE_PAF_OK;
			}

			void Finish(int32_t result) {}

		private:

			HVDB *m_parent;
		};

		class TrackParseJob : public job::JobItem
		{
		public:

			TrackParseJob(HVDB *parent, AudioItem *item) : job::JobItem("HVDB::TrackParseJob", NULL), m_parent(parent), m_audioItem(item)
			{

			}

			~TrackParseJob() {}

			int32_t Run()
			{
				m_parent->ParseTrack(m_audioItem);

				return SCE_PAF_OK;
			}

			void Finish(int32_t result) {}

		private:

			HVDB *m_parent;
			AudioItem *m_audioItem;
		};

		class LogClearJob : public job::JobItem
		{
		public:

			LogClearJob(HVDB *parent) : job::JobItem("HVDB::LogClearJob", NULL), m_parent(parent)
			{

			}

			~LogClearJob() {}

			int32_t Run();

			void Finish(int32_t result) {}

		private:

			HVDB *m_parent;
		};

		class ListViewCb : public ui::listview::ItemFactory
		{
		public:

			ListViewCb(HVDB *parent) : m_parent(parent)
			{

			}

			~ListViewCb()
			{

			}

			ui::ListItem *Create(CreateParam& param)
			{
				return m_parent->CreateListItem(param);
			}

			void Start(StartParam& param)
			{
				param.list_item->Show(common::transition::Type_Popup1);
			}

		private:

			HVDB *m_parent;
		};

		class ListViewTrackCb : public ui::listview::ItemFactory
		{
		public:

			ListViewTrackCb(HVDB *parent) : m_parent(parent)
			{

			}

			~ListViewTrackCb()
			{

			}

			ui::ListItem *Create(CreateParam& param)
			{
				return m_parent->CreateTrackListItem(param);
			}

			void Start(StartParam& param)
			{
				param.list_item->Show(common::transition::Type_Popup1);
			}

		private:

			HVDB *m_parent;
		};

		class AudioItem
		{
		public:

			AudioItem()
			{

			}

			wstring name;
			wstring subtitle;
			string id;
			string cover;
			IDParam texId;
			TexPool *texPool;
		};

		class TrackItem
		{
		public:

			TrackItem()
			{

			}

			wstring name;
			string url;
			AudioItem *audioItem;
		};

		HVDB();

		~HVDB() override;

		void ClearEntryResults();

		void AddEntry();

		void ParseTrack(AudioItem *audioItem);

		ui::ListItem* CreateListItem(ui::listview::ItemFactory::CreateParam& param);

		ui::ListItem* CreateTrackListItem(ui::listview::ItemFactory::CreateParam& param);

		MenuType GetMenuType() override
		{
			return MenuType_Hvdb;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count) override
		{
			*count = 0;
			return k_settingsIdList;
		}

	private:

		void OnBackButton();
		void OnListButton(ui::Widget *wdg);
		void OnDeleteButton(ui::Widget *wdg);
		void OnAddEntryButton();
		void OnSettingsButton();
		void OnOptionMenuEvent(int32_t type, int32_t subtype);
		void OnDialogEvent(int32_t type);
		void OnPlayerEvent(int32_t type);

		static void OnTexPoolAdd(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

		ui::Widget *m_browserRoot;
		ui::BusyIndicator *m_loaderIndicator;
		ui::Text *m_topText;
		ui::TextBox *m_addEntryTextBox;
		ui::ListView *m_list;
		ui::ListView *m_trackList;
		ui::Button *m_backButton;
		menu::PlayerSimple *m_player;
		string m_addEntryId;
		int32_t m_dialogIdx;
		bool m_interrupted;
		TexPool *m_texPool;
		vector<AudioItem> m_entryResults;
		vector<TrackItem> m_trackResults;

		const uint32_t k_settingsIdList[0] = {
		};
	};

}

#endif
