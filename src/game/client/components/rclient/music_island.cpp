#include "music_island.h"

#include <engine/font_icons.h>
#include <engine/gfx/image_manipulation.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>

#include <cstring>

#if defined(CONF_FAMILY_WINDOWS)
#pragma comment(lib, "runtimeobject.lib")
#include <winrt/base.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#endif

struct SEdgeHelperProperties
{
	static constexpr float ms_Padding = 1.0f;
	static constexpr float ms_Rounding = 3.0f;

	static constexpr float ms_ItemSpacing = 2.0f;

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
	Console()->Register("ri_show_cur_info", "", CFGFLAG_CLIENT, ConShowCurInfo, this, "Print current music info");
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
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
	ResetMusicImage();
}

void CMusicIsland::OnShutdown()
{
	StopInfoWorker();
	StopImageWorker();
	m_Extended = false;
	m_NextInfoUpdateTime = 0;
	ResetMusicInfo();
	ResetMusicImage();
}

void CMusicIsland::OnRender()
{
	if(!IsActive())
		return;

	const int64_t Now = time_get();
	if(!m_InfoWorkerRunning.load() && (m_NextInfoUpdateTime == 0 || Now >= m_NextInfoUpdateTime))
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
	CUIRect Base, MusicImage, Visualizer;

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

	Base.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, Base.h / 2);
	Base.Margin(SEdgeHelperProperties::ms_Padding, &Base);
	Base.VSplitLeft(15.0f, &MusicImage, &Base);
	MusicImage.Margin(SEdgeHelperProperties::ms_Padding, &MusicImage);
	Base.VSplitRight(15.0f, &Base, &Visualizer);
	Base.VMargin(5.0f, &Base);
	if(g_Config.m_RiShowMusicIslandImage)
		RenderMusicIslandImage(&MusicImage);
	if(g_Config.m_RiShowMusicIslandVisualizer)
		RenderMusicIslandVisualizer(&Visualizer);
	RenderMusicIslandMain(&Base);
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
	UpdateMusicInfo();
	winrt::uninit_apartment();
	m_InfoWorkerRunning.store(false);
#endif
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

			NewInfo.m_Available = true;
			NewInfo.m_Playing = PlaybackInfo && PlaybackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
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
#endif
}

bool CMusicIsland::IsActive() const
{
#if defined(CONF_FAMILY_WINDOWS)
	if(g_Config.m_RiShowMusicIsland)
		return true;
	else
		return false;
#else
	return false;
#endif
}

void CMusicIsland::ConShowCurInfo(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CMusicIsland *>(pUserData);
	const SMusicInfo Info = pSelf->GetMusicInfo();
	dbg_msg("Music", "Available: %d, Is playing: %d, Title: %s, Artist: %s", Info.m_Available, Info.m_Playing, Info.m_Title.c_str(), Info.m_Artist.c_str());
}

void CMusicIsland::RenderMusicIslandVisualizer(CUIRect *pBase)
{

}

void CMusicIsland::RenderMusicIslandMain(CUIRect *pBase)
{

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
	ImageRect.x += (ImageRect.w - CubeSize) / 2.0f;
	ImageRect.y += (ImageRect.h - CubeSize) / 2.0f;
	ImageRect.w = CubeSize;
	ImageRect.h = CubeSize;

	ImageRect.Draw(SEdgeHelperProperties::WindowColorDark(), IGraphics::CORNER_ALL, SEdgeHelperProperties::ms_Rounding);

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
