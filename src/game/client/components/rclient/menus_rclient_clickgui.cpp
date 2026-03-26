#include "menus_rclient_clickgui.h"

#include "base/str.h"
#include "engine/font_icons.h"
#include "engine/shared/config.h"
#include "game/client/animstate.h"
#include "game/client/gameclient.h"
#include "game/client/render.h"
#include "generated/client_data.h"

#include <algorithm>
#include <chrono>
#include <engine/graphics.h>

#include <game/client/components/chat.h>
#include <game/client/components/console.h>
#include <game/client/components/emoticon.h>
#include <game/client/components/menus.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
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

	static constexpr float ms_SettingsFunctionWidth = 410.0f;
	static constexpr float ms_SettingsFunctionHeight = 180.0f;

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

struct SClickGuiSettingsEntry
{
	CMenus::ERushieSettingsSection m_Section;
	const char *m_pTitle;
	const char *m_pIcon;
	int *m_pMainToggle;
};

static const SClickGuiSettingsEntry gs_aClickGuiSettingsEntries[] = {
#define CLICKGUI_SETTINGS_ENTRY(Name, Title, Icon, MainToggle) \
	{CMenus::SETTINGS_SECTION_##Name, Title, Icon, MainToggle},
	RUSHIE_SETTINGS_SECTION_LIST(CLICKGUI_SETTINGS_ENTRY)
#undef CLICKGUI_SETTINGS_ENTRY
};
static constexpr int gs_NumClickGuiSettingsEntries = sizeof(gs_aClickGuiSettingsEntries) / sizeof(gs_aClickGuiSettingsEntries[0]);
static_assert(gs_NumClickGuiSettingsEntries == CMenus::NUM_RUSHIE_SETTINGS_SECTIONS, "Rushie settings list is out of sync");

void CMenusRClientClickGui::SetUiMousePos(vec2 Pos)
{
	const vec2 WindowSize = vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight());
	const CUIRect *pScreen = Ui()->Screen();

	const vec2 UpdatedMousePos = Ui()->UpdatedMousePos();
	Pos = Pos / vec2(pScreen->w, pScreen->h) * WindowSize;
	Ui()->OnCursorMove(Pos.x - UpdatedMousePos.x, Pos.y - UpdatedMousePos.y);
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
	if(m_Active == Active)
		return;

	vec2 OldMousePos = Ui()->MousePos();
	m_Active = Active;

	if(m_Active)
	{
		m_MouseUnlocked = true;
		if(m_LastMousePos.has_value())
			SetUiMousePos(m_LastMousePos.value());
		else
			SetUiMousePos(Ui()->Screen()->Center());
	}
	else
	{
		if(m_MouseUnlocked)
			Ui()->ClosePopupMenus();

		m_MouseUnlocked = false;
		ResetTransientState();
		if(m_LastMousePos.has_value())
			SetUiMousePos(m_LastMousePos.value());
	}

	m_LastMousePos = OldMousePos;
}

void CMenusRClientClickGui::OnReset()
{
	m_Active = false;
	m_MouseUnlocked = false;
	m_LastMousePos = std::nullopt;
	ResetTransientState();
}

void CMenusRClientClickGui::OnRelease()
{
	SetActive(false);
}

bool CMenusRClientClickGui::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!IsActive() || !m_MouseUnlocked)
		return false;

	if(GameClient()->m_GameConsole.IsActive() || GameClient()->m_Menus.IsActive() || GameClient()->m_Chat.IsActive() || GameClient()->m_Emoticon.IsActive())
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);

	return true;
}

bool CMenusRClientClickGui::OnInput(const IInput::CEvent &Event)
{
	if(!IsActive())
		return false;

	if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		HandleEscape();
		return true;
	}

	if(GameClient()->m_GameConsole.IsActive() || GameClient()->m_Menus.IsActive() || GameClient()->m_Chat.IsActive() || GameClient()->m_Emoticon.IsActive())
		return false;

	Ui()->OnInput(Event);

	return m_MouseUnlocked;
}

bool CMenusRClientClickGui::HandleEscape()
{
	if(m_CurrentTab == CLICKGUI_TAB_SETTINGS && m_OpenSettingsSection < 0)
	{
		const bool SearchActive = Ui()->CheckActiveItem(&m_SearchInput);
		const bool HasSearchText = m_SearchInput.GetString()[0] != '\0';
		if(SearchActive || HasSearchText)
		{
			m_SearchInput.Clear();
			m_SearchInput.Deactivate();
			Ui()->SetActiveItem(&m_FocusResetAnchor);
			return true;
		}
	}

	if(m_CurrentTab == CLICKGUI_TAB_SETTINGS && m_OpenSettingsSection >= 0)
	{
		m_OpenSettingsSection = -1;
		return true;
	}

	SetActive(false);
	return true;
}

void CMenusRClientClickGui::ResetTransientState()
{
	m_OpenSettingsSection = -1;
	m_SearchInput.Clear();
	m_SearchRect = std::nullopt;
	m_SearchInput.Deactivate();
	Ui()->SetActiveItem(&m_FocusResetAnchor);
}

void CMenusRClientClickGui::OnRender()
{
	if(!m_Active)
		return;

	const bool UiBlocked = GameClient()->m_GameConsole.IsActive() || GameClient()->m_Menus.IsActive() || GameClient()->m_Chat.IsActive() || GameClient()->m_Emoticon.IsActive();
	if(!UiBlocked)
	{
		Ui()->StartCheck();
		Ui()->Update();
	}

	const CUIRect Screen = *Ui()->Screen();
	Ui()->MapScreen();

	const float ScreenWidth = Screen.w;
	const float ScreenHeight = Screen.h;

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
		Screen.x + (ScreenWidth - WindowWidth) * 0.5f,
		Screen.y + (ScreenHeight - WindowHeight) * 0.5f,
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
	//Tabs start
	CUIRect SettingsTabs;
	Body.VMargin(DefaultVMargin, &Body);
	Body.HSplitTop(TeeSkinSize, &SettingsTabs, &Body);
	const float SettingsTabsAutoGaps = (SettingsTabs.w - SettingsTabsWidth * NUM_CLICKGUI_TABS) / (NUM_CLICKGUI_TABS - 1);
	const ColorRGBA SettingsTabActiveColor = SClickGuiProperties::Hex2A2A2AColor();
	const ColorRGBA SettingsTabColor = ColorRGBA(SettingsTabActiveColor.r, SettingsTabActiveColor.g, SettingsTabActiveColor.b, 0.75f);
	const float SettingsTabsFontScale = 14.0f * PixelSize;

	static CButtonContainer s_aTabButtons[NUM_CLICKGUI_TABS];
	const char *apTabNames[NUM_CLICKGUI_TABS] = {
		Localize("Settings"),
		Localize("Voice"),
		Localize("Info")};

	for(int Tab = 0; Tab < NUM_CLICKGUI_TABS; ++Tab)
	{
		CUIRect Button;
		SettingsTabs.VSplitLeft(SettingsTabsWidth, &Button, &SettingsTabs);
		if(DoButton_MenuTab(Ui(), &s_aTabButtons[Tab], apTabNames[Tab], m_CurrentTab == Tab, &Button, IGraphics::CORNER_ALL, nullptr, &SettingsTabColor, &SettingsTabActiveColor, &SettingsTabActiveColor, DefaultRounding, SettingsTabsFontScale))
			m_CurrentTab = Tab;

		if(Tab != NUM_CLICKGUI_TABS - 1)
			SettingsTabs.VSplitLeft(SettingsTabsAutoGaps, nullptr, &SettingsTabs);
	}
	//Tabs end

	Body.HMargin(DefaultVMargin, &Body);

	//Main render
	Body.Draw(SClickGuiProperties::Hex1E1E1EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	if(m_CurrentTab == CLICKGUI_TAB_SETTINGS)
		RenderClickGuiRushieSettings(Body, PixelSize);
	if(m_CurrentTab == CLICKGUI_TAB_VOICE)
		RenderClickGuiRushieVoice(Body, PixelSize);
	if(m_CurrentTab == CLICKGUI_TAB_INFO)
		RenderClickGuiRushieInfo(Body, PixelSize);

	if(m_MouseUnlocked && !UiBlocked)
		RenderTools()->RenderCursor(Ui()->MousePos(), 24.0f);

	if(!UiBlocked)
		Ui()->FinishCheck();
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

void CMenusRClientClickGui::RenderClickGuiRushieSettings(CUIRect MainView, float ScreenPixelSize)
{
	const float SettingsFunctionWidth = SClickGuiProperties::ms_SettingsFunctionWidth * ScreenPixelSize;
	const float SettingsFunctionHeight = SClickGuiProperties::ms_SettingsFunctionHeight * ScreenPixelSize;
	const float DefaultVMargin = SClickGuiProperties::ms_DefaultVMargin * ScreenPixelSize;
	const float SmallVMargin = SClickGuiProperties::ms_SmallVMargin * ScreenPixelSize;
	const float DefaultRounding = SClickGuiProperties::ms_Rounding * ScreenPixelSize;
	const float ButtonSpace = SClickGuiProperties::ms_ButtonSpace * ScreenPixelSize;
	const float SmallButtonSpace = SClickGuiProperties::ms_SmallButtonSpace * ScreenPixelSize;
	const float ScrollbarWidth = 20.0f * ScreenPixelSize;

	static CButtonContainer s_aOpenButtons[gs_NumClickGuiSettingsEntries];
	static CButtonContainer s_aToggleButtons[gs_NumClickGuiSettingsEntries];
	static CButtonContainer s_BackButton;
	static CScrollRegion s_OverviewScroll;
	static vec2 s_OverviewOffset(0.0f, 0.0f);
	static CScrollRegion s_aDetailScroll[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	static vec2 s_aDetailOffsets[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	auto ApplyFunctionInsets = [&](CUIRect &Rect, float ScrollbarWidth, float TopInset) {
		Rect.HSplitTop(TopInset, nullptr, &Rect);
		Rect.HSplitBottom(DefaultVMargin, &Rect, nullptr);
		Rect.VSplitLeft(DefaultVMargin, nullptr, &Rect);
		Rect.VSplitRight(maximum(0.0f, DefaultVMargin - ScrollbarWidth), &Rect, nullptr);
	};
	auto RenderFontIcon = [&](const CUIRect &Rect, const char *pIcon, float Size, int Align, ColorRGBA Color) {
		SLabelProperties Props;
		Props.SetColor(Color);
		Props.m_EnableWidthCheck = false;
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		Ui()->DoLabel(&Rect, pIcon, Size, Align, Props);
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	};

	if(m_OpenSettingsSection < 0)
	{
		m_SearchInput.SetEmptyText(RCLocalize(""));
		CUIRect OverviewRect, SearchWrap, SearchRect, Label;
		MainView.HSplitBottom(40.0f * ScreenPixelSize, &OverviewRect, &SearchWrap);
		SearchWrap.Draw(SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_B, DefaultRounding);
		SearchWrap.HMargin(SmallButtonSpace, &SearchWrap);
		SearchWrap.VMargin(DefaultVMargin, &SearchWrap);
		SearchWrap.VSplitLeft(90.0f * ScreenPixelSize, &Label, &SearchWrap);
		SearchWrap.VSplitLeft(SmallVMargin, nullptr, &SearchWrap);
		Ui()->DoLabel(&Label, "Search:", 24.0f * ScreenPixelSize, TEXTALIGN_MC);
		SearchRect = SearchWrap;
		m_SearchRect = SearchRect;
		if(!Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive() && Input()->ModifierIsPressed() && Input()->KeyPress(KEY_F))
		{
			Ui()->SetActiveItem(&m_SearchInput);
			m_SearchInput.SelectAll();
		}
		Ui()->DoEditBox(&m_SearchInput, &SearchRect, 14.0f * ScreenPixelSize);

		const char *pSearch = m_SearchInput.GetString();
		const bool HasSearch = pSearch[0] != '\0';

		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollbarWidth = ScrollbarWidth;
		ScrollParams.m_ScrollUnit = 120.0f;
		ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
		ScrollParams.m_ScrollbarMargin = 5.0f * ScreenPixelSize;
		if(Ui()->MouseHovered(&OverviewRect))
			Ui()->SetHotScrollRegion(&s_OverviewScroll);
		s_OverviewScroll.Begin(&OverviewRect, &s_OverviewOffset, &ScrollParams);

		CUIRect ContentView = OverviewRect;
		ContentView.y += s_OverviewOffset.y;
		ApplyFunctionInsets(ContentView, ScrollParams.m_ScrollbarWidth, DefaultVMargin);

		const float CardGap = DefaultVMargin;
		const float CardWidth = SettingsFunctionWidth;
		const float CardHeight = SettingsFunctionHeight;
		int VisibleCount = 0;

		for(int i = 0; i < gs_NumClickGuiSettingsEntries; ++i)
		{
			const char *pTitle = RCLocalize(gs_aClickGuiSettingsEntries[i].m_pTitle);
			if(HasSearch && !str_utf8_find_nocase(pTitle, pSearch) && !str_utf8_find_nocase(gs_aClickGuiSettingsEntries[i].m_pTitle, pSearch))
				continue;

			const int Row = VisibleCount / 2;
			const int Col = VisibleCount % 2;
			CUIRect Card = {
				ContentView.x + Col * (CardWidth + CardGap),
				ContentView.y + Row * (CardHeight + CardGap),
				CardWidth,
				CardHeight};
			CUIRect OpenRect = Card;
			CUIRect ToggleRect;
			OpenRect.HSplitBottom(34.0f * ScreenPixelSize, &OpenRect, &ToggleRect);
			ToggleRect.Margin(4.0f * ScreenPixelSize, &ToggleRect);

			const bool Selected = i == m_OpenSettingsSection;
			Card.Draw(Selected ? SClickGuiProperties::Hex4E4E4EColor() : SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_ALL, DefaultRounding);
			OpenRect.Margin(10.0f * ScreenPixelSize, &OpenRect);

			CUIRect TitleRect, IconRect;
			OpenRect.HSplitTop(30.0f * ScreenPixelSize, &TitleRect, &IconRect);
			Ui()->DoLabel(&TitleRect, pTitle, 12.0f * ScreenPixelSize, TEXTALIGN_MC);
			RenderFontIcon(IconRect, gs_aClickGuiSettingsEntries[i].m_pIcon, 32.0f * ScreenPixelSize, TEXTALIGN_MC, ColorRGBA(1.0f, 1.0f, 1.0f, 0.9f));

			if(Ui()->DoButtonLogic(&s_aOpenButtons[i], 0, &OpenRect, BUTTONFLAG_LEFT))
				m_OpenSettingsSection = i;

			if(int *pMainToggle = gs_aClickGuiSettingsEntries[i].m_pMainToggle)
			{
				const ColorRGBA ToggleColor = *pMainToggle ? ColorRGBA(0.18f, 0.45f, 0.24f, 0.9f) : SClickGuiProperties::Hex141414Color();
				if(GameClient()->m_Menus.DoButton_Menu(&s_aToggleButtons[i], *pMainToggle ? RCLocalize("Enabled") : RCLocalize("Disabled"), 0, &ToggleRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, DefaultRounding * 0.75f, 0.0f, ToggleColor))
					*pMainToggle ^= 1;
			}
			else
			{
				Ui()->DoLabel(&ToggleRect, RCLocalize("No main toggle"), 10.0f * ScreenPixelSize, TEXTALIGN_MC);
			}

			++VisibleCount;
		}

		if(VisibleCount == 0)
		{
			CUIRect EmptyState = OverviewRect;
			EmptyState.Margin(DefaultVMargin, &EmptyState);
			Ui()->DoLabel(&EmptyState, RCLocalize("No matches"), 16.0f * ScreenPixelSize, TEXTALIGN_MC);
		}

		CUIRect ScrollRegionRect = OverviewRect;
		ScrollRegionRect.y = ContentView.y + ((VisibleCount + 1) / 2) * (CardHeight + CardGap);
		ScrollRegionRect.h = 0.0f;
		s_OverviewScroll.AddRect(ScrollRegionRect);
		s_OverviewScroll.End();
		return;
	}

	m_SearchRect = std::nullopt;
	const SClickGuiSettingsEntry &Entry = gs_aClickGuiSettingsEntries[m_OpenSettingsSection];
	CUIRect TopBar, BackRect, TitleRect, DetailRect;
	MainView.HSplitTop(40.0f * ScreenPixelSize, &TopBar, &MainView);
	TopBar.Draw(SClickGuiProperties::Hex1E1E1EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	TopBar.HMargin(SmallButtonSpace, &TopBar);
	TopBar.VMargin(ButtonSpace, &TopBar);
	TopBar.VSplitLeft(120.0f * ScreenPixelSize, &BackRect, &TitleRect);
	if(GameClient()->m_Menus.DoButton_Menu(&s_BackButton, RCLocalize("Back"), 0, &BackRect))
	{
		m_OpenSettingsSection = -1;
		return;
	}
	Ui()->DoLabel(&TopBar, RCLocalize(Entry.m_pTitle), 16.0f * ScreenPixelSize, TEXTALIGN_MC);
	MainView.HSplitTop(SmallVMargin, nullptr, &MainView);
	DetailRect = MainView;
	if(Ui()->MouseHovered(&DetailRect))
		Ui()->SetHotScrollRegion(&s_aDetailScroll[Entry.m_Section]);

	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollbarWidth = ScrollbarWidth;
	ScrollParams.m_ScrollUnit = 120.0f;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_ScrollbarMargin = 5.0f * ScreenPixelSize;
	s_aDetailScroll[Entry.m_Section].Begin(&DetailRect, &s_aDetailOffsets[Entry.m_Section], &ScrollParams);

	CUIRect ContentView = DetailRect;
	ContentView.y += s_aDetailOffsets[Entry.m_Section].y;
	ApplyFunctionInsets(ContentView, ScrollParams.m_ScrollbarWidth, 5.0f * ScreenPixelSize);
	GameClient()->m_Menus.RenderRushieSettingsSection(ContentView, Entry.m_Section);

	CUIRect ScrollRegionRect = DetailRect;
	ScrollRegionRect.y = ContentView.y + SmallVMargin;
	ScrollRegionRect.h = 0.0f;
	s_aDetailScroll[Entry.m_Section].AddRect(ScrollRegionRect);
	s_aDetailScroll[Entry.m_Section].End();
}

void CMenusRClientClickGui::RenderClickGuiRushieVoice(CUIRect MainView, float ScreenPixelSize)
{

}

void CMenusRClientClickGui::RenderClickGuiRushieInfo(CUIRect MainView, float ScreenPixelSize)
{

}
