#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <libdbg.h>
#include <paf.h>

#include "common.h"
#include "main.h"
#include "menus/menu_player.h"
#include "dialog.h"
#include "utils.h"
#include "yt_utils.h"
#include "beav_player.h"

using namespace paf;

static ui::Widget *s_playerPage = SCE_NULL;

menu::player::Player::Player(const char *id)
{
	rco::Element searchParam;
	Plugin::PageInitParam rwiParam;
	Plugin::TemplateInitParam tmpParam;
	ui::Widget *corePlane = SCE_NULL;

	g_rootPage->PlayEffectReverse(100.0f, effect::EffectType_Fadein1, SCE_NULL);

	// Create player scene
	searchParam.hash = utils::GetHash("page_player");
	s_playerPage = g_appPlugin->PageOpen(&searchParam, &rwiParam);

	searchParam.hash = utils::GetHash("plane_player_core");
	corePlane = s_playerPage->GetChild(&searchParam, 0);

	InvItemVideo *vid;

	invParseVideo(id, &vid);

	BEAVPlayer *beavPlayer = SCE_NULL;
	SceInt32 reqQuality = menu::settings::Settings::GetInstance()->quality;
	if (reqQuality == 0)
		beavPlayer = new BEAVPlayer(corePlane, vid->avcLqUrl);
	else
		beavPlayer = new BEAVPlayer(corePlane, vid->avcHqUrl);

	if (!beavPlayer->GetResult()) {
		delete beavPlayer;
		if (reqQuality == 0)
			beavPlayer = new BEAVPlayer(corePlane, vid->avcHqUrl);
		else
			beavPlayer = new BEAVPlayer(corePlane, vid->avcLqUrl);

		if (!beavPlayer->GetResult()) {
			delete beavPlayer;
			// cannot play this video
		}
	}
}

menu::player::Player::~Player()
{

}