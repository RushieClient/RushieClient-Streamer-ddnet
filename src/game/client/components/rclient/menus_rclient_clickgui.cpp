#include "menus_rclient_clickgui.h"

#include "base/str.h"
#include "engine/font_icons.h"
#include "engine/shared/config.h"
#include "game/client/animstate.h"
#include "game/client/gameclient.h"
#include "game/client/render.h"
#include "game/client/ui_listbox.h"
#include "generated/client_data.h"

#include <engine/graphics.h>

#include <game/client/components/chat.h>
#include <game/client/components/console.h>
#include <game/client/components/emoticon.h>
#include <game/client/components/menus.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

enum
{
	CLICKGUI_TAB_SETTINGS = 0,
	CLICKGUI_TAB_VOICE,
	CLICKGUI_TAB_INFO,
	NUM_CLICKGUI_TABS
};

static bool gs_EditProfilesOpen = false;

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

	if(gs_EditProfilesOpen)
	{
		gs_EditProfilesOpen = false;
		return true;
	}

	SetActive(false);
	return true;
}

void CMenusRClientClickGui::ResetTransientState()
{
	m_OpenSettingsSection = -1;
	gs_EditProfilesOpen = false;
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
	Ui()->DoLabel(&TopBarContent, Client()->PlayerName(), 24.0f * PixelSize, TEXTALIGN_ML);

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
	CUIRect ProfilesList, EditProfilesBTN;
	static CButtonContainer s_EditProfilesButton;
	static std::vector<CButtonContainer> s_vProfileApplyButtons;
	static CScrollRegion s_ProfilesScroll;
	static vec2 s_ProfilesOffset(0.0f, 0.0f);
	const std::vector<CRushieSettingsProfile> &vProfiles = GameClient()->m_RushieSettingsProfiles.m_vProfiles;
	if(s_vProfileApplyButtons.size() < vProfiles.size())
		s_vProfileApplyButtons.resize(vProfiles.size());

	SettingsProfiles.Draw(SClickGuiProperties::Hex1E1E1EColor(), IGraphics::CORNER_R, DefaultRounding);
	SettingsProfiles.HSplitBottom(BarSizeHeight, &ProfilesList, &EditProfilesBTN);
	EditProfilesBTN.Draw(SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_BR, DefaultRounding);
	EditProfilesBTN.HMargin(SmallButtonSpace, &EditProfilesBTN);
	EditProfilesBTN.VMargin(ButtonSpace, &EditProfilesBTN);
	EditProfilesBTN.Draw(SClickGuiProperties::Hex4E4E4EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	if(GameClient()->m_Menus.DoButton_Menu(&s_EditProfilesButton, "EDIT PROFILES", gs_EditProfilesOpen, &EditProfilesBTN))
		gs_EditProfilesOpen = !gs_EditProfilesOpen;

	ProfilesList.Margin(SmallButtonSpace, &ProfilesList);
	if(Ui()->MouseHovered(&ProfilesList))
		Ui()->SetHotScrollRegion(&s_ProfilesScroll);

	CScrollRegionParams ProfileScrollParams;
	ProfileScrollParams.m_ScrollbarWidth = 14.0f * PixelSize;
	ProfileScrollParams.m_ScrollUnit = 120.0f;
	ProfileScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ProfileScrollParams.m_ScrollbarMargin = 2.0f * PixelSize;
	s_ProfilesScroll.Begin(&ProfilesList, &s_ProfilesOffset, &ProfileScrollParams);

	CUIRect ProfileContent = ProfilesList;
	ProfileContent.y += s_ProfilesOffset.y;
	const float ProfileRowHeight = 38.0f * PixelSize;
	const float ProfileRowGap = 6.0f * PixelSize;
	if(vProfiles.empty())
	{
		CUIRect EmptyState = ProfilesList;
		EmptyState.Margin(8.0f * PixelSize, &EmptyState);
		Ui()->DoLabel(&EmptyState, "NO PROFILES", 12.0f * PixelSize, TEXTALIGN_MC);
	}
	for(size_t i = 0; i < vProfiles.size(); ++i)
	{
		CUIRect Row = {ProfileContent.x, ProfileContent.y + i * (ProfileRowHeight + ProfileRowGap), ProfileContent.w, ProfileRowHeight};
		CUIRect NameRect, ApplyRect;
		Row.Draw(SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_ALL, DefaultRounding * 0.6f);
		Row.Margin(4.0f * PixelSize, &Row);
		Row.VSplitRight(72.0f * PixelSize, &NameRect, &ApplyRect);
		NameRect.VSplitRight(SmallVMargin, &NameRect, nullptr);
		Ui()->DoLabel(&NameRect, vProfiles[i].m_Name.c_str(), 12.0f * PixelSize, TEXTALIGN_ML);
		if(GameClient()->m_Menus.DoButton_Menu(&s_vProfileApplyButtons[i], "APPLY", 0, &ApplyRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, DefaultRounding * 0.5f, 0.0f, SClickGuiProperties::Hex4E4E4EColor()))
			GameClient()->m_RushieSettingsProfiles.ApplyProfile(vProfiles[i]);
	}

	CUIRect ProfileScrollRect = ProfilesList;
	ProfileScrollRect.y = ProfileContent.y + vProfiles.size() * (ProfileRowHeight + ProfileRowGap);
	ProfileScrollRect.h = 0.0f;
	s_ProfilesScroll.AddRect(ProfileScrollRect);
	s_ProfilesScroll.End();
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

	auto RenderProfilesEditor = [&](CUIRect MainView) {
		static int s_SelectedProfile = -1;
		static int s_LastSelectedProfile = -2;
		static int s_IncludeDdnet = 1;
		static int s_IncludeBinds = 1;
		static int s_IncludeTClient = 1;
		static int s_IncludeTClientBindWheel = 1;
		static int s_IncludeRClient = 1;
		static int s_IncludeRClientBindWheel = 1;
		static int s_IncludeWarlist = 1;
		static int s_IncludeChatbinds = 1;
		static int s_IncludeSkinProfiles = 1;
		static CLineInputBuffered<64> s_ProfileNameInput;
		static CButtonContainer s_BackButton;
		static CButtonContainer s_SaveButton;
		static CButtonContainer s_ApplyButton;
		static CButtonContainer s_OverrideButton;
		static CButtonContainer s_FileButton;
		static CButtonContainer s_DeleteButton;
		static CListBox s_ListBox;
		static bool s_aProfileRows[1024];
		static std::vector<CButtonContainer> s_vApplyButtons;

		const float HeadlineHeight = 26.0f * PixelSize;
		const float HeadlineFontSize = 15.0f * PixelSize;
		const float FontSize = 12.0f * PixelSize;
		const float EditBoxFontSize = 12.0f * PixelSize;
		const float LineSize = 22.0f * PixelSize;
		const float MarginSmall = 6.0f * PixelSize;
		const float MarginExtraSmall = 3.0f * PixelSize;
		const float MarginBetweenViews = 12.0f * PixelSize;

		s_ProfileNameInput.SetEmptyText(RCLocalize("Profile name"));

		const std::vector<CRushieSettingsProfile> &vLocalProfiles = GameClient()->m_RushieSettingsProfiles.m_vProfiles;
		if(vLocalProfiles.empty())
			s_SelectedProfile = -1;
		else if(s_SelectedProfile >= (int)vLocalProfiles.size())
			s_SelectedProfile = (int)vLocalProfiles.size() - 1;

		if(s_SelectedProfile != s_LastSelectedProfile && s_SelectedProfile >= 0 && s_SelectedProfile < (int)vLocalProfiles.size())
			s_ProfileNameInput.Set(vLocalProfiles[s_SelectedProfile].m_Name.c_str());
		s_LastSelectedProfile = s_SelectedProfile;

		auto SetSelection = [&](int Index) {
			s_SelectedProfile = Index;
			if(Index >= 0 && Index < (int)vLocalProfiles.size())
				s_ProfileNameInput.Set(vLocalProfiles[Index].m_Name.c_str());
			else
				s_ProfileNameInput.Clear();
		};

		auto GetNewProfileName = [&]() {
			const char *pInputName = s_ProfileNameInput.GetString();
			if(pInputName[0] != '\0')
				return GameClient()->m_RushieSettingsProfiles.MakeUniqueProfileName(pInputName);
			return GameClient()->m_RushieSettingsProfiles.MakeUniqueProfileName("Rushie Profile");
		};

		auto GetOverrideProfileName = [&]() {
			const char *pInputName = s_ProfileNameInput.GetString();
			if(pInputName[0] != '\0')
				return GameClient()->m_RushieSettingsProfiles.MakeUniqueProfileName(pInputName, s_SelectedProfile);
			return GameClient()->m_RushieSettingsProfiles.MakeUniqueProfileName("Rushie Profile", s_SelectedProfile);
		};

		const CRushieSettingsProfile CurrentProfile = GameClient()->m_RushieSettingsProfiles.CaptureProfile(
			"",
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true);

		auto RenderProfileStats = [&](const CRushieSettingsProfile &Profile, CUIRect Rect) {
			char aBuf[256];
			auto GetStat = [&](int Source, char *pBuf, int Size) {
				if(!Profile.HasSource(Source))
					str_copy(pBuf, "-", Size);
				else
					str_format(pBuf, Size, "%d", Profile.CountForSource(Source));
			};
			char aDdnet[16], aBinds[16], aTclient[16], aRclient[16], aTWheel[16], aRWheel[16], aWarlist[16], aChatbinds[16], aSkinProfiles[16];
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_DDNET, aDdnet, sizeof(aDdnet));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_BINDS, aBinds, sizeof(aBinds));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_TCLIENT, aTclient, sizeof(aTclient));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_RCLIENT, aRclient, sizeof(aRclient));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_TCLIENT_BINDWHEEL, aTWheel, sizeof(aTWheel));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_RCLIENT_BINDWHEEL, aRWheel, sizeof(aRWheel));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_WARLIST, aWarlist, sizeof(aWarlist));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_CHATBINDS, aChatbinds, sizeof(aChatbinds));
			GetStat(RUSHIESETTINGSPROFILE_SOURCE_SKINPROFILES, aSkinProfiles, sizeof(aSkinProfiles));

			CUIRect Row;
			Rect.HSplitTop(LineSize, &Row, &Rect);
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Saved settings"), (int)Profile.m_vEntries.size());
			Ui()->DoLabel(&Row, aBuf, FontSize, TEXTALIGN_ML);

			Rect.HSplitTop(LineSize, &Row, &Rect);
			str_format(aBuf, sizeof(aBuf), "DDNet: %s   Binds: %s   TClient: %s   RClient: %s", aDdnet, aBinds, aTclient, aRclient);
			Ui()->DoLabel(&Row, aBuf, FontSize, TEXTALIGN_ML);

			Rect.HSplitTop(LineSize, &Row, &Rect);
			str_format(aBuf, sizeof(aBuf), "TWheel: %s   RWheel: %s   Warlist: %s   Chat binds: %s   Skin profiles: %s", aTWheel, aRWheel, aWarlist, aChatbinds, aSkinProfiles);
			Ui()->DoLabel(&Row, aBuf, FontSize, TEXTALIGN_ML);
		};

		CUIRect HeaderBar, HeaderLeft, HeaderRight, Content;
		MainView.HSplitTop(40.0f * PixelSize, &HeaderBar, &Content);
		HeaderBar.Draw(SClickGuiProperties::Hex2A2A2AColor(), IGraphics::CORNER_T, DefaultRounding);
		HeaderBar.HMargin(SmallButtonSpace, &HeaderBar);
		HeaderBar.VMargin(ButtonSpace, &HeaderBar);
		HeaderBar.VSplitLeft(120.0f * PixelSize, &HeaderLeft, &HeaderRight);
		if(GameClient()->m_Menus.DoButton_Menu(&s_BackButton, RCLocalize("Back"), 0, &HeaderLeft))
		{
			gs_EditProfilesOpen = false;
			return;
		}
		Ui()->DoLabel(&HeaderRight, "EDIT PROFILES", 16.0f * PixelSize, TEXTALIGN_MC);

		Content.Margin(DefaultVMargin * 0.5f, &Content);

		CUIRect TopBar, BottomArea, Label, Button;
		Content.HSplitTop(LineSize * 13.5f, &TopBar, &BottomArea);

		CUIRect InfoArea, ActionArea;
		TopBar.VSplitMid(&InfoArea, &ActionArea, MarginBetweenViews);

		CUIRect CurrentRect;
		InfoArea.HSplitTop(HeadlineHeight, &Label, &InfoArea);
		Ui()->DoLabel(&Label, RCLocalize("Current settings"), HeadlineFontSize, TEXTALIGN_ML);
		InfoArea.HSplitTop(MarginSmall, nullptr, &InfoArea);
		InfoArea.HSplitTop(LineSize * 3.0f, &CurrentRect, &InfoArea);
		RenderProfileStats(CurrentProfile, CurrentRect);
		InfoArea.HSplitTop(MarginSmall, nullptr, &InfoArea);

		if(s_SelectedProfile >= 0 && s_SelectedProfile < (int)vLocalProfiles.size())
		{
			CUIRect SelectedRect;
			InfoArea.HSplitTop(LineSize, nullptr, &InfoArea);
			InfoArea.HSplitTop(HeadlineHeight, &Label, &InfoArea);
			Ui()->DoLabel(&Label, RCLocalize("Selected profile"), HeadlineFontSize, TEXTALIGN_ML);
			InfoArea.HSplitTop(MarginSmall, nullptr, &InfoArea);
			InfoArea.HSplitTop(LineSize, &Label, &InfoArea);
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Selected"), vLocalProfiles[s_SelectedProfile].m_Name.c_str());
			Ui()->DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);
			InfoArea.HSplitTop(MarginExtraSmall, nullptr, &InfoArea);
			InfoArea.HSplitTop(LineSize * 3.0f, &SelectedRect, &InfoArea);
			RenderProfileStats(vLocalProfiles[s_SelectedProfile], SelectedRect);
		}
		else
		{
			InfoArea.HSplitTop(LineSize, nullptr, &InfoArea);
			InfoArea.HSplitTop(HeadlineHeight, &Label, &InfoArea);
			Ui()->DoLabel(&Label, RCLocalize("No profile selected"), HeadlineFontSize, TEXTALIGN_ML);
		}

		CUIRect Actions = ActionArea;
		CUIRect ToggleArea, ToggleLeft, ToggleRight;
		Actions.HSplitTop(HeadlineHeight, &Label, &Actions);
		Ui()->DoLabel(&Label, RCLocalize("Create or update"), HeadlineFontSize, TEXTALIGN_ML);
		Actions.HSplitTop(MarginSmall, nullptr, &Actions);

		Actions.HSplitTop(LineSize, &Button, &Actions);
		Ui()->DoEditBox(&s_ProfileNameInput, &Button, EditBoxFontSize);
		Actions.HSplitTop(MarginSmall, nullptr, &Actions);

		Actions.HSplitTop(LineSize * 5.5f, &ToggleArea, &Actions);
		ToggleArea.VSplitMid(&ToggleLeft, &ToggleRight, MarginSmall);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeDdnet, Localize("Include DDNet settings"), &s_IncludeDdnet, &ToggleLeft, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeBinds, Localize("Include key binds"), &s_IncludeBinds, &ToggleLeft, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeTClient, Localize("Include TClient settings"), &s_IncludeTClient, &ToggleLeft, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeTClientBindWheel, Localize("Include TClient bindwheel"), &s_IncludeTClientBindWheel, &ToggleLeft, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeRClient, Localize("Include RClient settings"), &s_IncludeRClient, &ToggleLeft, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeRClientBindWheel, Localize("Include Rushie bindwheel"), &s_IncludeRClientBindWheel, &ToggleRight, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeWarlist, Localize("Include warlist"), &s_IncludeWarlist, &ToggleRight, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeChatbinds, Localize("Include chat binds"), &s_IncludeChatbinds, &ToggleRight, LineSize);
		GameClient()->m_Menus.DoButton_CheckBoxAutoVMarginAndSet(&s_IncludeSkinProfiles, Localize("Include skin profiles"), &s_IncludeSkinProfiles, &ToggleRight, LineSize);
		Actions.HSplitTop(MarginSmall, nullptr, &Actions);

		CUIRect ButtonRowLeft, ButtonRowRight;
		Actions.HSplitTop(LineSize * 1.5f, &Button, &Actions);
		Button.VSplitMid(&ButtonRowLeft, &ButtonRowRight, MarginSmall);
		if(GameClient()->m_Menus.DoButton_Menu(&s_SaveButton, RCLocalize("Save New"), 0, &ButtonRowLeft))
		{
			const std::string ProfileName = GetNewProfileName();
			GameClient()->m_RushieSettingsProfiles.SaveProfile(ProfileName.c_str(), s_IncludeDdnet != 0, s_IncludeBinds != 0, s_IncludeTClient != 0, s_IncludeTClientBindWheel != 0, s_IncludeRClient != 0, s_IncludeRClientBindWheel != 0, s_IncludeWarlist != 0, s_IncludeChatbinds != 0, s_IncludeSkinProfiles != 0);
			SetSelection((int)GameClient()->m_RushieSettingsProfiles.m_vProfiles.size() - 1);
		}
		if(GameClient()->m_Menus.DoButton_Menu(&s_ApplyButton, RCLocalize("Apply Selected"), 0, &ButtonRowRight) && s_SelectedProfile >= 0 && s_SelectedProfile < (int)vLocalProfiles.size())
			GameClient()->m_RushieSettingsProfiles.ApplyProfile(vLocalProfiles[s_SelectedProfile]);
		Actions.HSplitTop(MarginExtraSmall, nullptr, &Actions);

		Actions.HSplitTop(LineSize * 1.5f, &Button, &Actions);
		Button.VSplitMid(&ButtonRowLeft, &ButtonRowRight, MarginSmall);
		if(GameClient()->m_Menus.DoButton_Menu(&s_OverrideButton, RCLocalize("Override Selected"), 0, &ButtonRowLeft) && s_SelectedProfile >= 0 && s_SelectedProfile < (int)vLocalProfiles.size())
		{
			const std::string ProfileName = GetOverrideProfileName();
			GameClient()->m_RushieSettingsProfiles.OverrideProfile(s_SelectedProfile, ProfileName.c_str(), s_IncludeDdnet != 0, s_IncludeBinds != 0, s_IncludeTClient != 0, s_IncludeTClientBindWheel != 0, s_IncludeRClient != 0, s_IncludeRClientBindWheel != 0, s_IncludeWarlist != 0, s_IncludeChatbinds != 0, s_IncludeSkinProfiles != 0);
			SetSelection(s_SelectedProfile);
		}
		if(GameClient()->m_Menus.DoButton_Menu(&s_FileButton, RCLocalize("Profiles file"), 0, &ButtonRowRight))
		{
			char aBuf[IO_MAX_PATH_LENGTH];
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, s_aConfigDomains[ConfigDomain::RCLIENTSETTINGSPROFILES].m_aConfigPath, aBuf, sizeof(aBuf));
			Client()->ViewFile(aBuf);
		}
		Actions.HSplitTop(MarginExtraSmall, nullptr, &Actions);

		Actions.HSplitTop(LineSize * 1.5f, &Button, &Actions);
		if(GameClient()->m_Menus.DoButton_Menu(&s_DeleteButton, RCLocalize("Delete Selected"), 0, &Button) && s_SelectedProfile >= 0 && s_SelectedProfile < (int)vLocalProfiles.size())
		{
			GameClient()->m_RushieSettingsProfiles.m_vProfiles.erase(GameClient()->m_RushieSettingsProfiles.m_vProfiles.begin() + s_SelectedProfile);
			if(GameClient()->m_RushieSettingsProfiles.m_vProfiles.empty())
				SetSelection(-1);
			else if(s_SelectedProfile >= (int)GameClient()->m_RushieSettingsProfiles.m_vProfiles.size())
				SetSelection((int)GameClient()->m_RushieSettingsProfiles.m_vProfiles.size() - 1);
			else
				SetSelection(s_SelectedProfile);
		}

		BottomArea.HSplitTop(MarginSmall, nullptr, &BottomArea);

		if(s_vApplyButtons.size() < vLocalProfiles.size())
			s_vApplyButtons.resize(vLocalProfiles.size());

		s_ListBox.DoStart(52.0f * PixelSize, vLocalProfiles.size(), BottomArea.w / 240.0f, 1, s_SelectedProfile, &BottomArea, true, IGraphics::CORNER_ALL, true);
		for(size_t i = 0; i < vLocalProfiles.size(); i++)
		{
			CListboxItem Item = s_ListBox.DoNextItem(&s_aProfileRows[i], s_SelectedProfile >= 0 && (size_t)s_SelectedProfile == i);
			if(!Item.m_Visible)
				continue;

			CUIRect Row = Item.m_Rect;
			Row.HMargin(MarginExtraSmall, &Row);
			Row.VMargin(MarginSmall, &Row);
			CUIRect InfoRect, ApplyRect;
			Row.VSplitRight(110.0f * PixelSize, &InfoRect, &ApplyRect);
			InfoRect.VSplitRight(MarginSmall * 2.0f, &InfoRect, nullptr);
			ApplyRect.VSplitLeft(MarginSmall * 2.0f, nullptr, &ApplyRect);
			ApplyRect.HMargin(8.0f * PixelSize, &ApplyRect);
			if(GameClient()->m_Menus.DoButton_Menu(&s_vApplyButtons[i], RCLocalize("Apply"), 0, &ApplyRect))
			{
				GameClient()->m_RushieSettingsProfiles.ApplyProfile(vLocalProfiles[i]);
				SetSelection((int)i);
			}

			InfoRect.HMargin(6.0f * PixelSize, &InfoRect);
			InfoRect.VSplitLeft(MarginSmall, nullptr, &InfoRect);

			CUIRect NameRect, StatsRect;
			InfoRect.HSplitTop(LineSize, &NameRect, &StatsRect);
			Ui()->DoLabel(&NameRect, vLocalProfiles[i].m_Name.c_str(), FontSize, TEXTALIGN_ML);

			char aBuf[256];
			auto GetStatCompact = [&](int Source, char *pBuf, int Size) {
				if(!vLocalProfiles[i].HasSource(Source))
					str_copy(pBuf, "-", Size);
				else
					str_format(pBuf, Size, "%d", vLocalProfiles[i].CountForSource(Source));
			};
			char aDdnet[16], aBinds[16], aTclient[16], aRclient[16], aTWheel[16], aRWheel[16];
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_DDNET, aDdnet, sizeof(aDdnet));
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_BINDS, aBinds, sizeof(aBinds));
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_TCLIENT, aTclient, sizeof(aTclient));
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_RCLIENT, aRclient, sizeof(aRclient));
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_TCLIENT_BINDWHEEL, aTWheel, sizeof(aTWheel));
			GetStatCompact(RUSHIESETTINGSPROFILE_SOURCE_RCLIENT_BINDWHEEL, aRWheel, sizeof(aRWheel));
			const bool HasExtras = vLocalProfiles[i].HasSource(RUSHIESETTINGSPROFILE_SOURCE_WARLIST) || vLocalProfiles[i].HasSource(RUSHIESETTINGSPROFILE_SOURCE_CHATBINDS) || vLocalProfiles[i].HasSource(RUSHIESETTINGSPROFILE_SOURCE_SKINPROFILES);
			char aExtras[16];
			if(!HasExtras)
				str_copy(aExtras, "-", sizeof(aExtras));
			else
				str_format(aExtras, sizeof(aExtras), "%d", vLocalProfiles[i].CountForSource(RUSHIESETTINGSPROFILE_SOURCE_WARLIST) + vLocalProfiles[i].CountForSource(RUSHIESETTINGSPROFILE_SOURCE_CHATBINDS) + vLocalProfiles[i].CountForSource(RUSHIESETTINGSPROFILE_SOURCE_SKINPROFILES));
			str_format(aBuf, sizeof(aBuf), "D:%s B:%s T:%s R:%s TW:%s RW:%s X:%s", aDdnet, aBinds, aTclient, aRclient, aTWheel, aRWheel, aExtras);
			Ui()->DoLabel(&StatsRect, aBuf, EditBoxFontSize, TEXTALIGN_ML);
		}

		const int ListSelection = s_ListBox.DoEnd();
		if(vLocalProfiles.empty())
		{
			if(s_SelectedProfile != -1)
				SetSelection(-1);
		}
		else if(ListSelection != s_SelectedProfile)
		{
			SetSelection(ListSelection);
		}
	};

	//Main render
	Body.Draw(SClickGuiProperties::Hex1E1E1EColor(), IGraphics::CORNER_ALL, DefaultRounding);
	if(gs_EditProfilesOpen)
	{
		RenderProfilesEditor(Body);
	}
	else
	{
		if(m_CurrentTab == CLICKGUI_TAB_SETTINGS)
			RenderClickGuiRushieSettings(Body, PixelSize);
		if(m_CurrentTab == CLICKGUI_TAB_VOICE)
			RenderClickGuiRushieVoice(Body, PixelSize);
		if(m_CurrentTab == CLICKGUI_TAB_INFO)
			RenderClickGuiRushieInfo(Body, PixelSize);
	}

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

	static CButtonContainer s_aOpenButtons[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	static CButtonContainer s_aToggleButtons[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	static CButtonContainer s_BackButton;
	static CScrollRegion s_OverviewScroll;
	static vec2 s_OverviewOffset(0.0f, 0.0f);
	static CScrollRegion s_aDetailScroll[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	static vec2 s_aDetailOffsets[CMenus::NUM_RUSHIE_SETTINGS_SECTIONS];
	const CMenus::SRushieSettingsSectionEntry *pEntries = CMenus::GetRushieSettingsSectionEntries();
	const int NumEntries = CMenus::GetNumRushieSettingsSections();
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

		for(int i = 0; i < NumEntries; ++i)
		{
			const CMenus::SRushieSettingsSectionEntry &Entry = pEntries[i];
			const char *pTitle = RCLocalize(Entry.m_pTitle, Entry.m_pTitleContext);
			if(HasSearch && !str_utf8_find_nocase(pTitle, pSearch) && !str_utf8_find_nocase(Entry.m_pTitle, pSearch))
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
			RenderFontIcon(IconRect, Entry.m_pIcon, 32.0f * ScreenPixelSize, TEXTALIGN_MC, ColorRGBA(1.0f, 1.0f, 1.0f, 0.9f));

			if(Ui()->DoButtonLogic(&s_aOpenButtons[i], 0, &OpenRect, BUTTONFLAG_LEFT))
				m_OpenSettingsSection = i;

			if(Entry.HasMainToggle())
			{
				const bool Enabled = Entry.IsMainToggleEnabled();
				const ColorRGBA ToggleColor = Enabled ? ColorRGBA(0.18f, 0.45f, 0.24f, 0.9f) : SClickGuiProperties::Hex141414Color();
				if(GameClient()->m_Menus.DoButton_Menu(&s_aToggleButtons[i], Enabled ? RCLocalize("Enabled") : RCLocalize("Disabled"), 0, &ToggleRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, DefaultRounding * 0.75f, 0.0f, ToggleColor))
					Entry.SetMainToggleEnabled(!Enabled);
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
	const CMenus::SRushieSettingsSectionEntry &Entry = pEntries[m_OpenSettingsSection];
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
	Ui()->DoLabel(&TopBar, RCLocalize(Entry.m_pTitle, Entry.m_pTitleContext), 16.0f * ScreenPixelSize, TEXTALIGN_MC);
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
