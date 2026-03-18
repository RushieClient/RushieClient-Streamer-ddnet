#include "music_island.h"
#include <game/client/gameclient.h>

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

void CMusicIsland::OnConsoleInit()
{
}

void CMusicIsland::SetExtended(bool Extended)
{
	if(m_Extended == Extended)
		return;

	m_Extended = Extended;
	if(m_Extended)
	{

	}
	else
	{
		OnReset();
	}
}

void CMusicIsland::OnReset()
{
	RIMusicReset();
	SetExtended(false);
}

void CMusicIsland::OnRelease()
{
	RIMusicReset();
	SetExtended(false);
}

void CMusicIsland::OnRender()
{
	if(!IsActive())
		return;

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

	Base.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, SEdgeHelperProperties::ms_Rounding);
	Base.Margin(SEdgeHelperProperties::ms_Padding, &Base);
}

bool CMusicIsland::IsActive() const
{
#if defined(CONF_FAMILY_WINDOWS)
	return true;
#else
	return false;
#endif
}
