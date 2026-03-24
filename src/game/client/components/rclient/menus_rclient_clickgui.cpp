#include "menus_rclient_clickgui.h"

#include "base/str.h"
#include "engine/shared/config.h"
#include "game/client/animstate.h"
#include "game/client/gameclient.h"
#include "game/client/render.h"
#include "generated/client_data.h"

#include <algorithm>
#include <chrono>
#include <engine/graphics.h>

#include <game/client/ui.h>
#include <game/localization.h>

using namespace std::chrono_literals;

enum
{
	CLICKGUI_TAB_SETTINGS = 0,
	CLICKGUI_TAB_VOICE,
	CLICKGUI_TAB_INFO,
	NUM_CLICKGUI_TABS
};

struct SClickGuiProperties
{
	static constexpr float ms_Width = 1200.0f;
	static constexpr float ms_Height = 680.0f;

	static constexpr float ms_SmallVMargin = 15.0f;
	static constexpr float ms_DefaultVMargin = 25.0f;

	static constexpr float ms_NicknameSizeWidth = 210.0f;
	static constexpr float ms_NicknameSizeHeight = 30.0f;
	static constexpr float ms_LogoSizeWidth = 135.0f;
	static constexpr float ms_NicknameLogoSpace = 305.0f;
	static constexpr float ms_TeeSkinSize = 30.0f;
	static constexpr float ms_BarSizeHeight = 40.0f;

	static constexpr float ms_SettingsProfilesWidth = 255.0f;
	static constexpr float ms_SettingsProfilesHeight = 535.0f;
	static constexpr float ms_HudEditorHeightGapProfiles = 90.0f;
	static constexpr float ms_ButtonSpace = 7.5f;
	static constexpr float ms_SmallButtonSpace = 5.0f;

	static constexpr float ms_SettingsTabsWidth = 285.0f;

	static constexpr float ms_Rounding = 10.0f;

	static ColorRGBA Hex141414Color() { return ColorRGBA(0.0784f, 0.0784f, 0.0784f, 1.0f); };
	static ColorRGBA Hex1E1E1EColor() { return ColorRGBA(0.1176f, 0.1176f, 0.1176f, 1.0f); };
	static ColorRGBA Hex2A2A2AColor() { return ColorRGBA(0.1647f, 0.1647f, 0.1647f, 1.0f); };
	static ColorRGBA Hex4E4E4EColor() { return ColorRGBA(0.3059f, 0.3059f, 0.3059f, 1.0f); };
};

static int DoButton_MenuTab(CUi *pUi, CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, int Corners, SUIAnimator *pAnimator = nullptr, const ColorRGBA *pDefaultColor = nullptr, const ColorRGBA *pActiveColor = nullptr, const ColorRGBA *pHoverColor = nullptr, float EdgeRounding = 10.0f, float FontScale = 1.0f)
{
	const bool MouseInside = pUi->HotItem() == pButtonContainer;
	CUIRect Rect = *pRect;

	if(pAnimator != nullptr)
	{
		const auto Time = time_get_nanoseconds();

		if(pAnimator->m_Time + 100ms < Time)
		{
			pAnimator->m_Value = pAnimator->m_Active ? 1 : 0;
			pAnimator->m_Time = Time;
		}

		pAnimator->m_Active = Checked || MouseInside;

		if(pAnimator->m_Active)
			pAnimator->m_Value = std::clamp<float>(pAnimator->m_Value + (Time - pAnimator->m_Time).count() / (double)std::chrono::nanoseconds(100ms).count(), 0.0f, 1.0f);
		else
			pAnimator->m_Value = std::clamp<float>(pAnimator->m_Value - (Time - pAnimator->m_Time).count() / (double)std::chrono::nanoseconds(100ms).count(), 0.0f, 1.0f);

		Rect.w += pAnimator->m_Value * pAnimator->m_WOffset;
		Rect.h += pAnimator->m_Value * pAnimator->m_HOffset;
		Rect.x += pAnimator->m_Value * pAnimator->m_XOffset;
		Rect.y += pAnimator->m_Value * pAnimator->m_YOffset;

		pAnimator->m_Time = Time;
	}

	if(Checked)
	{
		Rect.Draw(pActiveColor ? *pActiveColor : ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), Corners, EdgeRounding);
	}
	else if(MouseInside)
	{
		Rect.Draw(pHoverColor ? *pHoverColor : ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), Corners, EdgeRounding);
	}
	else
	{
		Rect.Draw(pDefaultColor ? *pDefaultColor : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), Corners, EdgeRounding);
	}

	if(pAnimator != nullptr)
	{
		if(pAnimator->m_RepositionLabel)
		{
			Rect.x += Rect.w - pRect->w + Rect.x - pRect->x;
			Rect.y += Rect.h - pRect->h + Rect.y - pRect->y;
		}

		if(!pAnimator->m_ScaleLabel)
		{
			Rect.w = pRect->w;
			Rect.h = pRect->h;
		}
	}

	CUIRect Label;
	Rect.HMargin(2.0f, &Label);
	pUi->DoLabel(&Label, pText, FontScale, TEXTALIGN_MC);

	return pUi->DoButtonLogic(pButtonContainer, Checked, pRect, BUTTONFLAG_LEFT);
}

void CMenusRClientClickGui::OnConsoleInit()
{
	Console()->Register("toggle_rclient_clickgui", "", CFGFLAG_CLIENT, ConToggleClickGui, this, "Toggle RClient click GUI");
}

void CMenusRClientClickGui::ConToggleClickGui(IConsole::IResult *pResult, void *pUserData)
{
	CMenusRClientClickGui *pSelf = static_cast<CMenusRClientClickGui *>(pUserData);
	pSelf->SetActive(!pSelf->IsActive());
}

void CMenusRClientClickGui::SetActive(bool Active)
{
	m_Active = Active;
}

void CMenusRClientClickGui::OnRender()
{
	if(!m_Active)
		return;

	vec2 ScreenTopLeft;
	vec2 ScreenBottomRight;
	Graphics()->GetScreen(&ScreenTopLeft.x, &ScreenTopLeft.y, &ScreenBottomRight.x, &ScreenBottomRight.y);

	const float ScreenWidth = ScreenBottomRight.x - ScreenTopLeft.x;
	const float ScreenHeight = ScreenBottomRight.y - ScreenTopLeft.y;

	const float PixelSize = ScreenHeight / Graphics()->ScreenHeight();
	const float WindowWidth = SClickGuiProperties::ms_Width * PixelSize;
	const float WindowHeight = SClickGuiProperties::ms_Height * PixelSize;
	const float SmallVMargin = SClickGuiProperties::ms_SmallVMargin * PixelSize;
	const float DefaultVMargin = SClickGuiProperties::ms_DefaultVMargin * PixelSize;
	const float NicknameSizeWidth = SClickGuiProperties::ms_NicknameSizeWidth * PixelSize;
	const float NicknameSizeHeight = SClickGuiProperties::ms_NicknameSizeHeight * PixelSize;
	const float LogoSizeWidth = SClickGuiProperties::ms_LogoSizeWidth * PixelSize;
	const float TeeSkinSize = SClickGuiProperties::ms_TeeSkinSize * PixelSize;
	const float BarSizeHeight = SClickGuiProperties::ms_BarSizeHeight * PixelSize;
	const float DefaultRounding = SClickGuiProperties::ms_Rounding * PixelSize;
	const float SettingsProfilesWidth = SClickGuiProperties::ms_SettingsProfilesWidth * PixelSize;
	const float SettingsProfilesHeight = SClickGuiProperties::ms_SettingsProfilesHeight * PixelSize;
	const float HudEditorHeightGapProfiles = SClickGuiProperties::ms_HudEditorHeightGapProfiles * PixelSize;
	const float ButtonSpace = SClickGuiProperties::ms_ButtonSpace * PixelSize;
	const float SmallButtonSpace = SClickGuiProperties::ms_SmallButtonSpace * PixelSize;
	const float SettingsTabsWidth = SClickGuiProperties::ms_SettingsTabsWidth * PixelSize;

	CUIRect Window = {
		ScreenTopLeft.x + (ScreenWidth - WindowWidth) * 0.5f,
		ScreenTopLeft.y + (ScreenHeight - WindowHeight) * 0.5f,
		WindowWidth,
		WindowHeight};
	Window.Draw(SClickGuiProperties::Hex141414Color(), IGraphics::CORNER_ALL, DefaultRounding);


	// TopBar start
	CUIRect TopBar, TopBarContent, TeeSkin;
	CUIRect Body;
	Window.HSplitTop(BarSizeHeight, &TopBar, &Body);
	TopBar.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_T, DefaultRounding);
	TopBar.VMargin(SmallVMargin, &TopBar);
	TopBar.HMargin(5.0f * PixelSize, &TopBar);
	const float TopBarGaps = (TopBar.w - NicknameSizeWidth * 2 - LogoSizeWidth) / 2;

	TopBar.VSplitLeft(NicknameSizeWidth, &TopBarContent, &TopBar);
	TopBarContent.VSplitLeft(TeeSkinSize, &TeeSkin, &TopBarContent);
	RenderDevSkin(TeeSkin.Center(), 40.0f * PixelSize, g_Config.m_ClPlayerSkin, "default", g_Config.m_ClPlayerUseCustomColor, g_Config.m_ClPlayerColorFeet, g_Config.m_ClPlayerColorBody, 0, true);
	TopBarContent.VSplitLeft(SmallVMargin, nullptr, &TopBarContent);
	Ui()->DoLabel(&TopBarContent, g_Config.m_PlayerName, 24.0f * PixelSize, TEXTALIGN_ML);

	TopBar.VSplitRight(NicknameSizeWidth, &TopBar, &TopBarContent);
	TopBarContent.VSplitRight(TeeSkinSize, &TopBarContent, &TeeSkin);
	RenderDevSkin(TeeSkin.Center(), 40.0f * PixelSize, "Bomb 2", "bomb", false, 0, 0, 0, true);
	TopBarContent.VSplitRight(SmallVMargin, &TopBarContent, nullptr);
	Ui()->DoLabel(&TopBarContent, "Client by Voix", 24.0f * PixelSize, TEXTALIGN_MR);

	TopBar.VMargin(TopBarGaps, &TopBar);
	TopBar.VSplitRight(TeeSkinSize, &TopBarContent, &TeeSkin);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RIICON].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem Quad(TeeSkin.x, TeeSkin.y, TeeSkin.w, TeeSkin.h);
	Graphics()->QuadsDrawTL(&Quad, 1);
	Graphics()->QuadsEnd();
	TopBarContent.VSplitRight(SmallVMargin, &TopBarContent, nullptr);
	Ui()->DoLabel(&TopBarContent, "RClient", 24.0f * PixelSize, TEXTALIGN_MC);
	// TopBar end

	Body.HSplitTop(SmallVMargin, nullptr, &Body);

	// LEFTSIDE start
	CUIRect SettingsProfiles, HudEditor;
	Body.VSplitLeft(SettingsProfilesWidth, &SettingsProfiles, &Body);
	SettingsProfiles.HSplitBottom(HudEditorHeightGapProfiles, &SettingsProfiles, &HudEditor);

	//Settings Profiles start
	CUIRect NewProfileBTN;
	SettingsProfiles.Draw(SClickGuiProperties::Hex1E1E1EColor(), IGraphics::CORNER_R, DefaultRounding);
	SettingsProfiles.HSplitBottom(BarSizeHeight, &SettingsProfiles, &NewProfileBTN);
	NewProfileBTN.Draw(SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_BR, DefaultRounding);
	NewProfileBTN.HMargin(SmallButtonSpace, &NewProfileBTN);
	NewProfileBTN.VMargin(ButtonSpace, &NewProfileBTN);
	NewProfileBTN.Draw(SClickGuiProperties::Hex4E4E4EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	Ui()->DoLabel(&NewProfileBTN, "NEW PROFILE", 16.0f * PixelSize, TEXTALIGN_MC);
	//Settings Profiles end

	//Hud editor start
	HudEditor.Margin(DefaultVMargin, &HudEditor);
	HudEditor.Draw(SClickGuiProperties::Hex4E4E4EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	Ui()->DoLabel(&HudEditor, "EDIT HUD", 16.0f * PixelSize, TEXTALIGN_MC);
	//Hud editor end
	//LEFTSIDE end

	//RIGHTSIDE start
	CUIRect SettingsTabs;
	Body.VMargin(DefaultVMargin, &Body);
	Body.HSplitTop(TeeSkinSize, &SettingsTabs, &Body);
	const float SettingsTabsAutoGaps = (SettingsTabs.w - SettingsTabsWidth * NUM_CLICKGUI_TABS) / (NUM_CLICKGUI_TABS - 1);
	const ColorRGBA SettingsTabActiveColor = SClickGuiProperties::Hex2A2A2AColor();
	const ColorRGBA SettingsTabColor = ColorRGBA(SettingsTabActiveColor.r, SettingsTabActiveColor.g, SettingsTabActiveColor.b, 0.75f);
	const float SettingsTabsFontScale = 14.0f * PixelSize;

	static int s_CurTab = CLICKGUI_TAB_SETTINGS;
	static CButtonContainer s_aTabButtons[NUM_CLICKGUI_TABS];
	const char *apTabNames[NUM_CLICKGUI_TABS] = {
		Localize("Settings"),
		Localize("Voice"),
		Localize("Info")};

	for(int Tab = 0; Tab < NUM_CLICKGUI_TABS; ++Tab)
	{
		CUIRect Button;
		SettingsTabs.VSplitLeft(SettingsTabsWidth, &Button, &SettingsTabs);
		if(DoButton_MenuTab(Ui(), &s_aTabButtons[Tab], apTabNames[Tab], s_CurTab == Tab, &Button, IGraphics::CORNER_ALL, nullptr, &SettingsTabColor, &SettingsTabActiveColor, &SettingsTabActiveColor, DefaultRounding, SettingsTabsFontScale))
			s_CurTab = Tab;

		if(Tab != NUM_CLICKGUI_TABS - 1)
			SettingsTabs.VSplitLeft(SettingsTabsAutoGaps, nullptr, &SettingsTabs);
	}


}


void CMenusRClientClickGui::RenderDevSkin(vec2 RenderPos, float Size, const char *pSkinName, const char *pBackupSkin, bool CustomColors, int FeetColor, int BodyColor, int Emote, bool Cute, ColorRGBA ColorFeet, ColorRGBA ColorBody)
{
	bool WhiteFeetTemp = g_Config.m_TcWhiteFeet;
	g_Config.m_TcWhiteFeet = false;

	CTeeRenderInfo SkinInfo;
	const CSkin *pSkin = GameClient()->m_Skins.Find(pSkinName);
	if(str_comp(pSkin->GetName(), pSkinName) != 0)
		pSkin = GameClient()->m_Skins.Find(pBackupSkin);

	SkinInfo.m_OriginalRenderSkin = pSkin->m_OriginalSkin;
	SkinInfo.m_ColorableRenderSkin = pSkin->m_ColorableSkin;
	SkinInfo.m_SkinMetrics = pSkin->m_Metrics;
	SkinInfo.m_CustomColoredSkin = CustomColors;
	if(SkinInfo.m_CustomColoredSkin)
	{
		SkinInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(BodyColor).UnclampLighting(ColorHSLA::DARKEST_LGT));
		SkinInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(FeetColor).UnclampLighting(ColorHSLA::DARKEST_LGT));
		if(ColorFeet.a != 0.0f)
		{
			SkinInfo.m_ColorBody = ColorBody;
			SkinInfo.m_ColorFeet = ColorFeet;
		}
	}
	else
	{
		SkinInfo.m_ColorBody = ColorRGBA(1.0f, 1.0f, 1.0f);
		SkinInfo.m_ColorFeet = ColorRGBA(1.0f, 1.0f, 1.0f);
	}
	SkinInfo.m_Size = Size;
	const CAnimState *pIdleState = CAnimState::GetIdle();
	vec2 OffsetToMid;
	CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &SkinInfo, OffsetToMid);
	vec2 TeeRenderPos(RenderPos.x, RenderPos.y + OffsetToMid.y);
	if(Cute)
		RenderTeeCute(pIdleState, &SkinInfo, Emote, vec2(1.0f, 0.0f), TeeRenderPos, true);
	else
		RenderTools()->RenderTee(pIdleState, &SkinInfo, Emote, vec2(1.0f, 0.0f), TeeRenderPos);
	g_Config.m_TcWhiteFeet = WhiteFeetTemp;
}

void CMenusRClientClickGui::RenderTeeCute(const CAnimState *pAnim, const CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos, bool CuteEyes, float Alpha)
{
	Dir = Ui()->MousePos() - Pos;
	if(pInfo->m_Size > 0.0f)
		Dir /= pInfo->m_Size;
	const float Length = length(Dir);
	if(Length > 1.0f)
		Dir /= Length;
	if(CuteEyes && Length < 0.4f)
		Emote = 2;
	RenderTools()->RenderTee(pAnim, pInfo, Emote, Dir, Pos, Alpha);
}
