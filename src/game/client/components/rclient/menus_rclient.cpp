#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/localization.h>
#include <generated/protocol.h>

#include <game/client/components/menu_background.h>
#include <game/client/components/menus.h>
#include <game/client/components/rclient/bindwheel.h>
#include <game/client/components/tclient/bindchat.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>

#include <vector>
enum
{
	RCLIENT_TAB_SETTINGS = 0,
	RCLIENT_TAB_BINDWHEEL,
	RCLIENT_TAB_NAMEPLATES_EDITOR,
	RCLIENT_TAB_RCON,
	RCLIENT_TAB_INFO,
	NUMBER_OF_RUSHIE_TABS
};

using namespace FontIcons;

static float s_Time = 0.0f;
static bool s_StartedTime = false;

const float FontSize = 14.0f;
const float EditBoxFontSize = 12.0f;
const float LineSize = 20.0f;
const float ColorPickerLineSize = 25.0f;
const float HeadlineFontSize = 20.0f;

const float HeadlineHeight = HeadlineFontSize + 0.0f;
const float MarginSmall = 5.0f;
const float MarginExtraSmall = 2.5f;
const float MarginBetweenSections = 30.0f;
const float MarginBetweenViews = 30.0f;

const float ColorPickerLabelSize = 13.0f;
const float ColorPickerLineSpacing = 5.0f;

static void SetFlag(int32_t &Flags, int n, bool Value)
{
	if(Value)
		Flags |= (1 << n);
	else
		Flags &= ~(1 << n);
}

static bool IsFlagSet(int32_t Flags, int n)
{
	return (Flags & (1 << n)) != 0;
}

void CMenus::RenderSettingsRushie(CUIRect MainView)
{
	s_Time += Client()->RenderFrameTime() * (1.0f / 100.0f);
	if(!s_StartedTime)
	{
		s_StartedTime = true;
		s_Time = (float)rand() / (float)RAND_MAX;
	}

	if(Client()->RconAuthed())
	{
		SetFlag(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_RCON, 0);
	}
	else
	{
		SetFlag(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_RCON, 1);
	}

	static int s_CurCustomTab = 0;

	CUIRect TabBar, Button;
	int TabCount = NUMBER_OF_RUSHIE_TABS;
	for(int Tab = 0; Tab < NUMBER_OF_RUSHIE_TABS; ++Tab)
	{
		if(IsFlagSet(g_Config.m_RiRClientSettingsTabs, Tab))
		{
			TabCount--;
			if(s_CurCustomTab == Tab)
				s_CurCustomTab++;
		}
	}

	MainView.HSplitTop(LineSize * 1.2f, &TabBar, &MainView);
	const float TabWidth = TabBar.w / TabCount;
	static CButtonContainer s_aPageTabs[NUMBER_OF_RUSHIE_TABS] = {};
	const char *apTabNames[] = {
		RCLocalize("Settings"),
		RCLocalize("Bindwheel in spec"),
		RCLocalize("Nameplate editor"),
		RCLocalize("RCON"),
		RCLocalize("Info")};

	for(int Tab = 0; Tab < NUMBER_OF_RUSHIE_TABS; ++Tab)
	{
		if(IsFlagSet(g_Config.m_RiRClientSettingsTabs, Tab))
			continue;

		TabBar.VSplitLeft(TabWidth, &Button, &TabBar);
		const int Corners = Tab == 0 ? IGraphics::CORNER_L : Tab == NUMBER_OF_RUSHIE_TABS - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE;
		if(DoButton_MenuTab(&s_aPageTabs[Tab], apTabNames[Tab], s_CurCustomTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			s_CurCustomTab = Tab;
	}

	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	if(s_CurCustomTab == RCLIENT_TAB_SETTINGS)
	{
		RenderSettingsRushieSettings(MainView);
	}

	if(s_CurCustomTab == RCLIENT_TAB_BINDWHEEL)
	{
		RenderSettingsRushieBindWheelSpec(MainView);
	}

	if(s_CurCustomTab == RCLIENT_TAB_NAMEPLATES_EDITOR)
	{
		RenderSettingsRushieNameplatesEditor(MainView);
	}
	if(s_CurCustomTab == RCLIENT_TAB_RCON)
	{
		RenderSettingsRushieRCON(MainView);
	}
	if(s_CurCustomTab == RCLIENT_TAB_INFO)
	{
		RenderSettingsRushieInfo(MainView);
	}
}
void CMenus::RenderSettingsRushieInfo(CUIRect MainView)
{
	CUIRect LeftView, RightView, Button, Label, LowerLeftView;
	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	MainView.VSplitMid(&LeftView, &RightView, MarginBetweenViews);
	LeftView.VSplitLeft(MarginSmall, nullptr, &LeftView);
	RightView.VSplitRight(MarginSmall, &RightView, nullptr);
	LeftView.HSplitMid(&LeftView, &LowerLeftView, 0.0f);

	LeftView.HSplitTop(HeadlineHeight, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("Rushie Client Links"), HeadlineFontSize, TEXTALIGN_ML);
	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);

	static CButtonContainer s_DiscordButton, s_WebsiteButton;
	CUIRect ButtonLeft, ButtonRight;

	LeftView.HSplitTop(LineSize * 2.0f, &Button, &LeftView);
	Button.VSplitMid(&ButtonLeft, &ButtonRight, MarginSmall);
	if(DoButtonLineSize_Menu(&s_DiscordButton, RCLocalize("Discord"), 0, &ButtonLeft, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		Client()->ViewLink("https://discord.gg/wUFTVAGVGa");
	if(DoButtonLineSize_Menu(&s_WebsiteButton, RCLocalize("Website"), 0, &ButtonRight, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		Client()->ViewLink(CRClient::RCLIENT_URL);

	LeftView = LowerLeftView;
	LeftView.HSplitBottom(LineSize * 4.0f + MarginSmall * 2.0f + HeadlineFontSize, nullptr, &LeftView);
	LeftView.HSplitTop(HeadlineHeight, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("Config Files"), HeadlineFontSize, TEXTALIGN_ML);
	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);

	char aBuf[128 + IO_MAX_PATH_LENGTH];
	CUIRect TClientConfig, ProfilesFile;

	LeftView.HSplitTop(LineSize * 2.0f, &Button, &LeftView);
	Button.VSplitMid(&TClientConfig, &ProfilesFile, MarginSmall);

	static CButtonContainer s_Config;
	if(DoButtonLineSize_Menu(&s_Config, RCLocalize("RClient Settings"), 0, &TClientConfig, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, s_aConfigDomains[ConfigDomain::RCLIENT].m_aConfigPath, aBuf, sizeof(aBuf));
		Client()->ViewFile(aBuf);
	}

	// =======RIGHT VIEW========

	RightView.HSplitTop(HeadlineHeight, &Label, &RightView);
	Ui()->DoLabel(&Label, RCLocalize("RClient Developer"), HeadlineFontSize, TEXTALIGN_ML);
	RightView.HSplitTop(MarginSmall, nullptr, &RightView);
	RightView.HSplitTop(MarginSmall, nullptr, &RightView);

	const float TeeSize = 50.0f;
	const float CardSize = TeeSize + MarginSmall;
	CUIRect TeeRect, DevCardRect;
	{
		RightView.HSplitTop(CardSize, &DevCardRect, &RightView);
		DevCardRect.VSplitLeft(CardSize, &TeeRect, &Label);
		Label.VSplitLeft(TextRender()->TextWidth(LineSize, "Voix"), &Label, &Button);
		Button.VSplitLeft(MarginSmall, nullptr, &Button);
		Button.w = LineSize, Button.h = LineSize, Button.y = Label.y + (Label.h / 2.0f - Button.h / 2.0f);
		Ui()->DoLabel(&Label, "Voix", LineSize, TEXTALIGN_ML);
		RenderDevSkin(TeeRect.Center(), 50.0f, "Bomb 2", "bomb", false, 0, 0, 0, false, true);
	}

	RightView.HSplitTop(MarginSmall, nullptr, &RightView);
	RightView.HSplitTop(HeadlineHeight, &Label, &RightView);
	Ui()->DoLabel(&Label, RCLocalize("Hide Settings Tabs"), HeadlineFontSize, TEXTALIGN_ML);
	RightView.HSplitTop(MarginSmall, nullptr, &RightView);
	CUIRect LeftSettings, RightSettings;

	RightView.VSplitMid(&LeftSettings, &RightSettings, MarginSmall);
	RightView.HSplitTop(LineSize * 3.5f, nullptr, &RightView);

	static int s_ShowSettings = IsFlagSet(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_SETTINGS);
	DoButton_CheckBoxAutoVMarginAndSet(&s_ShowSettings, RCLocalize("Settings"), &s_ShowSettings, &LeftSettings, LineSize);
	SetFlag(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_SETTINGS, s_ShowSettings);
	static int s_ShowBindWheel = IsFlagSet(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_BINDWHEEL);
	DoButton_CheckBoxAutoVMarginAndSet(&s_ShowBindWheel, RCLocalize("Bindwheel"), &s_ShowBindWheel, &RightSettings, LineSize);
	SetFlag(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_BINDWHEEL, s_ShowBindWheel);
	static int s_ShowNameplatesEditor = IsFlagSet(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_NAMEPLATES_EDITOR);
	DoButton_CheckBoxAutoVMarginAndSet(&s_ShowNameplatesEditor, RCLocalize("Nameplate editor"), &s_ShowNameplatesEditor, &LeftSettings, LineSize);
	SetFlag(g_Config.m_RiRClientSettingsTabs, RCLIENT_TAB_NAMEPLATES_EDITOR, s_ShowNameplatesEditor);
}

void CMenus::RenderSettingsRushieSettings(CUIRect MainView)
{
	// Add scroll region using the same pattern as other menus
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 120.0f;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

	MainView.y += ScrollOffset.y;

	// Add padding for scrollbar
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView, Button, Label;

	auto DoBindchatDefault = [&](CUIRect &Column, CBindChat::CBindRclient &BindDefault) {
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		Column.HSplitTop(LineSize, &Button, &Column);
		CBindChat::CBind *pOldBind = GameClient()->m_BindChat.GetBind(BindDefault.m_Bind.m_aCommand);
		static char s_aTempName[BINDCHAT_MAX_NAME] = "";
		char *pName;
		if(pOldBind == nullptr)
			pName = s_aTempName;
		else
			pName = pOldBind->m_aName;
		if(DoEditBoxWithLabel(&BindDefault.m_LineInput, &Button, RCLocalize(BindDefault.m_pTitle, "Chatbinds"), BindDefault.m_Bind.m_aName, pName, BINDCHAT_MAX_NAME) && BindDefault.m_LineInput.IsActive())
		{
			if(!pOldBind && pName[0] != '\0')
			{
				auto BindNew = BindDefault.m_Bind;
				str_copy(BindNew.m_aName, pName);
				GameClient()->m_BindChat.RemoveBind(pName); // Prevent duplicates
				GameClient()->m_BindChat.AddBind(BindNew);
				s_aTempName[0] = '\0';
			}
			if(pOldBind && pName[0] == '\0')
			{
				GameClient()->m_BindChat.RemoveBind(pName);
			}
		}
	};

	// auto DoBindchatDefaults = [&](CUIRect &Column, const char *pTitle, std::vector<CBindChat::CBindRclient> &vBindchatDefaults) {
	// 	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	// 	Ui()->DoLabel(&Label, pTitle, HeadlineFontSize, TEXTALIGN_ML);
	// 	Column.HSplitTop(MarginSmall, nullptr, &Column);
	// 	for(CBindChat::CBindRclient &BindchatDefault : vBindchatDefaults)
	// 		DoBindchatDefault(Column, BindchatDefault);
	// 	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	// };

	// Split view into two columns
	CUIRect Column;
	MainView.VSplitMid(&LeftView, &RightView, 10.0f);

	// Left column - Find/Copy Skin/Player
	Column = LeftView;

	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Auto Change Player Info"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	for(CBindChat::CBindRclient &BindchatDefault : s_aDefaultBindChatRclientFindSkin)
		DoBindchatDefault(Column, BindchatDefault);

	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	static CButtonContainer s_FindSkinChatButton;
	if(DoButtonLineSize_Menu(&s_FindSkinChatButton, RCLocalize("Reset Find/Copy Skin Chatbinds"), 0, &Button, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.5f, 0.0f, 0.0f, 0.25f)))
	{
		for(const CBindChat::CBindRclient &BindDefault : s_aDefaultBindChatRclientFindSkin)
		{
			GameClient()->m_BindChat.RemoveBindCommand(BindDefault.m_Bind.m_aCommand);
			GameClient()->m_BindChat.AddBind(BindDefault.m_Bind);
		}
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_PlayerClanAutoChange, RCLocalize("Auto change clan when dummy connect"), &g_Config.m_PlayerClanAutoChange, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_PlayerClanAutoChange)
	{
		static CLineInput s_WithDummy;
		static CLineInput s_WithoutDummy;

		s_WithDummy.SetBuffer(g_Config.m_PlayerClanWithDummy, sizeof(g_Config.m_PlayerClanWithDummy));
		s_WithoutDummy.SetBuffer(g_Config.m_PlayerClanNoDummy, sizeof(g_Config.m_PlayerClanNoDummy));

		// player clan with dummy
		Column.HSplitTop(LineSize, &Label, &Column);
		DoEditBoxWithLabel(&s_WithDummy, &Label, RCLocalize("Clan with dummy:"), "", g_Config.m_PlayerClanWithDummy, sizeof(g_Config.m_PlayerClanWithDummy));
		Column.HSplitTop(MarginSmall, nullptr, &Column);

		// player clan without dummy
		Column.HSplitTop(LineSize, &Label, &Column);
		DoEditBoxWithLabel(&s_WithoutDummy, &Label, RCLocalize("Clan without dummy:"), "", g_Config.m_PlayerClanNoDummy, sizeof(g_Config.m_PlayerClanNoDummy));
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Chat Functions"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	for(CBindChat::CBindRclient &BindchatDefault : s_aDefaultBindChatRclientChat)
		DoBindchatDefault(Column, BindchatDefault);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	static CButtonContainer s_ChatChatButton;
	if(DoButtonLineSize_Menu(&s_ChatChatButton, RCLocalize("Reset Chat Chatbinds"), 0, &Button, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.5f, 0.0f, 0.0f, 0.25f)))
	{
		for(const CBindChat::CBindRclient &BindDefault : s_aDefaultBindChatRclientChat)
		{
			GameClient()->m_BindChat.RemoveBindCommand(BindDefault.m_Bind.m_aCommand);
			GameClient()->m_BindChat.AddBind(BindDefault.m_Bind);
		}
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	// Block list
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowBlockedWordInConsole, RCLocalize("Show blocked word in console"), &g_Config.m_RiShowBlockedWordInConsole, &Column, LineSize);
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_RiShowBlockedWordInConsole, &Column, RCLocalize("In console will be like 'tee said badbad'"));
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowBlockedWordInConsole)
	{
		CUIRect ColorRect;
		Column.HSplitTop(LineSize, &ColorRect, &Column);
		ColorRect.VSplitLeft(160.0f, &Label, &ColorRect); // Use fixed width instead of undefined ColumnWidth
		Ui()->DoLabel(&Label, RCLocalize("Blocked words console color"), FontSize, TEXTALIGN_MC);
		static CButtonContainer s_ColorPickerButton;
		ColorRGBA Color = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_RiBlockedWordConsoleColor));
		ColorRect.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f * Ui()->ButtonColorMul(&s_ColorPickerButton)), IGraphics::CORNER_ALL, 5.0f);
		CUIRect ColorRectInner = ColorRect;
		ColorRectInner.Margin(1.5f, &ColorRectInner);
		ColorRectInner.Draw(Color, IGraphics::CORNER_ALL, 3.0f);

		if(Ui()->DoButtonLogic(&s_ColorPickerButton, 0, &ColorRect, BUTTONFLAG_LEFT))
		{
			m_ColorPickerPopupContext.m_Alpha = false;
			m_ColorPickerPopupContext.m_pHslaColor = &g_Config.m_RiBlockedWordConsoleColor;
			m_ColorPickerPopupContext.m_HslaColor = ColorHSLA(g_Config.m_RiBlockedWordConsoleColor);
			m_ColorPickerPopupContext.m_HsvaColor = color_cast<ColorHSVA>(m_ColorPickerPopupContext.m_HslaColor);
			m_ColorPickerPopupContext.m_RgbaColor = color_cast<ColorRGBA>(m_ColorPickerPopupContext.m_HslaColor);
			Ui()->ShowPopupColorPicker(Ui()->MouseX(), Ui()->MouseY(), &m_ColorPickerPopupContext);
		}
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiEnableCensorList, RCLocalize("Enable word block list"), &g_Config.m_RiEnableCensorList, &Column, LineSize);
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_RiEnableCensorList, &Column, RCLocalize("Replacing blocked word with replacement char(badbad->******)"));
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiMultipleReplacementChar, RCLocalize("Multiple replacement char on blocked word len"), &g_Config.m_RiMultipleReplacementChar, &Column, LineSize);
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_RiMultipleReplacementChar, &Column, RCLocalize("if no will be 'badbad->*' if yes 'badbad->******'"));
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CLineInput s_ReplacementChar;
	s_ReplacementChar.SetBuffer(g_Config.m_RiBlockedContentReplacementChar, sizeof(g_Config.m_RiBlockedContentReplacementChar));
	Column.HSplitTop(LineSize, &Label, &Column);
	DoEditBoxWithLabel(&s_ReplacementChar, &Label, RCLocalize("Replacement char"), "*", g_Config.m_RiBlockedContentReplacementChar, sizeof(g_Config.m_RiBlockedContentReplacementChar));
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static std::vector<CButtonContainer> s_vButtonContainers = {{}, {}, {}};
	DoLine_RadioMenu(Column, RCLocalize("Replace word with:", "Rclientfilter"),
		s_vButtonContainers,
		{RCLocalize("Regex", "Rclientfilter"), RCLocalize("Full", "Rclientfilter"), RCLocalize("Both", "Rclientfilter")},
		{0, 1, 2},
		g_Config.m_RiFilterChangeWholeWord);
	if(g_Config.m_RiFilterChangeWholeWord == 2)
	{
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		static CLineInput s_PartialReplacementChar;
		s_PartialReplacementChar.SetBuffer(g_Config.m_RiBlockedContentPartialReplacementChar, sizeof(g_Config.m_RiBlockedContentPartialReplacementChar));
		Column.HSplitTop(LineSize, &Label, &Column);
		DoEditBoxWithLabel(&s_PartialReplacementChar, &Label, RCLocalize("Partial Replacement char"), "*", g_Config.m_RiBlockedContentPartialReplacementChar, sizeof(g_Config.m_RiBlockedContentPartialReplacementChar));
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChatAnim, RCLocalize("Animate chat"), &g_Config.m_RiChatAnim, &Column, LineSize);
	if(g_Config.m_RiChatAnim)
	{
		Column.HSplitTop(20.0f, &Label, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiChatAnimMs, &g_Config.m_RiChatAnimMs, &Label, RCLocalize("Anim chat ms"), 100, 2000, &CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Scoreboard"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiScoreboardFriendMark, RCLocalize("Show friend icon in scoreboard"), &g_Config.m_RiScoreboardFriendMark, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Changed Tater"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiFrozenHudPosX, &g_Config.m_RiFrozenHudPosX, &Button, RCLocalize("Pos x of Frozen hud"), 0, 100);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiIndicatorTransparentToggle, RCLocalize("Player indicator transparent toggle"), &g_Config.m_RiIndicatorTransparentToggle, &Column, LineSize);
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_RiIndicatorTransparentToggle, &Column, RCLocalize("if you move away from the tee, the indicator will become more transparent"));
	if(g_Config.m_RiIndicatorTransparentToggle)
	{
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		Column.HSplitTop(LineSize, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiIndicatorTransparentOffset, &g_Config.m_RiIndicatorTransparentOffset, &Button, RCLocalize("Indicator transparent start"), 16, 200);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		Column.HSplitTop(LineSize, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiIndicatorTransparentOffsetMax, &g_Config.m_RiIndicatorTransparentOffsetMax, &Button, RCLocalize("Indicator transparent end"), 16, 200);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		Column.HSplitTop(LineSize, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiIndicatorTransparentMin, &g_Config.m_RiIndicatorTransparentMin, &Button, RCLocalize("Indicator transparent minimum"), 0, 100);
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Nameplates", "RClient"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(20.0f, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Nameplate Scheme"), 14.0f, TEXTALIGN_ML);
	Column.HSplitTop(5.0f, nullptr, &Column);
	Column.HSplitTop(20.0f, &Button, &Column);
	static CLineInput s_NamePlateScheme(g_Config.m_RiNamePlateScheme, sizeof(g_Config.m_RiNamePlateScheme));
	if(Ui()->DoEditBox(&s_NamePlateScheme, &Button, FontSize))
	{
		GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
	}
	Column.HSplitTop(5.0f, nullptr, &Column);
	Column.HSplitTop(20.0f, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("p=ping i=ignore m=ID n=name c=clan d=direction f=friend h=hook r=reason s=skin H=HookName F=FireName l=newline"), 10.0f, TEXTALIGN_ML);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoLine_RadioMenu(Column, RCLocalize("Show you' fire presses"),
		m_vButtonContainersNamePlateFirePresses,
		{Localize("None", "Show players' key presses"), Localize("Own", "Show players' key presses RC"), Localize("Dummy", "Show players' key presses"), Localize("Both", "Show players' key presses")},
		{0, 3, 1, 2},
		g_Config.m_RiShowFire);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowFire > 0)
	{
		Column.HSplitTop(20.0f, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiFireDetectionSize, &g_Config.m_RiFireDetectionSize, &Button, Localize("Size of fire press icons"), -50, 100);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowFireDynamic, RCLocalize("Fire will change pos when some nearby"), &g_Config.m_RiShowFireDynamic, &Column, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	DoLine_RadioMenu(Column, RCLocalize("Show players' hook presses"),
		m_vButtonContainersNamePlateHookPresses,
		{Localize("None", "Show players' key presses"), Localize("Own", "Show players' key presses RC"), Localize("Others", "Show players' key presses"), Localize("All", "Show players' key presses")},
		{0, 3, 1, 2},
		g_Config.m_RiShowHook);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowHook > 0)
	{
		Column.HSplitTop(20.0f, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiHookDetectionSize, &g_Config.m_RiHookDetectionSize, &Button, Localize("Size of hook press icons"), -50, 100);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowHookDynamic, RCLocalize("Hook will change pos when some nearby"), &g_Config.m_RiShowHookDynamic, &Column, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Dummy", "RClient"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiAdvancedShowhudDummyActions, RCLocalize("Show Advanced Dummy Actions"), &g_Config.m_RiAdvancedShowhudDummyActions, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowhudDummyPosition, TCLocalize("Show Dummy position"), &g_Config.m_RiShowhudDummyPosition, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowhudDummyPosition)
	{
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowLastPosHudDummy, TCLocalize("Show last known pos instead no info"), &g_Config.m_RiShowLastPosHudDummy, &Column, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChangeDummyColorWhenXDummyEqualXPlayer, TCLocalize("Change dummy pos x color when x dummy = x player"), &g_Config.m_RiChangeDummyColorWhenXDummyEqualXPlayer, &Column, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
		if(g_Config.m_RiChangeDummyColorWhenXDummyEqualXPlayer)
		{
			CUIRect Rightoffset;
			Column.VSplitLeft(25.0f, &Label, &Rightoffset);
			Column.HSplitTop(LineSize, nullptr, &Column);
			DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChangePlayerColorWhenXDummyEqualXPlayer, TCLocalize("Change player pos x color when x dummy = x player"), &g_Config.m_RiChangePlayerColorWhenXDummyEqualXPlayer, &Rightoffset, LineSize);
			Column.HSplitTop(MarginSmall, nullptr, &Column);
		}
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Effects"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowSparkleTrail, TCLocalize("Show sparkle trail (from invicible in rcon)"), &g_Config.m_RiShowSparkleTrail, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowAfkEmoteInMenu, RCLocalize("Show sleep emote in menu (ONLY CLIENT OTHER DON'T SEE THAT)"), &g_Config.m_RiShowAfkEmoteInMenu, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowAfkEmoteInMenu)
	{
		CUIRect Rightoffset;
		Column.VSplitLeft(25.0f, &Label, &Rightoffset);
		Column.HSplitTop(LineSize, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowAfkTextureInMenu, RCLocalize("Show afk texture instead emote"), &g_Config.m_RiShowAfkTextureInMenu, &Rightoffset, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowAfkEmoteInSpec, RCLocalize("Show sleep emote in spec (ONLY CLIENT OTHER DON'T SEE THAT)"), &g_Config.m_RiShowAfkEmoteInSpec, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiShowAfkEmoteInSpec)
	{
		CUIRect Rightoffset;
		Column.VSplitLeft(25.0f, &Label, &Rightoffset);
		Column.HSplitTop(LineSize, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowAfkTextureInSpec, RCLocalize("Show spec texture instead emote"), &g_Config.m_RiShowAfkTextureInSpec, &Rightoffset, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}

	// Right column - Tracker pos
	LeftView = Column;
	Column = RightView;

	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Tracker Player"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	for(CBindChat::CBindRclient &BindchatDefault : s_aDefaultBindChatRclientTracker)
		DoBindchatDefault(Column, BindchatDefault);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	static CButtonContainer s_TrackerChatButton;
	if(DoButtonLineSize_Menu(&s_TrackerChatButton, RCLocalize("Reset Tracker Chatbinds"), 0, &Button, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.5f, 0.0f, 0.0f, 0.25f)))
	{
		for(const CBindChat::CBindRclient &BindDefault : s_aDefaultBindChatRclientTrackerHistory)
		{
			GameClient()->m_BindChat.RemoveBindCommand(BindDefault.m_Bind.m_aCommand);
		}
		for(const CBindChat::CBindRclient &BindDefault : s_aDefaultBindChatRclientTracker)
		{
			GameClient()->m_BindChat.RemoveBindCommand(BindDefault.m_Bind.m_aCommand);
			GameClient()->m_BindChat.AddBind(BindDefault.m_Bind);
		}
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowLastPosHud, RCLocalize("Show last known pos instead no info"), &g_Config.m_RiShowLastPosHud, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChangeTargetColorWhenXTargetEqualXPlayer, RCLocalize("Change target pos x color when x target = x player"), &g_Config.m_RiChangeTargetColorWhenXTargetEqualXPlayer, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	if(g_Config.m_RiChangeTargetColorWhenXTargetEqualXPlayer)
	{
		CUIRect Rightoffset;
		Column.VSplitLeft(25.0f, &Label, &Rightoffset);
		Column.HSplitTop(LineSize, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChangePlayerColorWhenXTargetEqualXPlayer, RCLocalize("Change player pos x color when x target = x player"), &g_Config.m_RiChangePlayerColorWhenXTargetEqualXPlayer, &Rightoffset, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Hud"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(20.0f, &Label, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiHeartSize, &g_Config.m_RiHeartSize, &Label, RCLocalize("Friend heart size"), 0, 500, &CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowMilliSecondsTimer, RCLocalize("Show milliseconds in timer"), &g_Config.m_RiShowMilliSecondsTimer, &Column, LineSize);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Controls"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CButtonContainer s_ReaderButtonDummyPseudo, s_ClearButtonDummyPseudo,
		s_ReaderButtonDeepfly, s_ClearButtonDeepfly,
		s_ReaderButtonDeepflyToggle, s_ClearButtonDeepflyToggle,
		s_ReaderButton45Degrees, s_ClearButton45Degrees,
		s_ReaderButtonSmallSens, s_ClearButtonSmallSens,
		s_ReaderButtonLeftJump, s_ClearButtonLeftJump,
		s_ReaderButtonRightJump, s_ClearButtonRightJump;
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonDummyPseudo, s_ClearButtonDummyPseudo, RCLocalize("Dummy pseudo"), "+toggle cl_dummy_hammer 1 0");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonDeepfly, s_ClearButtonDeepfly, RCLocalize("Deepfly"), "+fire;+toggle cl_dummy_hammer 1 0");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonDeepflyToggle, s_ClearButtonDeepflyToggle, RCLocalize("Deepfly toggle"), "ri_deepfly_toggle");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButton45Degrees, s_ClearButton45Degrees, RCLocalize("45° bind"), "+ri_45_degrees");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	{
		CUIRect Rightoffset;
		Column.VSplitLeft(25.0f, &Label, &Rightoffset);
		Column.HSplitTop(LineSize, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiToggle45degrees, RCLocalize("Toggle 45 degrees"), &g_Config.m_RiToggle45degrees, &Rightoffset, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSmallSens, s_ClearButtonSmallSens, RCLocalize("Small sens bind"), "ri_small_sens");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	{
		CUIRect Rightoffset;
		Column.VSplitLeft(25.0f, &Label, &Rightoffset);
		Column.HSplitTop(LineSize, nullptr, &Column);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiToggleSmallSens, RCLocalize("Toggle small sens"), &g_Config.m_RiToggleSmallSens, &Rightoffset, LineSize);
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonLeftJump, s_ClearButtonLeftJump, RCLocalize("Left jump"), "+jump; +left");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonRightJump, s_ClearButtonRightJump, RCLocalize("Right jump"), "+jump; +right");

	// Laser Settings
	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Laser Settings(Pulse)"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(10.0f, nullptr, &Column);

	// RTX Laser
	Column.HSplitTop(20.0f, &Button, &Column);
	if(DoButton_CheckBox(&g_Config.m_RiBetterLasers, RCLocalize("Enhanced Laser Effects"), g_Config.m_RiBetterLasers, &Button))
		g_Config.m_RiBetterLasers ^= 1;

	if(g_Config.m_RiBetterLasers)
	{
		// Laser Glow Intensity
		Column.HSplitTop(20.0f, &Button, &Column);
		Ui()->DoScrollbarOption(&g_Config.m_RiLaserGlowIntensity, &g_Config.m_RiLaserGlowIntensity, &Button, RCLocalize("Laser Glow Intensity"), 30, 100);

		// Laser Preview
		Column.HSplitTop(20.0f, &Label, &Column);
		Ui()->DoLabel(&Label, RCLocalize("Laser Preview"), 16.0f, TEXTALIGN_ML);
		Column.HSplitTop(10.0f, nullptr, &Column);

		const float LaserPreviewHeight = 50.0f;
		CUIRect LaserPreview;
		Column.HSplitTop(LaserPreviewHeight, &LaserPreview, &Column);
		Column.HSplitTop(2 * MarginSmall, nullptr, &Column);
		DoLaserPreview(&LaserPreview, g_Config.m_ClLaserRifleInnerColor, g_Config.m_ClLaserRifleOutlineColor, LASERTYPE_RIFLE);

		Column.HSplitTop(LaserPreviewHeight, &LaserPreview, &Column);
		Column.HSplitTop(2 * MarginSmall, nullptr, &Column);
		DoLaserPreview(&LaserPreview, g_Config.m_ClLaserShotgunInnerColor, g_Config.m_ClLaserShotgunOutlineColor, LASERTYPE_SHOTGUN);
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Spectator"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CButtonContainer s_ReaderButtonSpecPlr, s_ClearButtonSpecPlr;
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSpecPlr, s_ClearButtonSpecPlr, RCLocalize("Tracker for spectating player"), "ri_tracker_spectator");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CButtonContainer s_ReaderButtonSpectatorLeft, s_ClearButtonSpectatorLeft,
		s_ReaderButtonSpectatorRight, s_ClearButtonSpectatorRight,
		s_ReaderButtonSpectatorUp, s_ClearButtonSpectatorUp,
		s_ReaderButtonSpectatorDown, s_ClearButtonSpectatorDown;
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSpectatorLeft, s_ClearButtonSpectatorLeft, RCLocalize("Move left"), "+ri_spec_left");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSpectatorRight, s_ClearButtonSpectatorRight, RCLocalize("Move right"), "+ri_spec_right");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSpectatorUp, s_ClearButtonSpectatorUp, RCLocalize("Move up"), "+ri_spec_up");
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonSpectatorDown, s_ClearButtonSpectatorDown, RCLocalize("Move down"), "+ri_spec_down");
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Chat Bubbles(E-Client)"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChatBubbles, RCLocalize("Show Chatbubbles above players"), &g_Config.m_RiChatBubbles, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChatBubblesDemo, RCLocalize("Show Chatbubbles in demo"), &g_Config.m_RiChatBubblesDemo, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiChatBubblesSelf, RCLocalize("Show Chatbubbles above you"), &g_Config.m_RiChatBubblesSelf, &Column, LineSize);
	Column.HSplitTop(LineSize, &Button, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiChatBubbleSize, &g_Config.m_RiChatBubbleSize, &Button, RCLocalize("Chat Bubble Size"), 20, 30);
	Column.HSplitTop(MarginSmall, &Button, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	DoFloatScrollBar(&g_Config.m_RiChatBubbleShowTime, &g_Config.m_RiChatBubbleShowTime, &Button, RCLocalize("Show the Bubbles for"), 200, 1000, 100, &CUi::ms_LinearScrollbarScale, 0, "s");
	Column.HSplitTop(LineSize, &Button, &Column);
	DoFloatScrollBar(&g_Config.m_RiChatBubbleFadeIn, &g_Config.m_RiChatBubbleFadeIn, &Button, RCLocalize("fade in for"), 15, 100, 100, &CUi::ms_LinearScrollbarScale, 0, "s");
	Column.HSplitTop(LineSize, &Button, &Column);
	DoFloatScrollBar(&g_Config.m_RiChatBubbleFadeOut, &g_Config.m_RiChatBubbleFadeOut, &Button, RCLocalize("fade out for"), 15, 100, 100, &CUi::ms_LinearScrollbarScale, 0, "s");

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Discord RPC(E-Client)"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_TcDiscordRPC, RCLocalize("Use Discord Rich Presence"), &g_Config.m_TcDiscordRPC, &Column, LineSize);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiDiscordMapStatus, RCLocalize("Show What Map you're on"), &g_Config.m_RiDiscordMapStatus, &Column, LineSize);
	static int DiscordRPC = g_Config.m_TcDiscordRPC;
	static int DiscordRPCMap = g_Config.m_RiDiscordMapStatus;
	static char DiscordRPCOnlineMsg[25];
	static char DiscordRPCOfflineMsg[25];
	static bool DiscordRpcSet = false;
	if(!DiscordRpcSet)
	{
		str_copy(DiscordRPCOnlineMsg, g_Config.m_RiDiscordOnlineStatus);
		str_copy(DiscordRPCOfflineMsg, g_Config.m_RiDiscordOfflineStatus);
		DiscordRpcSet = true;
	}
	if(DiscordRPC != g_Config.m_TcDiscordRPC)
	{
		m_RPC_Ratelimit = time_get() + time_freq() * 1.5f;
		DiscordRPC = g_Config.m_TcDiscordRPC;
	}

	if(g_Config.m_TcDiscordRPC)
	{
		if(DiscordRPCMap != g_Config.m_RiDiscordMapStatus)
		{
			// Ratelimit this so it doesn't get changed instantly every time you edit this
			DiscordRPCMap = g_Config.m_RiDiscordMapStatus;
			m_RPC_Ratelimit = time_get() + time_freq() * 1.5f;
		}
		else if(str_comp(DiscordRPCOnlineMsg, g_Config.m_RiDiscordOnlineStatus) != 0)
		{
			str_copy(DiscordRPCOnlineMsg, g_Config.m_RiDiscordOnlineStatus);
			m_RPC_Ratelimit = time_get() + time_freq() * 2.5f;
		}
		else if(str_comp(DiscordRPCOfflineMsg, g_Config.m_RiDiscordOfflineStatus) != 0)
		{
			str_copy(DiscordRPCOfflineMsg, g_Config.m_RiDiscordOfflineStatus);
			m_RPC_Ratelimit = time_get() + time_freq() * 2.5f;
		}
	}

	{
		static CLineInput s_DiscordRPConline;

		s_DiscordRPConline.SetBuffer(g_Config.m_RiDiscordOnlineStatus, sizeof(g_Config.m_RiDiscordOnlineStatus));

		Column.HSplitTop(LineSize, &Label, &Column);
		DoEditBoxWithLabel(&s_DiscordRPConline, &Label, RCLocalize("Online Message:"), "", g_Config.m_RiDiscordOnlineStatus, sizeof(g_Config.m_RiDiscordOnlineStatus));
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}

	{
		static CLineInput s_DiscordRPCoffline;

		s_DiscordRPCoffline.SetBuffer(g_Config.m_RiDiscordOfflineStatus, sizeof(g_Config.m_RiDiscordOfflineStatus));

		Column.HSplitTop(LineSize, &Label, &Column);
		DoEditBoxWithLabel(&s_DiscordRPCoffline, &Label, RCLocalize("Offline Message:"), "", g_Config.m_RiDiscordOfflineStatus, sizeof(g_Config.m_RiDiscordOfflineStatus));
		Column.HSplitTop(MarginSmall, nullptr, &Column);
	}

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("RClient User Indicator"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowRclientIndicator, RCLocalize("Show RClient User indicator"), &g_Config.m_RiShowRclientIndicator, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	Column.HSplitTop(20.0f, &Button, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiRclientIndicatorSize, &g_Config.m_RiRclientIndicatorSize, &Button, Localize("Size of Rclient indicator icons"), -50, 100);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiShowIndicatorDynamic, RCLocalize("Indicator will change pos when some nearby"), &g_Config.m_RiShowIndicatorDynamic, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiRclientIndicatorAboveSelf, RCLocalize("Show indicator above you"), &g_Config.m_RiRclientIndicatorAboveSelf, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Edge Info"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CButtonContainer s_ReaderButtonEdgeInfo, s_ClearButtonEdgeInfo;
	Column.HSplitTop(LineSize, &Label, &Column);
	DoLine_KeyReader(Label, s_ReaderButtonEdgeInfo, s_ClearButtonEdgeInfo, RCLocalize("Show edge info"), "ri_toggle_edgeinfo");
	Column.HSplitTop(MarginSmall, nullptr, &Column);

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiEdgeInfoCords, RCLocalize("Show edge info about freeze"), &g_Config.m_RiEdgeInfoCords, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiEdgeInfoJump, RCLocalize("Show edge info about jumps"), &g_Config.m_RiEdgeInfoJump, &Column, LineSize);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	auto DoOutlineType = [&](CButtonContainer &ButtonContainer, const char *pName, unsigned int &Color, ColorRGBA ColorDefault) {
		// Checkbox & Color
		DoLine_ColorPicker(&ButtonContainer, ColorPickerLineSize, ColorPickerLabelSize, 0, &Column, pName, &Color, ColorDefault);
		Column.HSplitTop(ColorPickerLineSpacing, nullptr, &Column);
	};
	static CButtonContainer s_aOutlineButtonContainers[3];
	DoOutlineType(s_aOutlineButtonContainers[0], TCLocalize("Color when over freeze"), g_Config.m_RiEdgeInfoColorFreeze, color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_RiEdgeInfoColorFreeze)));
	DoOutlineType(s_aOutlineButtonContainers[1], TCLocalize("Color when over kill"), g_Config.m_RiEdgeInfoColorKill, color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_RiEdgeInfoColorKill)));
	DoOutlineType(s_aOutlineButtonContainers[2], TCLocalize("Color when falling safely"), g_Config.m_RiEdgeInfoColorSafe, color_cast<ColorRGBA>(ColorHSLA(CConfig::ms_RiEdgeInfoColorSafe)));
	Column.HSplitTop(LineSize, &Button, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiEdgeInfoPosX, &g_Config.m_RiEdgeInfoPosX, &Button, RCLocalize("Edge info pos x"), 0, 100);
	Column.HSplitTop(MarginSmall, &Button, &Column);
	Column.HSplitTop(LineSize, &Button, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_RiEdgeInfoPosY, &g_Config.m_RiEdgeInfoPosY, &Button, RCLocalize("Edge info pos y"), 0, 100);
	Column.HSplitTop(MarginSmall, &Button, &Column);


	RightView = Column;

	CUIRect ScrollRegion;
	ScrollRegion.x = MainView.x;
	ScrollRegion.y = maximum(LeftView.y, RightView.y) + MarginSmall * 2.0f;
	ScrollRegion.w = MainView.w;
	ScrollRegion.h = 0.0f;
	s_ScrollRegion.AddRect(ScrollRegion);
	s_ScrollRegion.End();
}

void CMenus::RenderSettingsRushieNameplatesEditor(CUIRect MainView)
{
	// --- Делим MainView на три колонки ---
	CUIRect ListCol, SchemeCol, PreviewCol, Spacer;
	MainView.VSplitLeft(MainView.w * 0.2f, &ListCol, &SchemeCol); // 20% список
	SchemeCol.VSplitLeft(SchemeCol.w * 0.5f, &SchemeCol, &PreviewCol); // 40% схема, 40% превью
	ListCol.VSplitRight(MarginSmall, &ListCol, &Spacer); // отступ справа от списка
	SchemeCol.VSplitRight(MarginSmall, &SchemeCol, &Spacer); // отступ справа от схемы
	PreviewCol.VSplitLeft(MarginSmall, &Spacer, &PreviewCol); // отступ слева от превью

	// --- 1. Левая колонка: вертикальный список для добавления ---
	ListCol.Margin(MarginSmall, &ListCol);
	ListCol.Draw(ColorRGBA(0.13f, 0.13f, 0.13f, 0.7f), IGraphics::CORNER_ALL, 8.0f);
	static const struct
	{
		char Code;
		const char *pDesc;
		const char *pIcon;
	} s_aElems[] = {
		{'n', "Nick", "N"},
		{'c', "Clan", "C"},
		{'m', "ID", "#"},
		{'f', "Friend", "♥"},
		{'d', "Dir", "⇄"},
		{'p', "Ping", "P"},
		{'r', "Reason", "R"},
		{'s', "Skin", "S"},
		{'i', "Ignore", "I"},
		{'h', "Hook", "H"},
		{'l', "↵", "↵"},
		{'H', "HookName", "HN"},
		{'F', "FireName", "FN"},
		{'I', "RC_User", "RI"},
	};

	// Улучшенные расчеты размеров кнопок с минимальными ограничениями
	const float MinBtnHeight = 24.0f; // минимальная высота кнопки
	const float BtnSpacing = MarginExtraSmall; // отступ между кнопками
	const float ContentPadding = MarginExtraSmall; // внутренние отступы

	float AvailableHeight = ListCol.h - 2.0f * ContentPadding;
	float TotalSpacing = BtnSpacing * (std::size(s_aElems) - 1);
	float BtnH = (AvailableHeight - TotalSpacing) / (float)std::size(s_aElems);
	BtnH = std::max(BtnH, MinBtnHeight); // применяем минимальную высоту

	float BtnW = ListCol.w - 2.0f * ContentPadding;
	float bx = ListCol.x + ContentPadding;
	float by = ListCol.y + ContentPadding;
	// Проверяем, помещаются ли все кнопки в доступное пространство
	float TotalNeededHeight = std::size(s_aElems) * BtnH + TotalSpacing + 2.0f * ContentPadding;
	bool UseScrolling = TotalNeededHeight > ListCol.h;

	for(unsigned i = 0; i < std::size(s_aElems); ++i)
	{
		float CurrentY = by + i * (BtnH + BtnSpacing);

		// Пропускаем кнопки, которые не помещаются в видимую область
		if(UseScrolling && (CurrentY + BtnH > ListCol.y + ListCol.h - ContentPadding))
			break;

		CUIRect Btn = {bx, CurrentY, BtnW, BtnH};
		static CButtonContainer s_aAddElemButtons[32];
		char aLabel[32];
		str_format(aLabel, sizeof(aLabel), "%s  %s", s_aElems[i].pIcon, RCLocalize(s_aElems[i].pDesc, "Nameplate_Editor"));
		if(DoButton_Menu(&s_aAddElemButtons[i], aLabel, 0, &Btn, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 16.0f, 0.0f, ColorRGBA(0.22f, 0.22f, 0.22f, 0.8f)))
		{
			std::vector<char> vScheme2;
			for(const char *p = g_Config.m_RiNamePlateScheme; *p; ++p)
				vScheme2.push_back(*p);
			vScheme2.push_back(s_aElems[i].Code);
			char aBuf[64] = {0};
			for(size_t j = 0; j < vScheme2.size() && j < sizeof(aBuf) - 1; ++j)
				aBuf[j] = vScheme2[j];
			str_copy(g_Config.m_RiNamePlateScheme, aBuf, sizeof(g_Config.m_RiNamePlateScheme));
			GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
		}
	}

	// --- 2. Средняя колонка: вертикальный drag&drop редактор (схема) ---
	SchemeCol.Margin(MarginSmall, &SchemeCol);
	SchemeCol.Draw(ColorRGBA(0.1f, 0.1f, 0.1f, 0.7f), IGraphics::CORNER_ALL, 8.0f);
	std::vector<char> vScheme;
	for(const char *p = g_Config.m_RiNamePlateScheme; *p; ++p)
		vScheme.push_back(*p);

	// Улучшенные расчеты размеров карточек - адаптивное сжатие
	const float CardSpacing = MarginExtraSmall; // отступ между карточками
	const float SchemeContentPadding = MarginExtraSmall; // внутренние отступы

	int SchemeSize = std::max(1, (int)vScheme.size());
	float AvailableSchemeHeight = SchemeCol.h - 2.0f * SchemeContentPadding;
	float TotalCardSpacing = CardSpacing * (SchemeSize - 1);
	float CardH = (AvailableSchemeHeight - TotalCardSpacing) / (float)SchemeSize;

	// Адаптивное сжатие: если карточки слишком маленькие, уменьшаем отступы
	if(CardH < 20.0f && SchemeSize > 1)
	{
		float ReducedSpacing = std::max(1.0f, CardSpacing * 0.5f);
		TotalCardSpacing = ReducedSpacing * (SchemeSize - 1);
		CardH = (AvailableSchemeHeight - TotalCardSpacing) / (float)SchemeSize;
	}

	float CardW = SchemeCol.w - 2.0f * SchemeContentPadding;
	float cx = SchemeCol.x + SchemeContentPadding;
	float cy = SchemeCol.y + SchemeContentPadding;
	static int s_DragIdx = -1;
	static int s_HoverIdx = -1;

	// Используем фактический отступ (может быть уменьшен для адаптивного сжатия)
	float ActualCardSpacing = (SchemeSize > 1) ? (AvailableSchemeHeight - SchemeSize * CardH) / (SchemeSize - 1) : 0.0f;

	for(size_t i = 0; i < vScheme.size(); ++i)
	{
		float CurrentCardY = cy + i * (CardH + ActualCardSpacing);

		CUIRect Card = {cx, CurrentCardY, CardW, CardH};
		bool Hovered = Ui()->MouseInside(&Card);
		if(Hovered)
			s_HoverIdx = i;
		const char *pDesc = "?";
		const char *pIcon = "?";
		for(const auto &e : s_aElems)
			if(e.Code == vScheme[i])
			{
				pDesc = e.pDesc;
				pIcon = e.pIcon;
			}
		Card.Draw((s_DragIdx == int(i)) ? ColorRGBA(0.3f, 0.3f, 0.6f, 0.9f) : (Hovered ? ColorRGBA(0.2f, 0.2f, 0.2f, 0.9f) : ColorRGBA(0.18f, 0.18f, 0.18f, 0.8f)), IGraphics::CORNER_ALL, 6.0f);

		// Кнопка удаления - размещаем сначала, чтобы зарезервировать место
		CUIRect DelRect = Card;
		float DelSize = std::min(14.0f, CardH * 0.4f);
		DelRect.x += Card.w - DelSize - 2.0f;
		DelRect.y += 2.0f;
		DelRect.w = DelSize;
		DelRect.h = DelSize;

		// Уменьшаем область для контента, чтобы не накладываться на кнопку удаления
		CUIRect ContentArea = Card;
		ContentArea.w -= DelSize + 4.0f; // оставляем место для кнопки удаления

		// Адаптивное размещение иконки и текста в зависимости от высоты карточки
		CUIRect IconRect = ContentArea, TextRect = ContentArea;
		if(CardH >= 30.0f)
		{
			// Нормальное размещение для больших карточек
			IconRect.HSplitTop(CardH * 0.15f, nullptr, &IconRect);
			IconRect.HSplitTop(CardH * 0.35f, &IconRect, &TextRect);
			TextRect.HSplitTop(CardH * 0.35f, &TextRect, nullptr);

			Ui()->DoLabel(&IconRect, pIcon, std::min(18.0f, CardH * 0.4f), TEXTALIGN_MC);
			Ui()->DoLabel(&TextRect, RCLocalize(pDesc, "Nameplate_Editor"), std::min(10.0f, CardH * 0.25f), TEXTALIGN_MC);
		}
		else
		{
			// Компактное размещение для маленьких карточек
			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "%s  %s", pIcon, RCLocalize(pDesc, "Nameplate_Editor"));
			IconRect.HSplitTop(CardH * 0.1f, nullptr, &IconRect);
			IconRect.HSplitTop(CardH * 0.8f, &IconRect, nullptr);

			// Только иконка для очень маленьких карточек
			Ui()->DoLabel(&IconRect, aBuf, std::min(CardH * 0.6f, 16.0f), TEXTALIGN_MC);
		}
		static CButtonContainer s_aDelButtons[64];
		if(DoButton_Menu(&s_aDelButtons[i], "✖", 0, &DelRect, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 8.0f, 0.0f, ColorRGBA(0.5f, 0.1f, 0.1f, 0.8f)))
		{
			vScheme.erase(vScheme.begin() + i);
			char aBuf[64] = {0};
			for(size_t j = 0; j < vScheme.size() && j < sizeof(aBuf) - 1; ++j)
				aBuf[j] = vScheme[j];
			str_copy(g_Config.m_RiNamePlateScheme, aBuf, sizeof(g_Config.m_RiNamePlateScheme));
			GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
			return;
		}
		if(Ui()->MouseButton(0) && Hovered && s_DragIdx == -1)
			s_DragIdx = i;
	}
	if(s_DragIdx >= 0 && Ui()->MouseButton(0))
	{
		if(s_HoverIdx >= 0 && s_HoverIdx != s_DragIdx)
		{
			std::swap(vScheme[s_DragIdx], vScheme[s_HoverIdx]);
			char aBuf[64] = {0};
			for(size_t j = 0; j < vScheme.size() && j < sizeof(aBuf) - 1; ++j)
				aBuf[j] = vScheme[j];
			str_copy(g_Config.m_RiNamePlateScheme, aBuf, sizeof(g_Config.m_RiNamePlateScheme));
			GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
			s_DragIdx = s_HoverIdx;
		}
	}
	else if(!Ui()->MouseButton(0))
	{
		s_DragIdx = -1;
		s_HoverIdx = -1;
	}

	// --- 3. Правая колонка: предпросмотр и чекбокс dummy ---
	CUIRect PreviewLabel, PreviewRect, DummyCheckRect;
	PreviewCol.HSplitTop(24.0f, &PreviewLabel, &PreviewCol);
	Ui()->DoLabel(&PreviewLabel, RCLocalize("Preview:"), 14.0f, TEXTALIGN_ML);
	PreviewCol.HSplitTop(PreviewCol.h - 84.0f, &PreviewRect, &PreviewCol); // 60+24=84, чтобы хватило места под строку схемы
	PreviewRect.Margin(6.0f, &PreviewRect);
	PreviewRect.Draw(ColorRGBA(0.12f, 0.12f, 0.12f, 0.7f), IGraphics::CORNER_ALL, 8.0f);
	static int showDummyPreview = false;
	PreviewCol.HSplitTop(24.0f, &DummyCheckRect, &PreviewCol);
	DoButton_CheckBoxAutoVMarginAndSet(&showDummyPreview, RCLocalize("Show dummy"), &showDummyPreview, &DummyCheckRect, 20.0f);
	vec2 PreviewPos = vec2(PreviewRect.x + PreviewRect.w / 2, PreviewRect.y + PreviewRect.h * 0.45f);
	GameClient()->m_NamePlates.RenderNamePlatePreview(PreviewPos, showDummyPreview ? 1 : 0);
	// --- Вывод схемы текстом ---
	static CLineInput s_CurrentScheme;
	s_CurrentScheme.SetBuffer(g_Config.m_RiNamePlateScheme, sizeof(g_Config.m_RiNamePlateScheme));
	CUIRect Label;
	PreviewCol.HSplitTop(LineSize, &Label, &PreviewCol);
	DoEditBoxWithLabel(&s_CurrentScheme, &Label, RCLocalize("Current scheme:"), "", g_Config.m_RiNamePlateScheme, sizeof(g_Config.m_RiNamePlateScheme));
	PreviewCol.HSplitTop(MarginSmall, nullptr, &PreviewCol);

	// --- Кнопки сброса (возвращаем к оригинальному виду) ---
	PreviewCol.HSplitTop(MarginSmall, nullptr, &PreviewCol);
	CUIRect ResetCol, ClearCol;
	static CButtonContainer s_ResetButton, s_ClearButton;
	PreviewCol.VSplitLeft(PreviewCol.w * 0.5f, &ResetCol, &ClearCol);
	ResetCol.VSplitRight(4.0f, &ResetCol, &Spacer);
	ClearCol.VSplitLeft(4.0f, &Spacer, &ClearCol);
	if(DoButton_Menu(&s_ResetButton, RCLocalize("Reset to default"), 0, &ResetCol, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 14.0f, 0.0f, ColorRGBA(0.3f, 0.1f, 0.1f, 0.8f)))
	{
		str_copy(g_Config.m_RiNamePlateScheme, "IpifmnlclrlFHlsldlh", sizeof(g_Config.m_RiNamePlateScheme));
		GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
	}
	if(DoButton_Menu(&s_ClearButton, RCLocalize("Clear all"), 0, &ClearCol, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 14.0f, 0.0f, ColorRGBA(0.3f, 0.1f, 0.1f, 0.8f)))
	{
		g_Config.m_RiNamePlateScheme[0] = 0;
		GameClient()->m_NamePlates.RiResetNameplatesPos(*GameClient(), g_Config.m_RiNamePlateScheme);
	}
}

void CMenus::RenderSettingsRushieRCON(CUIRect MainView)
{
	// Add scroll region using the same pattern as other menus
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 120.0f;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

	MainView.y += ScrollOffset.y;

	// Add padding for scrollbar
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView, Label;

	// auto DoBindchatDefault = [&](CUIRect &Column, CBindChat::CBindRclient &BindDefault) {
	// 	Column.HSplitTop(MarginSmall, nullptr, &Column);
	// 	Column.HSplitTop(LineSize, &Button, &Column);
	// 	CBindChat::CBind *pOldBind = GameClient()->m_BindChat.GetBind(BindDefault.m_Bind.m_aCommand);
	// 	static char s_aTempName[BINDCHAT_MAX_NAME] = "";
	// 	char *pName;
	// 	if(pOldBind == nullptr)
	// 		pName = s_aTempName;
	// 	else
	// 		pName = pOldBind->m_aName;
	// 	if(DoEditBoxWithLabel(&BindDefault.m_LineInput, &Button, RCLocalize(BindDefault.m_pTitle, "Chatbinds"), BindDefault.m_Bind.m_aName, pName, BINDCHAT_MAX_NAME) && BindDefault.m_LineInput.IsActive())
	// 	{
	// 		if(!pOldBind && pName[0] != '\0')
	// 		{
	// 			auto BindNew = BindDefault.m_Bind;
	// 			str_copy(BindNew.m_aName, pName);
	// 			GameClient()->m_BindChat.RemoveBind(pName); // Prevent duplicates
	// 			GameClient()->m_BindChat.AddBind(BindNew);
	// 			s_aTempName[0] = '\0';
	// 		}
	// 		if(pOldBind && pName[0] == '\0')
	// 		{
	// 			GameClient()->m_BindChat.RemoveBind(pName);
	// 		}
	// 	}
	// };

	// auto DoBindchatDefaults = [&](CUIRect &Column, const char *pTitle, std::vector<CBindChat::CBindRclient> &vBindchatDefaults) {
	// 	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	// 	Ui()->DoLabel(&Label, pTitle, HeadlineFontSize, TEXTALIGN_ML);
	// 	Column.HSplitTop(MarginSmall, nullptr, &Column);
	// 	for(CBindChat::CBindRclient &BindchatDefault : vBindchatDefaults)
	// 		DoBindchatDefault(Column, BindchatDefault);
	// 	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	// };

	// Split view into two columns
	CUIRect Column;
	MainView.VSplitMid(&LeftView, &RightView, 10.0f);

	// Left column - Find/Copy Skin/Player
	Column = LeftView;

	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Controls"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	static CButtonContainer s_ReaderButtonRCON, s_ClearButtonRCON;
	DoLine_KeyReader(Label, s_ReaderButtonRCON, s_ClearButtonRCON, RCLocalize("Admin Panel"), "toggle_adminpanel");

	Column.HSplitTop(MarginBetweenSections, nullptr, &Column);
	Column.HSplitTop(HeadlineHeight, &Label, &Column);
	Ui()->DoLabel(&Label, RCLocalize("Adminpanel"), HeadlineFontSize, TEXTALIGN_MC);
	Column.HSplitTop(MarginSmall, nullptr, &Column);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_RiPlaySounds, RCLocalize("Plays sound at exec command"), &g_Config.m_RiPlaySounds, &Column, LineSize);

	LeftView = Column;
	Column = RightView;

	RightView = Column;

	CUIRect ScrollRegion;
	ScrollRegion.x = MainView.x;
	ScrollRegion.y = maximum(LeftView.y, RightView.y) + MarginSmall * 2.0f;
	ScrollRegion.w = MainView.w;
	ScrollRegion.h = 0.0f;
	s_ScrollRegion.AddRect(ScrollRegion);
	s_ScrollRegion.End();
}

void CMenus::RenderSettingsRushieBindWheelSpec(CUIRect MainView)
{
	CUIRect LeftView, RightView, Label, Button;
	MainView.VSplitLeft(MainView.w / 2.1f, &LeftView, &RightView);

	const float Radius = minimum(RightView.w, RightView.h) / 2.0f;
	vec2 Center = RightView.Center();
	// Draw Circle
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f);
	Graphics()->DrawCircle(Center.x, Center.y, Radius, 64);
	Graphics()->QuadsEnd();

	static char s_aBindName[BINDWHEEL_MAX_NAME_RCLIENT];
	static char s_aBindCommand[BINDWHEEL_MAX_CMD_RCLIENT];

	static int s_SelectedBindIndex = -1;
	int HoveringIndex = -1;

	float MouseDist = distance(Center, Ui()->MousePos());
	const int SegmentCount = GameClient()->m_BindWheelSpec.m_vBinds.size();
	if (MouseDist < Radius && MouseDist > Radius * 0.25f && SegmentCount > 0)
	{
		float SegmentAngle = 2.0f * pi / SegmentCount;

		float HoveringAngle = angle(Ui()->MousePos() - Center) + SegmentAngle / 2.0f;
		if (HoveringAngle < 0.0f)
			HoveringAngle += 2.0f * pi;

		HoveringIndex = (int)(HoveringAngle / (2.0f * pi) * SegmentCount);
		HoveringIndex = std::clamp(HoveringIndex, 0, SegmentCount - 1);
		if (Ui()->MouseButtonClicked(0))
		{
			s_SelectedBindIndex = HoveringIndex;
			str_copy(s_aBindName, GameClient()->m_BindWheelSpec.m_vBinds[HoveringIndex].m_aName);
			str_copy(s_aBindCommand, GameClient()->m_BindWheelSpec.m_vBinds[HoveringIndex].m_aCommand);
		}
		else if (Ui()->MouseButtonClicked(1) && s_SelectedBindIndex >= 0 && HoveringIndex >= 0 && HoveringIndex != s_SelectedBindIndex)
		{
			CBindWheelSpec::CBind BindA = GameClient()->m_BindWheelSpec.m_vBinds[s_SelectedBindIndex];
			CBindWheelSpec::CBind BindB = GameClient()->m_BindWheelSpec.m_vBinds[HoveringIndex];
			str_copy(GameClient()->m_BindWheelSpec.m_vBinds[s_SelectedBindIndex].m_aName, BindB.m_aName);
			str_copy(GameClient()->m_BindWheelSpec.m_vBinds[s_SelectedBindIndex].m_aCommand, BindB.m_aCommand);
			str_copy(GameClient()->m_BindWheelSpec.m_vBinds[HoveringIndex].m_aName, BindA.m_aName);
			str_copy(GameClient()->m_BindWheelSpec.m_vBinds[HoveringIndex].m_aCommand, BindA.m_aCommand);
		}
		else if (Ui()->MouseButtonClicked(2))
		{
			s_SelectedBindIndex = HoveringIndex;
		}
	}
	else if (MouseDist < Radius && Ui()->MouseButtonClicked(0))
	{
		s_SelectedBindIndex = -1;
		str_copy(s_aBindName, "");
		str_copy(s_aBindCommand, "");
	}

	const float Theta = pi * 2.0f / std::max<float>(1.0f, GameClient()->m_BindWheelSpec.m_vBinds.size());
	for (int i = 0; i < static_cast<int>(GameClient()->m_BindWheelSpec.m_vBinds.size()); i++)
	{
		TextRender()->TextColor(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));

		float SegmentFontSize = FontSize * 1.1f;
		if (i == s_SelectedBindIndex)
		{
			SegmentFontSize = FontSize * 1.7f;
			TextRender()->TextColor(ColorRGBA(0.5f, 1.0f, 0.75f, 1.0f));
		}
		else if (i == HoveringIndex)
		{
			SegmentFontSize = FontSize * 1.35f;
		}

		const CBindWheelSpec::CBind Bind = GameClient()->m_BindWheelSpec.m_vBinds[i];
		const float Angle = Theta * i;

		const vec2 Pos = direction(Angle) * (Radius * 0.75f) + Center;
		const CUIRect Rect = CUIRect{ Pos.x - 50.0f, Pos.y - 50.0f, 100.0f, 100.0f };
		Ui()->DoLabel(&Rect, Bind.m_aName, SegmentFontSize, TEXTALIGN_MC);
	}

	TextRender()->TextColor(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));

	LeftView.HSplitTop(LineSize, &Button, &LeftView);
	Button.VSplitLeft(100.0f, &Label, &Button);
	Ui()->DoLabel(&Label, RCLocalize("Name:"), FontSize, TEXTALIGN_ML);
	static CLineInput s_NameInput;
	s_NameInput.SetBuffer(s_aBindName, sizeof(s_aBindName));
	s_NameInput.SetEmptyText(RCLocalize("Name"));
	Ui()->DoEditBox(&s_NameInput, &Button, EditBoxFontSize);

	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);
	LeftView.HSplitTop(LineSize, &Button, &LeftView);
	Button.VSplitLeft(100.0f, &Label, &Button);
	Ui()->DoLabel(&Label, RCLocalize("Command:"), FontSize, TEXTALIGN_ML);
	static CLineInput s_BindInput;
	s_BindInput.SetBuffer(s_aBindCommand, sizeof(s_aBindCommand));
	s_BindInput.SetEmptyText(RCLocalize("Command"));
	Ui()->DoEditBox(&s_BindInput, &Button, EditBoxFontSize);

	static CButtonContainer s_AddButton, s_RemoveButton, s_OverrideButton;

	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);
	LeftView.HSplitTop(LineSize, &Button, &LeftView);
	if (DoButton_Menu(&s_OverrideButton, RCLocalize("Override Selected"), 0, &Button) && s_SelectedBindIndex >= 0 && s_SelectedBindIndex < static_cast<int>(GameClient()->m_BindWheel.m_vBinds.size()))
	{
		CBindWheel::CBind TempBind;
		if (str_length(s_aBindName) == 0)
			str_copy(TempBind.m_aName, "*");
		else
			str_copy(TempBind.m_aName, s_aBindName);

		str_copy(GameClient()->m_BindWheelSpec.m_vBinds[s_SelectedBindIndex].m_aName, TempBind.m_aName);
		str_copy(GameClient()->m_BindWheelSpec.m_vBinds[s_SelectedBindIndex].m_aCommand, s_aBindCommand);
	}
	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);
	LeftView.HSplitTop(LineSize, &Button, &LeftView);
	CUIRect ButtonAdd, ButtonRemove;
	Button.VSplitMid(&ButtonRemove, &ButtonAdd, MarginSmall);
	if(DoButton_Menu(&s_AddButton, RCLocalize("Add Bind"), 0, &ButtonAdd))
	{
		CBindWheel::CBind TempBind;
		if (str_length(s_aBindName) == 0)
			str_copy(TempBind.m_aName, "*");
		else
			str_copy(TempBind.m_aName, s_aBindName);

		GameClient()->m_BindWheelSpec.AddBind(TempBind.m_aName, s_aBindCommand);
		s_SelectedBindIndex = static_cast<int>(GameClient()->m_BindWheelSpec.m_vBinds.size()) - 1;
	}
	if(DoButton_Menu(&s_RemoveButton, RCLocalize("Remove Bind"), 0, &ButtonRemove) && s_SelectedBindIndex >= 0)
	{
		GameClient()->m_BindWheelSpec.RemoveBind(s_SelectedBindIndex);
		s_SelectedBindIndex = -1;
	}

	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);
	LeftView.HSplitTop(LineSize, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("The command is ran in console not chat"), FontSize, TEXTALIGN_ML);
	LeftView.HSplitTop(LineSize * 0.8f, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("Use left mouse to select"), FontSize * 0.8f, TEXTALIGN_ML);
	LeftView.HSplitTop(LineSize * 0.8f, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("Use right mouse to swap with selected"), FontSize * 0.8f, TEXTALIGN_ML);
	LeftView.HSplitTop(LineSize * 0.8f, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("Use middle mouse select without copy"), FontSize * 0.8f, TEXTALIGN_ML);
	LeftView.HSplitTop(MarginBetweenSections, &Label, &LeftView);
	LeftView.HSplitTop(LineSize, &Label, &LeftView);
	TextRender()->TextColor(ColorRGBA(0.53f, 1.00f, 0.53f, 1.0f));
	Ui()->DoLabel(&Label, RCLocalize("Rclient bindwheel settings"), FontSize, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	LeftView.HSplitTop(LineSize, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("playernickname to enter nickname"), FontSize, TEXTALIGN_ML);
	{
		CUIRect Rightoffset;
		LeftView.VSplitLeft(25.0f, &Label, &Rightoffset);
		Rightoffset.HSplitTop(LineSize, &Label, &Rightoffset);
		TextRender()->TextColor(ColorRGBA(1.00f, 0.53f, 0.53f, 1.0f));
		Ui()->DoLabel(&Label, RCLocalize("Do \"playernickname\" yourself in u need"), FontSize, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
	LeftView.HSplitTop(LineSize, &Label, &LeftView);
	LeftView.HSplitTop(LineSize, &Label, &LeftView);
	Ui()->DoLabel(&Label, RCLocalize("playerid to enter id"), FontSize, TEXTALIGN_ML);


	LeftView.HSplitBottom(LineSize, &LeftView, &Label);
	static CButtonContainer s_ReaderButtonSpecWheel, s_ClearButtonSpecWheel;
	DoLine_KeyReader(Label, s_ReaderButtonSpecWheel, s_ClearButtonSpecWheel, RCLocalize("Bind Wheel In Spec Key"), "+bindwheel_spec");

	LeftView.HSplitBottom(LineSize, &LeftView, &Label);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_TcResetBindWheelMouse, RCLocalize("Reset position of mouse when opening bindwheel"), &g_Config.m_TcResetBindWheelMouse, &Label, LineSize);
}

bool CMenus::DoFloatScrollBar(const void *pId, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, int DivideBy, const IScrollbarScale *pScale, unsigned Flags, const char *pSuffix)
{
	const bool Infinite = Flags & CUi::SCROLLBAR_OPTION_INFINITE;
	const bool NoClampValue = Flags & CUi::SCROLLBAR_OPTION_NOCLAMPVALUE;
	const bool MultiLine = Flags & CUi::SCROLLBAR_OPTION_MULTILINE;

	int Value = *pOption;
	if(Infinite)
	{
		Max += 1;
		if(Value == 0)
			Value = Max;
	}

	// Allow adjustment of slider options when ctrl is pressed (to avoid scrolling, or accidentally adjusting the value)
	int Increment = std::max(1, (Max - Min) / 35);
	if(Input()->ModifierIsPressed() && Input()->KeyPress(KEY_MOUSE_WHEEL_UP) && Ui()->MouseInside(pRect))
	{
		Value += Increment;
		Value = std::clamp(Value, Min, Max);
	}
	if(Input()->ModifierIsPressed() && Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) && Ui()->MouseInside(pRect))
	{
		Value -= Increment;
		Value = std::clamp(Value, Min, Max);
	}
	if(Input()->KeyPress(KEY_A) && Ui()->MouseInside(pRect))
	{
		Value -= Input()->ModifierIsPressed() ? 5 : 1;
		Value = std::clamp(Value, Min, Max);
	}
	if(Input()->KeyPress(KEY_D) && Ui()->MouseInside(pRect))
	{
		Value += Input()->ModifierIsPressed() ? 5 : 1;
		Value = std::clamp(Value, Min, Max);
	}

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s: %.1f%s", pStr, (float)Value / DivideBy, pSuffix);

	Value = std::clamp(Value, Min, Max);

	CUIRect Label, ScrollBar;
	if(MultiLine)
		pRect->HSplitMid(&Label, &ScrollBar);
	else
		pRect->VSplitMid(&Label, &ScrollBar, minimum(10.0f, pRect->w * 0.05f));

	const float aFontSize = Label.h * CUi::ms_FontmodHeight * 0.8f;
	Ui()->DoLabel(&Label, aBuf, aFontSize, TEXTALIGN_ML);

	Value = pScale->ToAbsolute(Ui()->DoScrollbarH(pId, &ScrollBar, pScale->ToRelative(Value, Min, Max)), Min, Max);
	if(NoClampValue && ((Value == Min && *pOption < Min) || (Value == Max && *pOption > Max)))
	{
		Value = *pOption; // use previous out of range value instead if the scrollbar is at the edge
	}
	else if(Infinite)
	{
		if(Value == Max)
			Value = 0;
	}

	if(*pOption != Value)
	{
		*pOption = Value;
		return true;
	}
	return false;
}
