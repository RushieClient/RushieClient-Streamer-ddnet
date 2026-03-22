#include "music_island.h"

#include <engine/font_icons.h>
#include <engine/gfx/image_manipulation.h>
#include <engine/keys.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>

#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

#if defined(CONF_FAMILY_WINDOWS)
#pragma comment(lib, "runtimeobject.lib")
#include <winrt/base.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#elif defined(CONF_PLATFORM_LINUX) && defined(CONF_MUSIC_ISLAND_MPRIS)
#include <gio/gio.h>
#endif

struct SMusicIslandProperties
{
	static constexpr float ms_Padding = 1.0f;
	static constexpr float ms_Rounding = 3.0f;
	static constexpr float ms_BaseHeight = 10.0f;
	static constexpr float ms_ExpandedInfoHeight = 8.5f;
	static constexpr float ms_ControlGap = 1.0f;
	static constexpr float ms_ControlHeight = 8.0f;
	static ColorRGBA WindowColorDark() { return ColorRGBA(0.2f, 0.2f, 0.2f, 0.9f); };
};

static ColorRGBA MusicIslandWindowColor()
{
	return color_cast<ColorRGBA>(ColorHSLA(g_Config.m_RiShowMusicIslandColorBar, true));
}

static vec2 NativeMouseToScreen(IInput *pInput, IGraphics *pGraphics, vec2 ScreenTL, vec2 ScreenBR)
{
	const vec2 NativeMousePos = pInput->NativeMousePos();
	return vec2(
		ScreenTL.x + NativeMousePos.x * (ScreenBR.x - ScreenTL.x) / pGraphics->ScreenWidth(),
		ScreenTL.y + NativeMousePos.y * (ScreenBR.y - ScreenTL.y) / pGraphics->ScreenHeight());
}

float CMusicIsland::GetStableGameTimerWidth(ITextRender *pTextRender, float FontSize, float TimeSeconds, bool ShowCentiseconds)
{
	static float s_LastFontSize = -1.0f;
	static float s_TextWidthM = 0.0f;
	static float s_TextWidthH = 0.0f;
	static float s_TextWidth0D = 0.0f;
	static float s_TextWidth00D = 0.0f;
	static float s_TextWidth000D = 0.0f;
	static float s_TextWidthMwC = 0.0f;
	static float s_TextWidthHwC = 0.0f;
	static float s_TextWidth0DwC = 0.0f;
	static float s_TextWidth00DwC = 0.0f;
	static float s_TextWidth000DwC = 0.0f;

	if(s_LastFontSize != FontSize)
	{
		s_TextWidthM = pTextRender->TextWidth(FontSize, "00:00", -1, -1.0f);
		s_TextWidthH = pTextRender->TextWidth(FontSize, "00:00:00", -1, -1.0f);
		s_TextWidth0D = pTextRender->TextWidth(FontSize, "0d 00:00:00", -1, -1.0f);
		s_TextWidth00D = pTextRender->TextWidth(FontSize, "00d 00:00:00", -1, -1.0f);
		s_TextWidth000D = pTextRender->TextWidth(FontSize, "000d 00:00:00", -1, -1.0f);
		s_TextWidthMwC = pTextRender->TextWidth(FontSize, "00:00.00", -1, -1.0f);
		s_TextWidthHwC = pTextRender->TextWidth(FontSize, "00:00:00.00", -1, -1.0f);
		s_TextWidth0DwC = pTextRender->TextWidth(FontSize, "0d 00:00:00.00", -1, -1.0f);
		s_TextWidth00DwC = pTextRender->TextWidth(FontSize, "00d 00:00:00.00", -1, -1.0f);
		s_TextWidth000DwC = pTextRender->TextWidth(FontSize, "000d 00:00:00.00", -1, -1.0f);
		s_LastFontSize = FontSize;
	}

	if(!ShowCentiseconds)
	{
		return TimeSeconds >= 3600 * 24 * 100 ? s_TextWidth000D :
			TimeSeconds >= 3600 * 24 * 10 ? s_TextWidth00D :
			TimeSeconds >= 3600 * 24 ? s_TextWidth0D :
			TimeSeconds >= 3600 ? s_TextWidthH :
			s_TextWidthM;
	}

	return TimeSeconds >= 3600 * 24 * 100 ? s_TextWidth000DwC :
		TimeSeconds >= 3600 * 24 * 10 ? s_TextWidth00DwC :
		TimeSeconds >= 3600 * 24 ? s_TextWidth0DwC :
		TimeSeconds >= 3600 ? s_TextWidthHwC :
		s_TextWidthMwC;
}

bool CMusicIsland::GetGameTimerRenderInfo(const CNetObj_GameInfo *pGameInfo, IClient *pClient, ITextRender *pTextRender, float FontSize, SGameTimerRenderInfo &RenderInfo)
{
	if(!pGameInfo || (pGameInfo->m_GameStateFlags & GAMESTATEFLAG_SUDDENDEATH))
		return false;

	const int GameTick = pClient->GameTick(g_Config.m_ClDummy);
	const int GameTickSpeed = pClient->GameTickSpeed();
	float TimeSeconds = 0.0f;
	if(pGameInfo->m_TimeLimit && pGameInfo->m_WarmupTimer <= 0)
	{
		TimeSeconds = pGameInfo->m_TimeLimit * 60.0f -
			(float)(GameTick - pGameInfo->m_RoundStartTick) / GameTickSpeed;

		if(pGameInfo->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
			TimeSeconds = 0.0f;
	}
	else if(pGameInfo->m_GameStateFlags & GAMESTATEFLAG_RACETIME)
	{
		TimeSeconds = (float)(GameTick + pGameInfo->m_WarmupTimer) / GameTickSpeed;
	}
	else
	{
		TimeSeconds = (float)(GameTick - pGameInfo->m_RoundStartTick) / GameTickSpeed;
	}

	str_time((int64_t)(TimeSeconds * 100), g_Config.m_RiShowMilliSecondsTimer ? ETimeFormat::DAYS_CENTISECS : ETimeFormat::DAYS, RenderInfo.m_aText, sizeof(RenderInfo.m_aText));
	RenderInfo.m_TextWidth = GetStableGameTimerWidth(pTextRender, FontSize, TimeSeconds, g_Config.m_RiShowMilliSecondsTimer != 0);
	RenderInfo.m_ActualTextWidth = pTextRender->TextWidth(FontSize, RenderInfo.m_aText, -1, -1.0f);
	RenderInfo.m_TextColor = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);

	if(pGameInfo->m_TimeLimit && TimeSeconds <= 60.0f && pGameInfo->m_WarmupTimer <= 0)
	{
		const float Alpha = TimeSeconds <= 10.0f && (2 * time_get() / time_freq()) % 2 ? 0.5f : 1.0f;
		RenderInfo.m_TextColor = ColorRGBA(1.0f, 0.25f, 0.25f, Alpha);
	}

	return true;
}

float CMusicIsland::GetScrollingTextOffset(float Overflow, float Seconds)
{
	if(Overflow <= 0.0f)
		return 0.0f;

	const float PauseSeconds = 0.75f;
	const float ScrollSpeed = 16.0f;
	const float TravelSeconds = Overflow / ScrollSpeed;
	if(TravelSeconds <= 0.0f)
		return 0.0f;

	const float CycleSeconds = PauseSeconds + TravelSeconds + PauseSeconds + TravelSeconds;
	float PhaseSeconds = std::fmod(Seconds, CycleSeconds);

	if(PhaseSeconds < PauseSeconds)
		return 0.0f;
	PhaseSeconds -= PauseSeconds;

	if(PhaseSeconds < TravelSeconds)
		return PhaseSeconds * ScrollSpeed;
	PhaseSeconds -= TravelSeconds;

	if(PhaseSeconds < PauseSeconds)
		return Overflow;
	PhaseSeconds -= PauseSeconds;

	return maximum(0.0f, Overflow - PhaseSeconds * ScrollSpeed);
}

void CMusicIsland::RenderCenteredClippedText(IGraphics *pGraphics, ITextRender *pTextRender, const CUIRect &Rect, const char *pText, float FontSize, const ColorRGBA &Color, float ScrollSeconds)
{
	if(Rect.w <= 0.0f || Rect.h <= 0.0f || pText == nullptr || pText[0] == '\0')
		return;

	const float TextWidth = pTextRender->TextWidth(FontSize, pText, -1, -1.0f);
	float TextX = Rect.x + (Rect.w - TextWidth) / 2.0f;
	if(TextWidth > Rect.w)
		TextX = Rect.x - GetScrollingTextOffset(TextWidth - Rect.w, ScrollSeconds);
	float TextY = Rect.y + (Rect.h - FontSize) / 2.0f;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	pGraphics->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float ScreenWidth = ScreenX1 - ScreenX0;
	const float ScreenHeight = ScreenY1 - ScreenY0;
	const float PixelSizeX = ScreenWidth / pGraphics->ScreenWidth();
	const float PixelSizeY = ScreenHeight / pGraphics->ScreenHeight();

	TextX = round_to_int(TextX / PixelSizeX) * PixelSizeX;
	TextY = round_to_int(TextY / PixelSizeY) * PixelSizeY;

	const int ClipX = (int)std::round((Rect.x - ScreenX0) * pGraphics->ScreenWidth() / ScreenWidth);
	const int ClipY = (int)std::round((Rect.y - ScreenY0) * pGraphics->ScreenHeight() / ScreenHeight);
	const int ClipW = (int)std::round(Rect.w * pGraphics->ScreenWidth() / ScreenWidth);
	const int ClipH = (int)std::round(Rect.h * pGraphics->ScreenHeight() / ScreenHeight);

	pGraphics->ClipEnable(ClipX, ClipY, ClipW, ClipH);
	pTextRender->TextColor(Color);
	pTextRender->Text(TextX, TextY, FontSize, pText, -1.0f);
	pTextRender->TextColor(pTextRender->DefaultTextColor());
	pGraphics->ClipDisable();
}

static float GetVisualizerBarPulse(float Time, int LayerIndex)
{
	const float Phase = 0.91f * (LayerIndex + 1);
	const float Slow = 0.5f + 0.5f * std::sin(Time * (4.8f + LayerIndex * 0.35f) + Phase);
	const float Mid = 0.5f + 0.5f * std::sin(Time * (9.4f + LayerIndex * 0.8f) + Phase * 1.8f + 0.35f);
	const float Fast = 0.5f + 0.5f * std::sin(Time * (15.5f + LayerIndex * 1.15f) + Phase * 2.6f + 1.1f);
	return maximum(0.0f, minimum(1.0f, Slow * 0.5f + Mid * 0.34f + Fast * 0.16f));
}

static bool HasMusicIslandPlatformBackend()
{
#if defined(CONF_FAMILY_WINDOWS)
	return true;
#elif defined(CONF_PLATFORM_LINUX) && defined(CONF_MUSIC_ISLAND_MPRIS)
	return true;
#else
	return false;
#endif
}

#if defined(CONF_PLATFORM_LINUX) && defined(CONF_MUSIC_ISLAND_MPRIS)
constexpr const char *gs_pMprisPlayerInterface = "org.mpris.MediaPlayer2.Player";
constexpr const char *gs_pMprisObjectPath = "/org/mpris/MediaPlayer2";
constexpr const char *gs_pDbusService = "org.freedesktop.DBus";
constexpr const char *gs_pDbusPath = "/org/freedesktop/DBus";
constexpr const char *gs_pDbusInterface = "org.freedesktop.DBus";
constexpr const char *gs_pDbusPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr const char *gs_pMprisBusPrefix = "org.mpris.MediaPlayer2.";

struct SMprisPlayerState
{
	bool m_Playing = false;
	bool m_CanPlay = false;
	bool m_CanPause = false;
	bool m_CanGoPrevious = false;
	bool m_CanGoNext = false;
	std::string m_BusName;
	std::string m_Title;
	std::string m_Artist;
	std::string m_Album;
};

static GVariant *MprisCallSync(GDBusConnection *pConnection, const char *pBusName, const char *pInterface, const char *pMethod, GVariant *pParameters, const GVariantType *pReplyType)
{
	GError *pError = nullptr;
	GVariant *pResult = g_dbus_connection_call_sync(
		pConnection,
		pBusName,
		gs_pMprisObjectPath,
		pInterface,
		pMethod,
		pParameters,
		pReplyType,
		G_DBUS_CALL_FLAGS_NONE,
		1000,
		nullptr,
		&pError);
	if(pError != nullptr)
	{
		g_error_free(pError);
		return nullptr;
	}
	return pResult;
}

static std::vector<std::string> GetMprisBusNames(GDBusConnection *pConnection)
{
	std::vector<std::string> vNames;

	GError *pError = nullptr;
	GVariant *pResult = g_dbus_connection_call_sync(
		pConnection,
		gs_pDbusService,
		gs_pDbusPath,
		gs_pDbusInterface,
		"ListNames",
		nullptr,
		G_VARIANT_TYPE("(as)"),
		G_DBUS_CALL_FLAGS_NONE,
		1000,
		nullptr,
		&pError);
	if(pError != nullptr)
	{
		g_error_free(pError);
		return vNames;
	}
	if(pResult == nullptr)
		return vNames;

	GVariant *pNames = nullptr;
	g_variant_get(pResult, "(@as)", &pNames);
	GVariantIter Iter;
	const char *pName = nullptr;
	g_variant_iter_init(&Iter, pNames);
	while(g_variant_iter_next(&Iter, "&s", &pName))
	{
		if(std::strncmp(pName, gs_pMprisBusPrefix, std::strlen(gs_pMprisBusPrefix)) == 0)
			vNames.emplace_back(pName);
	}

	g_variant_unref(pNames);
	g_variant_unref(pResult);
	return vNames;
}

static void ParseMprisMetadata(GVariant *pMetadata, SMprisPlayerState &State)
{
	GVariantIter Iter;
	const char *pKey = nullptr;
	GVariant *pValue = nullptr;
	g_variant_iter_init(&Iter, pMetadata);
	while(g_variant_iter_next(&Iter, "{&sv}", &pKey, &pValue))
	{
		GVariant *pInnerValue = g_variant_get_variant(pValue);
		if(std::strcmp(pKey, "xesam:title") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_STRING))
		{
			State.m_Title = g_variant_get_string(pInnerValue, nullptr);
		}
		else if(std::strcmp(pKey, "xesam:album") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_STRING))
		{
			State.m_Album = g_variant_get_string(pInnerValue, nullptr);
		}
		else if(std::strcmp(pKey, "xesam:artist") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE("as")))
		{
			gsize NumArtists = 0;
			const gchar **ppArtists = g_variant_get_strv(pInnerValue, &NumArtists);
			if(NumArtists > 0 && ppArtists[0] != nullptr)
				State.m_Artist = ppArtists[0];
			g_free((gpointer)ppArtists);
		}
		g_variant_unref(pInnerValue);
		g_variant_unref(pValue);
	}
}

static bool QueryMprisPlayer(GDBusConnection *pConnection, const char *pBusName, SMprisPlayerState &State)
{
	GVariant *pResult = MprisCallSync(
		pConnection,
		pBusName,
		gs_pDbusPropertiesInterface,
		"GetAll",
		g_variant_new("(s)", gs_pMprisPlayerInterface),
		G_VARIANT_TYPE("(a{sv})"));
	if(pResult == nullptr)
		return false;

	GVariant *pProperties = nullptr;
	g_variant_get(pResult, "(@a{sv})", &pProperties);
	GVariantIter Iter;
	const char *pKey = nullptr;
	GVariant *pValue = nullptr;
	g_variant_iter_init(&Iter, pProperties);
	while(g_variant_iter_next(&Iter, "{&sv}", &pKey, &pValue))
	{
		GVariant *pInnerValue = g_variant_get_variant(pValue);
		if(std::strcmp(pKey, "PlaybackStatus") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_STRING))
		{
			State.m_Playing = std::strcmp(g_variant_get_string(pInnerValue, nullptr), "Playing") == 0;
		}
		else if(std::strcmp(pKey, "CanPlay") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_BOOLEAN))
		{
			State.m_CanPlay = g_variant_get_boolean(pInnerValue);
		}
		else if(std::strcmp(pKey, "CanPause") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_BOOLEAN))
		{
			State.m_CanPause = g_variant_get_boolean(pInnerValue);
		}
		else if(std::strcmp(pKey, "CanGoPrevious") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_BOOLEAN))
		{
			State.m_CanGoPrevious = g_variant_get_boolean(pInnerValue);
		}
		else if(std::strcmp(pKey, "CanGoNext") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_BOOLEAN))
		{
			State.m_CanGoNext = g_variant_get_boolean(pInnerValue);
		}
		else if(std::strcmp(pKey, "Metadata") == 0 && g_variant_is_of_type(pInnerValue, G_VARIANT_TYPE_VARDICT))
		{
			ParseMprisMetadata(pInnerValue, State);
		}
		g_variant_unref(pInnerValue);
		g_variant_unref(pValue);
	}

	State.m_BusName = pBusName;
	g_variant_unref(pProperties);
	g_variant_unref(pResult);
	return true;
}

static bool QueryBestMprisPlayer(SMprisPlayerState &State)
{
	GError *pError = nullptr;
	GDBusConnection *pConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &pError);
	if(pError != nullptr)
	{
		g_error_free(pError);
		return false;
	}
	if(pConnection == nullptr)
		return false;

	bool FoundPlayer = false;
	SMprisPlayerState FallbackState;

	for(const std::string &BusName : GetMprisBusNames(pConnection))
	{
		SMprisPlayerState CandidateState;
		if(!QueryMprisPlayer(pConnection, BusName.c_str(), CandidateState))
			continue;

		if(!FoundPlayer)
		{
			FallbackState = CandidateState;
			FoundPlayer = true;
		}

		if(CandidateState.m_Playing)
		{
			State = std::move(CandidateState);
			g_object_unref(pConnection);
			return true;
		}
	}

	g_object_unref(pConnection);
	if(!FoundPlayer)
		return false;

	State = std::move(FallbackState);
	return true;
}

static void TriggerMprisControlAction(const char *pMethod)
{
	SMprisPlayerState PlayerState;
	if(!QueryBestMprisPlayer(PlayerState) || PlayerState.m_BusName.empty())
		return;

	if(pMethod == nullptr)
		return;

	GError *pError = nullptr;
	GDBusConnection *pConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &pError);
	if(pError != nullptr)
	{
		g_error_free(pError);
		return;
	}
	if(pConnection == nullptr)
		return;

	GVariant *pResult = MprisCallSync(pConnection, PlayerState.m_BusName.c_str(), gs_pMprisPlayerInterface, pMethod, nullptr, nullptr);
	if(pResult != nullptr)
		g_variant_unref(pResult);
	g_object_unref(pConnection);
}
#endif

#if defined(CONF_FAMILY_WINDOWS)
static std::string MakeArtworkKey(const std::string &Title, const std::string &Artist, const std::string &Album)
{
	std::string Key = Title;
	Key.push_back('\x1f');
	Key += Artist;
	Key.push_back('\x1f');
	Key += Album;
	return Key;
}

static bool DecodeThumbnailToImage(const winrt::Windows::Storage::Streams::IRandomAccessStreamReference &Thumbnail, CImageInfo &Image)
{
	using namespace winrt::Windows::Graphics::Imaging;

	if(!Thumbnail)
		return false;

	const auto Stream = Thumbnail.OpenReadAsync().get();
	if(!Stream || Stream.Size() == 0)
		return false;

	const auto Decoder = BitmapDecoder::CreateAsync(Stream).get();
	if(!Decoder)
		return false;

	constexpr uint32_t MaxArtworkSize = 128;
	const uint32_t Width = Decoder.PixelWidth();
	const uint32_t Height = Decoder.PixelHeight();
	if(Width == 0 || Height == 0)
		return false;

	uint32_t TargetWidth = Width;
	uint32_t TargetHeight = Height;
	BitmapTransform Transform;
	if(Width > MaxArtworkSize || Height > MaxArtworkSize)
	{
		if(Width >= Height)
		{
			TargetWidth = MaxArtworkSize;
			TargetHeight = maximum<uint32_t>(1, Height * MaxArtworkSize / Width);
		}
		else
		{
			TargetWidth = maximum<uint32_t>(1, Width * MaxArtworkSize / Height);
			TargetHeight = MaxArtworkSize;
		}
		Transform.ScaledWidth(TargetWidth);
		Transform.ScaledHeight(TargetHeight);
	}

	const auto PixelData = Decoder.GetPixelDataAsync(
		BitmapPixelFormat::Rgba8,
		BitmapAlphaMode::Straight,
		Transform,
		ExifOrientationMode::IgnoreExifOrientation,
		ColorManagementMode::DoNotColorManage)
							   .get();

	const winrt::com_array<uint8_t> Pixels = PixelData.DetachPixelData();
	const size_t ExpectedSize = (size_t)TargetWidth * TargetHeight * 4;
	if(Pixels.size() < ExpectedSize)
		return false;

	Image.Free();
	Image.m_Width = TargetWidth;
	Image.m_Height = TargetHeight;
	Image.m_Format = CImageInfo::FORMAT_RGBA;
	Image.m_pData = static_cast<uint8_t *>(malloc(ExpectedSize));
	if(Image.m_pData == nullptr)
	{
		Image.m_Width = 0;
		Image.m_Height = 0;
		Image.m_Format = CImageInfo::FORMAT_UNDEFINED;
		return false;
	}

	std::memcpy(Image.m_pData, Pixels.data(), ExpectedSize);
	return true;
}
#endif

CMusicIsland::CMusicIsland()
{
	OnReset();
}

CMusicIsland::~CMusicIsland()
{
	StopInfoWorker();
	StopImageWorker();
	ResetMusicImage();
}

void CMusicIsland::ResetMusicInfo()
{
	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	m_MusicInfo = {};
}

void CMusicIsland::ResetMusicImage()
{
	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	m_CurrentArtworkKey.clear();
	m_MusicImageDirty = false;
	m_PendingMusicImage.Free();

	if(m_MusicImageTexture.IsValid())
		Graphics()->UnloadTexture(&m_MusicImageTexture);
	m_MusicImageWidth = 0;
	m_MusicImageHeight = 0;
}

CMusicIsland::SMusicInfo CMusicIsland::GetMusicInfo() const
{
	std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
	return m_MusicInfo;
}

void CMusicIsland::OnConsoleInit()
{
	Console()->Register("ri_show_cur_music_info", "", CFGFLAG_CLIENT, ConShowCurMusicInfo, this, "Print current music info");
}

void CMusicIsland::SetExtended(bool Extended)
{
	m_Extended = Extended;
}

void CMusicIsland::OnReset()
{
	StopInfoWorker();
	StopImageWorker();
	m_Extended = false;
	m_ExtendAnim = 0.0f;
	m_LastNativeMousePressed = false;
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
	ResetMusicImage();
}

void CMusicIsland::OnShutdown()
{
	StopInfoWorker();
	StopImageWorker();
	m_Extended = false;
	m_ExtendAnim = 0.0f;
	m_LastNativeMousePressed = false;
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
	ResetMusicImage();
}

bool CMusicIsland::OnInput(const IInput::CEvent &Event)
{
	if(!IsActive())
		return false;

	if(!CanUseMouseInteraction())
		return false;

	if(Event.m_Key != KEY_MOUSE_1 || (Event.m_Flags & (IInput::FLAG_PRESS | IInput::FLAG_RELEASE)) == 0)
		return false;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	return m_Rect.Inside(NativeMouseToScreen(Input(), Graphics(), vec2(ScreenX0, ScreenY0), vec2(ScreenX1, ScreenY1)));
}

bool CMusicIsland::CanUseMouseInteraction() const
{
	return GameClient()->m_Chat.IsActive() ||
		GameClient()->m_Scoreboard.IsActive();
}

void CMusicIsland::OnRender()
{
}

void CMusicIsland::RenderHud()
{
	if(!IsActive())
		return;

	const int64_t Now = time_get();
	if(HasMusicIslandPlatformBackend() && !m_InfoWorkerRunning.load() && (m_NextInfoUpdateTime == 0 || Now >= m_NextInfoUpdateTime))
		StartInfoWorker(Now);

	if(g_Config.m_RiShowMusicIslandImage)
		UpdateMusicImageTexture();
	else
	{
		StopImageWorker();
		ResetMusicImage();
	}

	RenderMusicIsland();
}

void CMusicIsland::RenderMusicIsland()
{
	CUIRect WindowRect;

	vec2 ScreenTL, ScreenBR;
	Graphics()->GetScreen(&ScreenTL.x, &ScreenTL.y, &ScreenBR.x, &ScreenBR.y);

	WindowRect.h = SMusicIslandProperties::ms_BaseHeight;
	WindowRect.w = 75.0f;
	WindowRect.x = ScreenTL.x + (ScreenBR.x - ScreenTL.x - WindowRect.w) / 2.0f;
	WindowRect.y = ScreenTL.y + 2.5f;

	if(WindowRect.y + WindowRect.h > ScreenBR.y)
	{
		WindowRect.y -= WindowRect.y + WindowRect.h - ScreenBR.y;
	}
	if(WindowRect.x + WindowRect.w > ScreenBR.x)
	{
		WindowRect.x -= WindowRect.x + WindowRect.w - ScreenBR.x;
	}

	const float MainExtraHeight = SMusicIslandProperties::ms_ExpandedInfoHeight;
	const float ControlsExtraHeight = SMusicIslandProperties::ms_ControlGap + SMusicIslandProperties::ms_ControlHeight;
	const float ExpandedExtraHeight = MainExtraHeight + ControlsExtraHeight;
	const bool UseMouse = CanUseMouseInteraction();
	const vec2 MousePos = UseMouse ? NativeMouseToScreen(Input(), Graphics(), ScreenTL, ScreenBR) : vec2(-1.0f, -1.0f);
	const bool MousePressed = UseMouse && Input()->KeyIsPressed(KEY_MOUSE_1);
	const bool MouseClicked = UseMouse && MousePressed && !m_LastNativeMousePressed;

	CUIRect BaseHoverRect = WindowRect;
	CUIRect ExpandedHoverRect = WindowRect;
	ExpandedHoverRect.h += ExpandedExtraHeight;

	const bool Hovered = UseMouse && (m_ExtendAnim > 0.0f ? ExpandedHoverRect : BaseHoverRect).Inside(MousePos);
	m_Extended = Hovered;

	const float TargetAnim = Hovered ? 1.0f : 0.0f;
	const float AnimStep = Client()->RenderFrameTime() * 10.0f;
	if(m_ExtendAnim < TargetAnim)
		m_ExtendAnim = minimum(TargetAnim, m_ExtendAnim + AnimStep);
	else if(m_ExtendAnim > TargetAnim)
		m_ExtendAnim = maximum(TargetAnim, m_ExtendAnim - AnimStep);

	const float SmoothAnim = m_ExtendAnim * m_ExtendAnim * (3.0f - 2.0f * m_ExtendAnim);
	WindowRect.h += ExpandedExtraHeight * SmoothAnim;

	m_Rect = WindowRect;
	WindowRect.Draw(MusicIslandWindowColor(), IGraphics::CORNER_ALL, SMusicIslandProperties::ms_BaseHeight / 2.0f);

	CUIRect HeaderRect = WindowRect;
	CUIRect ControlsRect;
	if(SmoothAnim > 0.0f)
	{
		HeaderRect.HSplitTop(SMusicIslandProperties::ms_BaseHeight + MainExtraHeight * SmoothAnim, &HeaderRect, &ControlsRect);
		const float AnimatedGap = SMusicIslandProperties::ms_ControlGap * SmoothAnim;
		if(AnimatedGap > 0.0f)
			ControlsRect.HSplitTop(AnimatedGap, nullptr, &ControlsRect);
	}

	CUIRect Base = HeaderRect;
	CUIRect MusicImage, Visualizer;
	Base.VMargin(3.0f, &Base);
	Base.HMargin(SMusicIslandProperties::ms_Padding, &Base);
	if(g_Config.m_RiShowMusicIslandImage)
	{
		Base.VSplitLeft(8.0f, &MusicImage, &Base);
		MusicImage.HMargin(SMusicIslandProperties::ms_Padding, &MusicImage);
	}
	if(g_Config.m_RiShowMusicIslandVisualizer)
	{
		Base.VSplitRight(8.0f, &Base, &Visualizer);
	}
	Base.VMargin(1.0f, &Base);
	const SMusicInfo MusicInfo = GetMusicInfo();
	if(g_Config.m_RiShowMusicIslandImage)
		RenderMusicIslandImage(&MusicImage);
	if(g_Config.m_RiShowMusicIslandVisualizer)
		RenderMusicIslandVisualizer(&Visualizer);
	RenderMusicIslandMain(&Base);

	if(SmoothAnim > 0.0f)
		RenderMusicIslandControls(&ControlsRect, MusicInfo, MousePos, MouseClicked, MousePressed, SmoothAnim);

	m_LastNativeMousePressed = MousePressed;
}

bool CMusicIsland::DoControlButton(const CUIRect *pRect, const char *pIcon, bool Enabled, vec2 MousePos, bool MouseClicked, bool MousePressed, float AnimProgress)
{
	const bool Hovered = Enabled && pRect->Inside(MousePos);
	const bool Active = Hovered && MousePressed;
	const float BaseAlpha = !Enabled ? 0.3f : (Active ? 1.0f : (Hovered ? 0.95f : 0.75f));
	const float IconAlpha = BaseAlpha * AnimProgress;

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH |
		ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING |
		ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING);
	TextRender()->TextOutlineColor(TextRender()->DefaultTextOutlineColor().WithAlpha(IconAlpha));
	TextRender()->TextColor(TextRender()->DefaultTextColor().WithAlpha(IconAlpha));

	const float IconSize = pRect->h * 0.68f;
	const float IconWidth = TextRender()->TextWidth(IconSize, pIcon, -1, -1.0f);
	const float IconX = pRect->x + (pRect->w - IconWidth) / 2.0f;
	const float IconY = pRect->y + (pRect->h - IconSize) / 2.0f;
	TextRender()->Text(IconX, IconY, IconSize, pIcon, -1.0f);

	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextOutlineColor(TextRender()->DefaultTextOutlineColor());
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	return Enabled && Hovered && MouseClicked;
}

void CMusicIsland::RenderMusicIslandControls(CUIRect *pBase, const SMusicInfo &MusicInfo, vec2 MousePos, bool MouseClicked, bool MousePressed, float AnimProgress)
{
	CUIRect Controls = *pBase;
	Controls.Margin(vec2(14.0f, 0.7f), &Controls);
	if(Controls.w <= 0.0f || Controls.h <= 0.0f)
		return;

	const float Gap = 2.0f;
	const float ButtonWidth = (Controls.w - Gap * 2.0f) / 3.0f;
	if(ButtonWidth <= 0.0f)
		return;

	CUIRect PreviousButton;
	CUIRect PlayPauseButton;
	CUIRect NextButton;

	Controls.VSplitLeft(ButtonWidth, &PreviousButton, &Controls);
	Controls.VSplitLeft(Gap, nullptr, &Controls);
	Controls.VSplitLeft(ButtonWidth, &PlayPauseButton, &Controls);
	Controls.VSplitLeft(Gap, nullptr, &Controls);
	NextButton = Controls;

	if(DoControlButton(&PreviousButton, FontIcon::BACKWARD_STEP, MusicInfo.m_CanGoPrevious, MousePos, MouseClicked, MousePressed, AnimProgress))
		TriggerControlAction(CONTROL_BUTTON_PREVIOUS);

	const char *pPlayPauseIcon = MusicInfo.m_Playing ? FontIcon::PAUSE : FontIcon::PLAY;
	const bool CanTogglePlayback = MusicInfo.m_Playing ? MusicInfo.m_CanPause : MusicInfo.m_CanPlay;
	if(DoControlButton(&PlayPauseButton, pPlayPauseIcon, CanTogglePlayback, MousePos, MouseClicked, MousePressed, AnimProgress))
		TriggerControlAction(CONTROL_BUTTON_PLAY_PAUSE);

	if(DoControlButton(&NextButton, FontIcon::FORWARD_STEP, MusicInfo.m_CanGoNext, MousePos, MouseClicked, MousePressed, AnimProgress))
		TriggerControlAction(CONTROL_BUTTON_NEXT);
}

void CMusicIsland::StartInfoWorker(int64_t Now)
{
	if(m_InfoWorkerRunning.load())
		return;

	if(m_InfoWorker.joinable())
		m_InfoWorker.join();

	m_InfoWorkerStopRequested.store(false);
	m_InfoWorkerRunning.store(true);
	m_NextInfoUpdateTime = Now + time_freq();
	m_InfoWorker = std::thread(&CMusicIsland::InfoWorkerLoop, this);
}

void CMusicIsland::StopInfoWorker()
{
	m_InfoWorkerStopRequested.store(true);
	if(m_InfoWorker.joinable())
		m_InfoWorker.join();

	m_InfoWorkerRunning.store(false);
}

void CMusicIsland::StopImageWorker()
{
#if defined(CONF_FAMILY_WINDOWS)
	m_ImageWorkerStopRequested.store(true);
	if(m_ImageWorker.joinable())
		m_ImageWorker.join();

	m_ImageWorkerRunning.store(false);
#endif
}

void CMusicIsland::InfoWorkerLoop()
{
#if defined(CONF_FAMILY_WINDOWS)
	winrt::init_apartment(winrt::apartment_type::multi_threaded);
#endif
	UpdateMusicInfo();
#if defined(CONF_FAMILY_WINDOWS)
	winrt::uninit_apartment();
#endif
	m_InfoWorkerRunning.store(false);
}

void CMusicIsland::UpdateMusicInfo()
{
#if defined(CONF_FAMILY_WINDOWS)
	SMusicInfo NewInfo;
	const bool WantArtwork = g_Config.m_RiShowMusicIslandImage != 0;
	std::string NewArtworkKey;
	bool UpdateArtwork = false;
	bool HasThumbnail = false;
	winrt::Windows::Storage::Streams::IRandomAccessStreamReference Thumbnail = nullptr;

	try
	{
		using namespace winrt::Windows::Media::Control;

		const auto SessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
		const auto Session = SessionManager.GetCurrentSession();
		if(Session)
		{
			const auto PlaybackInfo = Session.GetPlaybackInfo();
			const auto MediaProperties = Session.TryGetMediaPropertiesAsync().get();
			const auto Controls = PlaybackInfo ? PlaybackInfo.Controls() : nullptr;

			NewInfo.m_Available = true;
			NewInfo.m_Playing = PlaybackInfo && PlaybackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
			NewInfo.m_CanPlay = Controls && Controls.IsPlayEnabled();
			NewInfo.m_CanPause = Controls && Controls.IsPauseEnabled();
			NewInfo.m_CanGoPrevious = Controls && Controls.IsPreviousEnabled();
			NewInfo.m_CanGoNext = Controls && Controls.IsNextEnabled();
			NewInfo.m_Title = winrt::to_string(MediaProperties.Title());
			NewInfo.m_Artist = winrt::to_string(MediaProperties.Artist());
			NewInfo.m_Album = winrt::to_string(MediaProperties.AlbumTitle());

			if(WantArtwork)
			{
				NewArtworkKey = MakeArtworkKey(NewInfo.m_Title, NewInfo.m_Artist, NewInfo.m_Album);

				{
					std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
					UpdateArtwork = NewArtworkKey != m_CurrentArtworkKey;
				}

				if(UpdateArtwork)
				{
					Thumbnail = MediaProperties.Thumbnail();
					HasThumbnail = Thumbnail != nullptr;
				}
			}
		}
		else if(WantArtwork)
		{
			std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
			UpdateArtwork = !m_CurrentArtworkKey.empty();
		}
	}
	catch(const winrt::hresult_error &)
	{
		NewInfo = {};
		if(WantArtwork)
		{
			std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
			UpdateArtwork = !m_CurrentArtworkKey.empty();
		}
	}
	catch(...)
	{
		NewInfo = {};
		if(WantArtwork)
		{
			std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
			UpdateArtwork = !m_CurrentArtworkKey.empty();
		}
	}

	if(m_InfoWorkerStopRequested.load())
		return;

	std::string ArtworkKey;
	{
		std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
		m_MusicInfo = std::move(NewInfo);
		if(UpdateArtwork)
		{
			m_CurrentArtworkKey = std::move(NewArtworkKey);
			ArtworkKey = m_CurrentArtworkKey;
		}
	}

	if(!UpdateArtwork)
		return;

	StopImageWorker();

	if(!HasThumbnail)
	{
		std::lock_guard<std::mutex> ImageGuard(m_MusicInfoMutex);
		m_MusicImageDirty = true;
		m_PendingMusicImage.Free();
		return;
	}

	const auto AgileThumbnail = winrt::agile_ref<winrt::Windows::Storage::Streams::IRandomAccessStreamReference>(Thumbnail);

	m_ImageWorkerStopRequested.store(false);
	m_ImageWorkerRunning.store(true);
	m_ImageWorker = std::thread([this, ArtworkKey, AgileThumbnail]() {
		winrt::init_apartment(winrt::apartment_type::multi_threaded);

		CImageInfo DecodedImage;
		try
		{
			const auto ThumbnailRef = AgileThumbnail.get();
			if(ThumbnailRef)
				DecodeThumbnailToImage(ThumbnailRef, DecodedImage);
		}
		catch(const winrt::hresult_error &)
		{
			DecodedImage.Free();
		}
		catch(...)
		{
			DecodedImage.Free();
		}

		if(!m_ImageWorkerStopRequested.load())
		{
			std::lock_guard<std::mutex> ImageGuard(m_MusicInfoMutex);
			if(m_CurrentArtworkKey == ArtworkKey)
			{
				m_MusicImageDirty = true;
				m_PendingMusicImage.Free();
				m_PendingMusicImage = std::move(DecodedImage);
			}
		}
		else
		{
			DecodedImage.Free();
		}

		winrt::uninit_apartment();
		m_ImageWorkerRunning.store(false);
	});
#elif defined(CONF_PLATFORM_LINUX) && defined(CONF_MUSIC_ISLAND_MPRIS)
	SMusicInfo NewInfo;
	SMprisPlayerState PlayerState;
	if(QueryBestMprisPlayer(PlayerState))
	{
		NewInfo.m_Available = true;
		NewInfo.m_Playing = PlayerState.m_Playing;
		NewInfo.m_CanPlay = PlayerState.m_CanPlay;
		NewInfo.m_CanPause = PlayerState.m_CanPause;
		NewInfo.m_CanGoPrevious = PlayerState.m_CanGoPrevious;
		NewInfo.m_CanGoNext = PlayerState.m_CanGoNext;
		NewInfo.m_Title = std::move(PlayerState.m_Title);
		NewInfo.m_Artist = std::move(PlayerState.m_Artist);
		NewInfo.m_Album = std::move(PlayerState.m_Album);
	}

	if(m_InfoWorkerStopRequested.load())
		return;

	{
		std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
		m_MusicInfo = std::move(NewInfo);
	}
#endif
}

void CMusicIsland::TriggerControlAction(EControlButton Button)
{
#if defined(CONF_FAMILY_WINDOWS)
	m_NextInfoUpdateTime = 0;
	std::thread([Button]() {
		try
		{
			winrt::init_apartment(winrt::apartment_type::multi_threaded);

			using namespace winrt::Windows::Media::Control;

			const auto SessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
			const auto Session = SessionManager.GetCurrentSession();
			if(Session)
			{
				switch(Button)
				{
				case CONTROL_BUTTON_PREVIOUS:
					Session.TrySkipPreviousAsync().get();
					break;
				case CONTROL_BUTTON_PLAY_PAUSE:
					Session.TryTogglePlayPauseAsync().get();
					break;
				case CONTROL_BUTTON_NEXT:
					Session.TrySkipNextAsync().get();
					break;
				default:
					break;
				}
			}

			winrt::uninit_apartment();
		}
		catch(...)
		{
		}
	}).detach();
#elif defined(CONF_PLATFORM_LINUX) && defined(CONF_MUSIC_ISLAND_MPRIS)
	m_NextInfoUpdateTime = 0;
	const char *pMethod = nullptr;
	switch(Button)
	{
	case CONTROL_BUTTON_PREVIOUS:
		pMethod = "Previous";
		break;
	case CONTROL_BUTTON_PLAY_PAUSE:
		pMethod = "PlayPause";
		break;
	case CONTROL_BUTTON_NEXT:
		pMethod = "Next";
		break;
	default:
		break;
	}
	std::thread([pMethod]() {
		TriggerMprisControlAction(pMethod);
	}).detach();
#else
	(void)Button;
#endif
}

bool CMusicIsland::IsActive() const
{
#if defined(CONF_FAMILY_WINDOWS) || defined(CONF_PLATFORM_LINUX)
	return g_Config.m_RiShowMusicIsland != 0;
#else
	return false;
#endif
}

void CMusicIsland::ConShowCurMusicInfo(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CMusicIsland *>(pUserData);
	const SMusicInfo Info = pSelf->GetMusicInfo();
	dbg_msg("Music", "Available: %d, Is playing: %d, Title: %s, Artist: %s", Info.m_Available, Info.m_Playing, Info.m_Title.c_str(), Info.m_Artist.c_str());
}

void CMusicIsland::RenderMusicIslandVisualizer(CUIRect *pBase)
{
	CUIRect VisualizerRect = *pBase;
	VisualizerRect.Margin(vec2(0.35f, 0.45f), &VisualizerRect);
	if(VisualizerRect.w <= 0.0f || VisualizerRect.h <= 0.0f)
		return;

	const SMusicInfo MusicInfo = GetMusicInfo();
	const float Time = LocalTime();
	const bool Animated = MusicInfo.m_Playing;
	const float MotionScale = Animated ? 1.0f : 0.0f;
	const float AlphaScale = MusicInfo.m_Playing ? 1.0f : (MusicInfo.m_Available ? 0.65f : 0.45f);
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float PixelSizeX = (ScreenX1 - ScreenX0) / Graphics()->ScreenWidth();
	const float PixelSizeY = (ScreenY1 - ScreenY0) / Graphics()->ScreenHeight();

	constexpr int BarCount = 5;
	const int CenterBar = BarCount / 2;
	float aBarWidths[BarCount];
	float TotalWidth = 0.0f;
	for(int BarIndex = 0; BarIndex < BarCount; ++BarIndex)
	{
		const int DistIndex = std::abs(BarIndex - CenterBar);
		const float ShapeStrength = 1.0f - DistIndex / 2.0f;
		aBarWidths[BarIndex] = 0.74f + ShapeStrength * 0.46f;
		TotalWidth += aBarWidths[BarIndex];
	}

	const float BarSpacing = 0.52f;
	TotalWidth += BarSpacing * (BarCount - 1);
	float CursorX = VisualizerRect.x + (VisualizerRect.w - TotalWidth) / 2.0f;
	const float CenterY = VisualizerRect.y + VisualizerRect.h / 2.0f;

	for(int BarIndex = 0; BarIndex < BarCount; ++BarIndex)
	{
		const float ShapeStrength = Animated ? (1.0f - std::abs(BarIndex - CenterBar) / 2.0f) : 0.0f;
		const float Width = aBarWidths[BarIndex];
		const float Pulse = Animated ? GetVisualizerBarPulse(Time, BarIndex) : 0.0f;
		const float BaseHeight = Animated ? (1.85f + ShapeStrength * 1.35f) : 1.65f;
		const float JumpHeight = (1.2f + ShapeStrength * 1.95f) * MotionScale;
		const float Height = minimum(VisualizerRect.h, BaseHeight + Pulse * JumpHeight);

		float Left = round_to_int(CursorX / PixelSizeX) * PixelSizeX;
		float Right = round_to_int((CursorX + Width) / PixelSizeX) * PixelSizeX;
		float Top = CenterY - Height / 2.0f;
		float Bottom = CenterY + Height / 2.0f;
		if(Right <= Left)
			Right = Left + PixelSizeX;
		if(Bottom <= Top)
			Bottom = Top + maximum(PixelSizeY, 0.001f);

		CUIRect BarRect = {Left, Top, Right - Left, Bottom - Top};
		const float BarAlpha = (0.62f + ShapeStrength * 0.33f) * AlphaScale;
		BarRect.Draw4(
			ColorRGBA(0.47f, 0.9f, 1.0f, BarAlpha),
			ColorRGBA(0.47f, 0.9f, 1.0f, BarAlpha),
			ColorRGBA(0.0f, 0.66f, 1.0f, BarAlpha * 0.92f),
			ColorRGBA(0.0f, 0.66f, 1.0f, BarAlpha * 0.92f),
			IGraphics::CORNER_ALL,
			BarRect.w / 2.0f);

		CursorX += Width + BarSpacing;
	}
}

void CMusicIsland::RenderMusicIslandMain(CUIRect *pBase)
{
	constexpr float BaseFontSize = 8.0f;
	constexpr float ExpandedTimerFontSize = 6.1f;
	constexpr float TitleFontSize = 4.3f;
	constexpr float ArtistFontSize = 3.5f;
	constexpr float MetadataGap = 0.4f;
	const float AnimProgress = m_ExtendAnim * m_ExtendAnim * (3.0f - 2.0f * m_ExtendAnim);
	const SMusicInfo MusicInfo = GetMusicInfo();
	SGameTimerRenderInfo RenderInfo;
	CUIRect TextRect = *pBase;
	TextRect.VMargin(0.5f, &TextRect);
	if(TextRect.w <= 0.0f || TextRect.h <= 0.0f)
		return;

	CUIRect TimerRect = TextRect;
	CUIRect MetadataRect = TextRect;
	const float TimerFontTarget = BaseFontSize + (ExpandedTimerFontSize - BaseFontSize) * AnimProgress;
	float TimerBlockHeight = TextRect.h;
	if(AnimProgress > 0.0f)
	{
		TimerBlockHeight = minimum(TextRect.h, 4.8f + 2.2f * (1.0f - AnimProgress));
		TextRect.HSplitTop(TimerBlockHeight, &TimerRect, &MetadataRect);
	}

	float RenderFontSize = minimum(TimerFontTarget, TimerRect.h);
	if(!GetGameTimerRenderInfo(GameClient()->m_Snap.m_pGameInfoObj, Client(), TextRender(), RenderFontSize, RenderInfo))
		return;

	if(g_Config.m_RiShowMusicIslandTimerFull && RenderInfo.m_TextWidth > TimerRect.w)
	{
		const float WidthScale = TimerRect.w / RenderInfo.m_TextWidth;
		RenderFontSize = maximum(1.0f, RenderFontSize * WidthScale);
		if(!GetGameTimerRenderInfo(GameClient()->m_Snap.m_pGameInfoObj, Client(), TextRender(), RenderFontSize, RenderInfo))
			return;
	}

	const bool ShouldScroll = !g_Config.m_RiShowMusicIslandTimerFull && RenderInfo.m_ActualTextWidth > TimerRect.w;
	const float LayoutWidth = g_Config.m_RiShowMusicIslandTimerFull ? RenderInfo.m_ActualTextWidth :
		(RenderInfo.m_TextWidth <= TimerRect.w ? RenderInfo.m_TextWidth : RenderInfo.m_ActualTextWidth);
	float TextX = TimerRect.x + (TimerRect.w - LayoutWidth) / 2.0f;
	if(ShouldScroll)
	{
		const float ScrollWidth = maximum(RenderInfo.m_TextWidth, RenderInfo.m_ActualTextWidth);
		const float Overflow = ScrollWidth - TimerRect.w;
		TextX = TimerRect.x - GetScrollingTextOffset(Overflow, LocalTime());
	}

	float TextY = TimerRect.y + (TimerRect.h - RenderFontSize) / 2.0f;
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float ScreenWidth = ScreenX1 - ScreenX0;
	const float ScreenHeight = ScreenY1 - ScreenY0;
	const float PixelSizeX = ScreenWidth / Graphics()->ScreenWidth();
	const float PixelSizeY = ScreenHeight / Graphics()->ScreenHeight();
	TextX = round_to_int(TextX / PixelSizeX) * PixelSizeX;
	TextY = round_to_int(TextY / PixelSizeY) * PixelSizeY;
	const int ClipX = (int)std::round((TimerRect.x - ScreenX0) * Graphics()->ScreenWidth() / ScreenWidth);
	const int ClipY = (int)std::round((TimerRect.y - ScreenY0) * Graphics()->ScreenHeight() / ScreenHeight);
	const int ClipW = (int)std::round(TimerRect.w * Graphics()->ScreenWidth() / ScreenWidth);
	const int ClipH = (int)std::round(TimerRect.h * Graphics()->ScreenHeight() / ScreenHeight);

	Graphics()->ClipEnable(ClipX, ClipY, ClipW, ClipH);
	TextRender()->TextColor(RenderInfo.m_TextColor);
	TextRender()->Text(TextX, TextY, RenderFontSize, RenderInfo.m_aText, -1.0f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->ClipDisable();

	if(AnimProgress <= 0.0f)
		return;

	const float MetadataAlpha = AnimProgress * AnimProgress;
	MetadataRect.HMargin(0.25f, &MetadataRect);
	if(MetadataRect.w <= 0.0f || MetadataRect.h <= 0.0f)
		return;

	const float TitleLineHeight = TitleFontSize + 0.6f;
	const float ArtistLineHeight = ArtistFontSize + 0.5f;
	const float TotalTextHeight = TitleLineHeight + MetadataGap + ArtistLineHeight;
	if(MetadataRect.h < TotalTextHeight)
		return;

	CUIRect TitleRect = MetadataRect;
	CUIRect ArtistRect = MetadataRect;
	const float TopOffset = (MetadataRect.h - TotalTextHeight) / 2.0f;
	TitleRect.y += TopOffset;
	TitleRect.h = TitleLineHeight;
	ArtistRect.y = TitleRect.y + TitleRect.h + MetadataGap;
	ArtistRect.h = ArtistLineHeight;

	RenderCenteredClippedText(Graphics(), TextRender(), TitleRect, MusicInfo.m_Title.c_str(), TitleFontSize, ColorRGBA(1.0f, 1.0f, 1.0f, MetadataAlpha), LocalTime());
	RenderCenteredClippedText(Graphics(), TextRender(), ArtistRect, MusicInfo.m_Artist.c_str(), ArtistFontSize, ColorRGBA(0.82f, 0.86f, 0.92f, MetadataAlpha), LocalTime() + 0.8f);
}

void CMusicIsland::UpdateMusicImageTexture()
{
	CImageInfo PendingImage;

	{
		std::lock_guard<std::mutex> Guard(m_MusicInfoMutex);
		if(!m_MusicImageDirty)
			return;

		PendingImage = std::move(m_PendingMusicImage);
		m_MusicImageDirty = false;
	}

	if(m_MusicImageTexture.IsValid())
		Graphics()->UnloadTexture(&m_MusicImageTexture);

	m_MusicImageWidth = 0;
	m_MusicImageHeight = 0;
	if(PendingImage.m_pData == nullptr)
		return;

	m_MusicImageWidth = (int)PendingImage.m_Width;
	m_MusicImageHeight = (int)PendingImage.m_Height;
	m_MusicImageTexture = Graphics()->LoadTextureRawMove(PendingImage, 0, "music-island-artwork");
}

void CMusicIsland::RenderMusicIslandImage(CUIRect *pBase)
{
	CUIRect ImageRect = *pBase;
	const float CubeSize = minimum(ImageRect.w, ImageRect.h);
	ImageRect.x += ImageRect.w - CubeSize;
	ImageRect.y += (ImageRect.h - CubeSize) / 2.0f;
	ImageRect.w = CubeSize;
	ImageRect.h = CubeSize;

	ImageRect.Draw(SMusicIslandProperties::WindowColorDark(), IGraphics::CORNER_ALL, SMusicIslandProperties::ms_Rounding);

	if(m_MusicImageTexture.IsValid() && m_MusicImageWidth > 0 && m_MusicImageHeight > 0)
	{
		Graphics()->WrapClamp();
		Graphics()->TextureSet(m_MusicImageTexture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		const IGraphics::CQuadItem QuadItem(ImageRect.x, ImageRect.y, ImageRect.w, ImageRect.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();
		return;
	}

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	const float IconSize = minimum(ImageRect.w, ImageRect.h) * 0.75f;
	const float IconWidth = TextRender()->TextWidth(IconSize, FontIcon::MUSIC);
	const float IconX = ImageRect.x + (ImageRect.w - IconWidth) / 2.0f;
	const float IconY = ImageRect.y + (ImageRect.h - IconSize) / 2.0f;
	TextRender()->Text(IconX, IconY, IconSize, FontIcon::MUSIC, -1.0f);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}
