#include "music_island.h"
#include <game/client/gameclient.h>

#if defined(CONF_FAMILY_WINDOWS)
#pragma comment(lib, "runtimeobject.lib")
#include <winrt/base.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>
#endif

struct SEdgeHelperProperties
{
	static constexpr float ms_Padding = 3.0f;
	static constexpr float ms_Rounding = 3.0f;

	static constexpr float ms_ItemSpacing = 2.0f;

	static constexpr float ms_CubeSize = 24.0f;
	static constexpr float ms_ArrowsSize = 18.0f;
	static constexpr float ms_WallWidth = 3.0f;
	static constexpr float ms_CircleRadius = 8.0f;
	static constexpr float ms_CircleThickness = 2.0f;

	static constexpr float ms_HeadlineFontSize = 8.0f;

	static ColorRGBA WindowColor() { return ColorRGBA(0.451f, 0.451f, 0.451f, 0.9f); };
	static ColorRGBA WindowColorDark() { return ColorRGBA(0.2f, 0.2f, 0.2f, 0.9f); };
	static ColorRGBA WindowColorMedium() { return ColorRGBA(0.35f, 0.35f, 0.35f, 0.9f); };

	static ColorRGBA ActionActiveButtonColor() { return ColorRGBA(0.53f, 0.78f, 0.53f, 0.8f); };
	static ColorRGBA ActionAltActiveButtonColor() { return ColorRGBA(1.0f, 0.42f, 0.42f, 0.8f); };
	static ColorRGBA BlueSteelButtonColor() { return ColorRGBA(0.2f, 0.4f, 0.65f, 0.8f); };
	static ColorRGBA ActionWhiteButtonColor() { return ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f); };
};

CMusicIsland::CMusicIsland()
{
	OnReset();
}

CMusicIsland::~CMusicIsland()
{
	StopInfoWorker();
}

void CMusicIsland::ResetMusicInfo()
{
	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	m_MusicInfo = {};
}

CMusicIsland::SMusicInfo CMusicIsland::GetMusicInfo() const
{
	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	return m_MusicInfo;
}

void CMusicIsland::OnConsoleInit()
{
	Console()->Register("ri_show_cur_info", "", CFGFLAG_CLIENT, ConShowCurInfo, this, "Print current music info");
}

void CMusicIsland::SetExtended(bool Extended)
{
	m_Extended = Extended;
}

void CMusicIsland::OnReset()
{
	StopInfoWorker();
	m_Extended = false;
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
}

void CMusicIsland::OnShutdown()
{
	StopInfoWorker();
	m_Extended = false;
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
}

void CMusicIsland::OnRender()
{
	if(!IsActive())
		return;

	const int64_t Now = time_get();
	if(!m_InfoWorkerRunning.load() && (m_NextInfoUpdateTime == 0 || Now >= m_NextInfoUpdateTime))
		StartInfoWorker(Now);

	RenderMusicIsland();
}

void CMusicIsland::RenderMusicIsland()
{
	CUIRect Base;

	vec2 ScreenTL, ScreenBR;
	Graphics()->GetScreen(&ScreenTL.x, &ScreenTL.y, &ScreenBR.x, &ScreenBR.y);

	Base.h = 10.0f;
	Base.w = 75.0f;
	Base.x = ScreenTL.x + (ScreenBR.x - ScreenTL.x - Base.w) / 2.0f;
	Base.y = ScreenTL.y + 2.5f;

	if(Base.y + Base.h > ScreenBR.y)
	{
		Base.y -= Base.y + Base.h - ScreenBR.y;
	}
	if(Base.x + Base.w > ScreenBR.x)
	{
		Base.x -= Base.x + Base.w - ScreenBR.x;
	}

	m_Rect = Base;

	Base.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, SEdgeHelperProperties::ms_Rounding);
	Base.Margin(SEdgeHelperProperties::ms_Padding, &Base);
}

void CMusicIsland::StartInfoWorker(int64_t Now)
{
#if defined(CONF_FAMILY_WINDOWS)
	if(m_InfoWorkerRunning.load())
		return;

	if(m_InfoWorker.joinable())
		m_InfoWorker.join();

	m_InfoWorkerStopRequested.store(false);
	m_InfoWorkerRunning.store(true);
	m_NextInfoUpdateTime = Now + time_freq();
	m_InfoWorker = std::thread(&CMusicIsland::InfoWorkerLoop, this);
#endif
}

void CMusicIsland::StopInfoWorker()
{
#if defined(CONF_FAMILY_WINDOWS)
	m_InfoWorkerStopRequested.store(true);
	if(m_InfoWorker.joinable())
		m_InfoWorker.join();

	m_InfoWorkerRunning.store(false);
#endif
}

void CMusicIsland::InfoWorkerLoop()
{
#if defined(CONF_FAMILY_WINDOWS)
	winrt::init_apartment(winrt::apartment_type::multi_threaded);
	UpdateMusicInfo();
	winrt::uninit_apartment();
	m_InfoWorkerRunning.store(false);
#endif
}

void CMusicIsland::UpdateMusicInfo()
{
#if defined(CONF_FAMILY_WINDOWS)
	SMusicInfo NewInfo;

	try
	{
		using namespace winrt::Windows::Media::Control;

		const auto SessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
		const auto Session = SessionManager.GetCurrentSession();
		if(Session)
		{
			const auto PlaybackInfo = Session.GetPlaybackInfo();
			const auto MediaProperties = Session.TryGetMediaPropertiesAsync().get();

			NewInfo.m_Available = true;
			NewInfo.m_Playing = PlaybackInfo && PlaybackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
			NewInfo.m_Title = winrt::to_string(MediaProperties.Title());
			NewInfo.m_Artist = winrt::to_string(MediaProperties.Artist());
		}
	}
	catch(const winrt::hresult_error &)
	{
		NewInfo = {};
	}
	catch(...)
	{
		NewInfo = {};
	}

	if(m_InfoWorkerStopRequested.load())
		return;

	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	m_MusicInfo = std::move(NewInfo);
#endif
}

bool CMusicIsland::IsActive() const
{
#if defined(CONF_FAMILY_WINDOWS)
	return true;
#else
	return false;
#endif
}

void CMusicIsland::ConShowCurInfo(IConsole::IResult *pResult, void *pUserData)
{
	(void)pResult;

	auto *pSelf = static_cast<CMusicIsland *>(pUserData);
	if(pSelf == nullptr)
		return;

	const SMusicInfo Info = pSelf->GetMusicInfo();

	dbg_msg("Music", "Available: %d, Is playing: %d, Title: %s, Artist: %s", Info.m_Available, Info.m_Playing, Info.m_Title.c_str(), Info.m_Artist.c_str());
}
