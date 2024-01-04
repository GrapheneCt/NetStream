#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <libsysmodule.h>
#include <fmod/fmod.hpp>
#include <fmod/fmod_psvita.h>
#include <fmodngpext.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "player_fmod.h"

using namespace paf;

static FMOD::System *s_system;
static FMOD::ChannelGroup *s_chg;

static const char *k_supportedExtensions[] = {
	".it",
	".mod",
	".xm",
	".s3m",
	".oma",
	".aa3",
	".at3",
	".ogg",
	".mp3",
	".opus",
	".flac",
	".wav",
	".at9",
	".webm"
};

FMOD_RESULT FMODPlayer::HybridInterface::Open(const char *name, unsigned int *filesize, void **handle, void *userdata)
{
	int32_t ret = 0;
	common::SharedPtr<File> *file = NULL;

	if (LocalFile::Exists(name))
	{

		common::SharedPtr<LocalFile> *lfile = new common::SharedPtr<LocalFile>;
		*lfile = LocalFile::Open(name, SCE_O_RDONLY, 0, &ret);
		file = reinterpret_cast<common::SharedPtr<File> *>(lfile);
	}
	else
	{
		common::SharedPtr<CurlFile> *rfile = new common::SharedPtr<CurlFile>;
		*rfile = CurlFile::Open(name, SCE_O_RDONLY, 0, &ret);
		file = reinterpret_cast<common::SharedPtr<File> *>(rfile);
	}

	if (ret == SCE_PAF_OK)
	{
		*filesize = file->get()->GetFileSize();
		*handle = file;
		return FMOD_OK;
	}
	delete file;
	*handle = NULL;
	return FMOD_ERR_FILE_NOTFOUND;
}

FMOD_RESULT FMODPlayer::HybridInterface::Close(void *handle, void *userdata)
{
	common::SharedPtr<File> *file = reinterpret_cast<common::SharedPtr<File> *>(handle);
	delete file;
	return FMOD_OK;
}

FMOD_RESULT FMODPlayer::HybridInterface::Read(void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata)
{
	int32_t readres = 0;
	common::SharedPtr<File> *file = reinterpret_cast<common::SharedPtr<File> *>(handle);
	readres = file->get()->Read(buffer, sizebytes);
	if (readres >= 0)
	{
		*bytesread = readres;
		if (readres == sizebytes)
		{
			return FMOD_OK;
		}
		return FMOD_ERR_FILE_EOF;
	}
	return FMOD_ERR_FILE_BAD;
}

FMOD_RESULT FMODPlayer::HybridInterface::Seek(void *handle, unsigned int pos, void *userdata)
{
	off_t seekres = 0;
	common::SharedPtr<File> *file = reinterpret_cast<common::SharedPtr<File> *>(handle);
	seekres = file->get()->Seek(pos, SCE_SEEK_SET);
	if (seekres == pos)
	{
		return FMOD_OK;
	}
	return FMOD_ERR_FILE_BAD;
}

void FMODPlayer::BootJob::Run()
{
	FMOD_RESULT ret = FMOD_OK;
	m_parent->m_snd = NULL;
	m_parent->m_ch = NULL;

	ret = s_system->createSound(m_parent->m_path.c_str(), FMOD_UNIQUE | FMOD_CREATESTREAM, NULL, &m_parent->m_snd);
	if (ret != FMOD_OK)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	FMOD_TAG tg;
	tg.data = NULL;

	if (m_parent->m_snd->getTag("PIC", -1, &tg) != FMOD_OK)
	{
		if (m_parent->m_snd->getTag("APIC", -1, &tg) != FMOD_OK)
		{
			m_parent->m_snd->getTag("METADATA_BLOCK_PICTURE", -1, &tg);
		}
	}

	if (tg.data && tg.datatype == FMOD_TAGDATATYPE_BINARY)
	{
		for (int i = 0; i < tg.datalen; i++)
		{
			if (!sce_paf_memcmp(tg.data, "\xFF\xD8\xFF", 3) || !sce_paf_memcmp(tg.data, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00\x00\x00\x0D\x49\x48\x44\x52", 16))
			{
				break;
			}
			tg.data = (char *)tg.data + 1;
			tg.datalen--;
		}

		graph::Surface::LoadOption opt;
		intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), tg.data, tg.datalen, &opt);

		if (tex.get())
		{
			thread::RMutex::main_thread_mutex.Lock();
			m_parent->m_target->SetTexture(tex);
			graph::PlaneObj *po = static_cast<graph::PlaneObj *>(m_parent->m_target->GetDrawObj(ui::Plane::OBJ_PLANE));
			po->SetScaleMode(graph::PlaneObj::SCALE_ASPECT_SIZE, graph::PlaneObj::SCALE_ASPECT_SIZE);
			thread::RMutex::main_thread_mutex.Unlock();
		}
	}

	ret = s_system->playSound(m_parent->m_snd, s_chg, false, &m_parent->m_ch);
	if (ret != FMOD_OK)
	{
		m_parent->m_snd->release();
		m_parent->m_snd = NULL;
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	m_parent->SetInitState(InitState_InitOk);
}

FMODPlayer::FMODPlayer(ui::Widget *targetPlane, const char *url)
{
	SCE_DBG_LOG_INFO("[FMOD] Open url: %s\n", url);
	m_target = targetPlane;
	if (!sce_paf_strncmp(url, "mp4://", 6))
	{
		m_path = url + 6;
	}
	else
	{
		m_path = url;
	}
	s_system->attachChannelGroupToPort(FMOD_PSVITA_PORT_TYPE_VOICE, 0, s_chg);
	common::MainThreadCallList::Register(UpdateTask, this);
}

FMODPlayer::~FMODPlayer()
{
	common::MainThreadCallList::Unregister(UpdateTask, this);
	Term();
	s_system->detachChannelGroupFromPort(s_chg);
}

void FMODPlayer::InitAsync()
{
	BootJob *job = new BootJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
	SetInitState(InitState_InProgress);
}

void FMODPlayer::Term()
{
	if (m_initState != InitState_NotInit)
	{
		if (m_ch)
		{
			m_ch->stop();
		}

		if (m_snd)
		{
			m_snd->release();
		}
	}

	SetInitState(InitState_NotInit);
}

GenericPlayer::InitState FMODPlayer::GetInitState()
{
	return m_initState;
}

void FMODPlayer::SetInitState(GenericPlayer::InitState state)
{
	m_initState = state;
	SCE_DBG_LOG_INFO("[FMOD] State changed to: %d\n", state);
	event::BroadcastGlobalEvent(g_appPlugin, GenericPlayerChangeState, m_initState);
}

uint32_t FMODPlayer::GetTotalTimeMs()
{
	uint32_t ret = 0;
	m_snd->getLength(&ret, FMOD_TIMEUNIT_MS);
	return ret;
}

uint32_t FMODPlayer::GetCurrentTimeMs()
{
	uint32_t ret = 0;
	m_ch->getPosition(&ret, FMOD_TIMEUNIT_MS);
	return ret;
}

bool FMODPlayer::JumpToTimeMs(uint32_t time)
{
	SCE_DBG_LOG_INFO("[FMOD] JumpToTime | seconds: %u\n", time / 1000);
	uint32_t total = GetTotalTimeMs();
	if (time >= total)
	{
		m_ch->stop();
		return true;
	}
	return (m_ch->setPosition(time, FMOD_TIMEUNIT_MS) == FMOD_OK);
}

GenericPlayer::PlayerState FMODPlayer::GetState()
{
	bool play = false;
	bool pause = false;

	m_ch->isPlaying(&play);
	if (play)
	{
		return PlayerState_Play;
	}

	m_ch->getPaused(&pause);
	if (pause)
	{
		return PlayerState_Pause;
	}

	return PlayerState_Eof;
}

void FMODPlayer::SetPowerSaving(bool enable)
{
	m_powerSaving = enable;
}

void FMODPlayer::LimitFPS(bool enable)
{
	m_limitFps = enable;
}

void FMODPlayer::SwitchPlaybackState()
{
	bool play = false;
	bool pause = false;

	m_ch->getPaused(&pause);
	m_ch->isPlaying(&play);

	if (!play)
	{
		s_system->playSound(m_snd, NULL, false, &m_ch);
		return;
	}

	if (pause)
	{
		m_ch->setPaused(false);
	}
	else
	{
		m_ch->setPaused(true);
	}
}

bool FMODPlayer::IsPaused()
{
	bool pause = false;
	m_ch->getPaused(&pause);
	return pause;
}

void FMODPlayer::UpdateTask(void *pArgBlock)
{
	s_system->update();
}

void FMODPlayer::PreInit()
{
	new Module("app0:module/libfmodstudio.suprx");
	new Module("app0:module/libfmodngpext.suprx");
	FMOD::System_Create(&s_system);
	FMOD_NGPEXT_PARAM param;
	sce_paf_memset(&param, 0, sizeof(FMOD_NGPEXT_PARAM));
	param.at9prio = 1;
	FMOD_NGP_System_Init(reinterpret_cast<FMOD_SYSTEM *>(s_system), &param);
	s_system->setSoftwareFormat(48000, FMOD_SPEAKERMODE_STEREO, 2);
	s_system->setStreamBufferSize(SCE_KERNEL_256KiB, FMOD_TIMEUNIT_RAWBYTES);
	s_system->setFileSystem(HybridInterface::Open, HybridInterface::Close, HybridInterface::Read, HybridInterface::Seek, NULL, NULL, -1);
	s_system->init(1, FMOD_INIT_NORMAL, NULL);
	s_system->createChannelGroup("FMODPlayer::Output", &s_chg);
}

GenericPlayer::SupportType FMODPlayer::IsSupported(const char *path)
{
	char *extStart = sce_paf_strrchr(path, '.');
	if (!extStart)
	{
		return SupportType_MaybeSupported;
	}

	for (int i = 0; i < sizeof(k_supportedExtensions) / sizeof(char*); i++)
	{
		if (!sce_paf_strcasecmp(extStart, k_supportedExtensions[i]))
			return SupportType_Supported;
	}

	return SupportType_NotSupported;
}