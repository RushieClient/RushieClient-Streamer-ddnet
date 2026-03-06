#include "menus_start_rclient.h"

#include <algorithm>

#include <engine/client/updater.h>
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>
#include <game/version.h>

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

void CMenusStartRClient::RenderStartMenu(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);

	const CUIRect View = MainView;
	const auto Lerp = [](float From, float To, float Amount) {
		return From + (To - From) * Amount;
	};

	m_LogoMenuAnim = std::clamp(m_LogoMenuAnim + Client()->RenderFrameTime() * (m_LogoMenuExpanded ? 6.0f : -6.0f), 0.0f, 1.0f);
	const float AnimProgress = m_LogoMenuAnim * m_LogoMenuAnim * (3.0f - 2.0f * m_LogoMenuAnim);

	// render logo
	const float LogoCircleSizeCollapsed = 170.0f;
	const float LogoCircleSizeExpanded = 118.0f;
	const float LogoBorderSize = 4.0f;
	const float LogoSizeCollapsed = 120.0f;
	const float LogoSizeExpanded = 80.0f;
	const float ViewCenterX = View.x + View.w / 2.0f;
	const float LogoCenterYCollapsed = View.y + View.h / 2.0f - 10.0f;
	const float LogoCenterYExpanded = LogoCenterYCollapsed;
	const float LogoCenterXExpanded = ViewCenterX + std::min(View.w * 0.18f, 170.0f);
	const float LogoCircleSize = Lerp(LogoCircleSizeCollapsed, LogoCircleSizeExpanded, AnimProgress);
	const float LogoSize = Lerp(LogoSizeCollapsed, LogoSizeExpanded, AnimProgress);
	const float LogoCenterX = Lerp(ViewCenterX, LogoCenterXExpanded, AnimProgress);
	const float LogoCenterY = Lerp(LogoCenterYCollapsed, LogoCenterYExpanded, AnimProgress);

	CUIRect LogoCircle = {LogoCenterX - LogoCircleSize / 2.0f, LogoCenterY - LogoCircleSize / 2.0f, LogoCircleSize, LogoCircleSize};

	// render ribbon
	const CUIRect *pScreen = Ui()->Screen();
	const float RibbonHeight = 68.0f;
	const float RibbonY = LogoCircle.y + LogoCircle.h / 2.0f - RibbonHeight / 2.0f;
	const float RibbonReveal = AnimProgress * AnimProgress * (3.0f - 2.0f * AnimProgress);
	if(RibbonReveal > 0.0f)
	{
		const float LeftReveal = (LogoCenterX - pScreen->x) * RibbonReveal;
		const float RightReveal = (pScreen->x + pScreen->w - LogoCenterX) * RibbonReveal;
		CUIRect Ribbon = {
			LogoCenterX - LeftReveal,
			RibbonY,
			LeftReveal + RightReveal,
			RibbonHeight};
		Ribbon.Draw(ColorRGBA(0.32f, 0.32f, 0.34f, 1.0f), IGraphics::CORNER_NONE, 0.0f);
	}

	static CButtonContainer s_LogoButton;
	const bool LogoHovered = Ui()->MouseHovered(&LogoCircle);
	const bool LogoActive = Ui()->CheckActiveItem(&s_LogoButton);
	const float LogoShade = LogoActive ? 0.92f : (LogoHovered ? 1.0f : 0.97f);
	LogoCircle.Draw(ColorRGBA(LogoShade, LogoShade, LogoShade, 1.0f), IGraphics::CORNER_ALL, LogoCircle.h / 2.0f);

	CUIRect LogoCircleInner;
	LogoCircle.Margin(LogoBorderSize, &LogoCircleInner);
	LogoCircleInner.Draw(ColorRGBA(0.12f, 0.12f, 0.12f, 1.0f), IGraphics::CORNER_ALL, LogoCircleInner.h / 2.0f);

	CUIRect LogoRect;
	LogoCircleInner.Margin((LogoCircleInner.w - LogoSize) / 2.0f, &LogoRect);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RCLIENT_BIG_LOGO].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadItem(LogoRect.x, LogoRect.y, LogoRect.w, LogoRect.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	if(Ui()->DoButtonLogic(&s_LogoButton, 0, &LogoCircle, BUTTONFLAG_LEFT))
	{
		m_LogoMenuExpanded = !m_LogoMenuExpanded;
	}

	const float VMargin = std::max(30.0f, View.w / 2.0f - 190.0f);

	int NewPage = -1;

	CUIRect ExtMenu;
	View.HSplitBottom(15.0f, nullptr, &ExtMenu);
	CUIRect Button;
	ExtMenu.VSplitLeft(75.0f, &Button, &ExtMenu);

	static CButtonContainer s_DiscordButton;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(GameClient()->m_Menus.DoButton_Menu(&s_DiscordButton, FontIcon::TERMINAL, 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f)))
	{
		Client()->ViewLink(Localize("https://ddnet.org/discord"));
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	const float ButtonWidthTarget = std::clamp(View.w * 0.115f, 108.0f, 150.0f);
	const float ButtonHeight = RibbonHeight;
	const float ButtonSpacing = 3.0f;
	const float SideGap = 18.0f;
	const float ButtonsCollapsedX = LogoCenterX - ButtonWidthTarget / 2.0f;
	const float ButtonsCollapsedY = RibbonY;
	const float LeftBlockWidth = 4.0f * ButtonWidthTarget + 3.0f * ButtonSpacing;
	const float RightBlockWidth = ButtonWidthTarget;
	const float LeftStartX = std::max(View.x + 16.0f, LogoCircle.x - SideGap - LeftBlockWidth);
	const float RightStartX = std::min(View.x + View.w - RightBlockWidth - 16.0f, LogoCircle.x + LogoCircle.w + SideGap);
	const auto ButtonRevealProgress = [&](int Index, float Delay) {
		return std::clamp((AnimProgress - Delay - Index * 0.08f) / 0.5f, 0.0f, 1.0f);
	};
	const auto ButtonEase = [](float Progress) {
		return Progress * Progress * (3.0f - 2.0f * Progress);
	};
	const auto PrepareAnimatedButton = [&](bool LeftSide, int Index, CUIRect *pButtonRect) {
		const float Progress = ButtonEase(ButtonRevealProgress(Index, LeftSide ? 0.05f : 0.1f));
		if(Progress <= 0.0f)
			return 0.0f;

		const float Width = Lerp(ButtonWidthTarget * 0.55f, ButtonWidthTarget, Progress);
		const float XTarget = LeftSide ? LeftStartX + Index * (ButtonWidthTarget + ButtonSpacing) : RightStartX + Index * (ButtonWidthTarget + ButtonSpacing);
		const float X = Lerp(ButtonsCollapsedX, XTarget, Progress);
		const float Y = ButtonsCollapsedY;
		*pButtonRect = {X, Y, Width, ButtonHeight};
		return Progress;
	};
	const auto RenderRibbonButton = [&](CButtonContainer *pButton, const CUIRect &Rect, float Progress, ColorRGBA BaseColor, const char *pIcon, const char *pLabel) -> bool {
		if(Progress <= 0.0f)
			return false;

		const bool Hovered = Ui()->MouseHovered(&Rect);
		const bool Active = Ui()->CheckActiveItem(pButton);
		const float Highlight = Active ? 0.82f : (Hovered ? 1.08f : 1.0f);
		ColorRGBA Fill = BaseColor;
		Fill.r = std::clamp(Fill.r * Highlight, 0.0f, 1.0f);
		Fill.g = std::clamp(Fill.g * Highlight, 0.0f, 1.0f);
		Fill.b = std::clamp(Fill.b * Highlight, 0.0f, 1.0f);
		Fill.a = 1.0f;

		CUIRect Panel = Rect;
		Panel.Draw(Fill, IGraphics::CORNER_NONE, 0.0f);

		CUIRect LabelRect = Rect;
		LabelRect.Margin(8.0f, &LabelRect);
		CUIRect IconRect, TextRect;
		LabelRect.HSplitTop(LabelRect.h * 0.58f, &IconRect, &TextRect);

		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		Ui()->DoLabel(&IconRect, pIcon, IconRect.h * 0.55f, TEXTALIGN_MC);
		TextRender()->SetRenderFlags(0);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		Ui()->DoLabel(&TextRect, pLabel, TextRect.h * 0.72f, TEXTALIGN_MC);

		return Ui()->DoButtonLogic(pButton, 0, &Rect, BUTTONFLAG_LEFT) != 0;
	};

	static CButtonContainer s_QuitButton;
	bool UsedEscape = false;
	float ButtonProgress;
	ButtonProgress = PrepareAnimatedButton(true, 0, &Button);
	if((ButtonProgress > 0.0f && RenderRibbonButton(&s_QuitButton, Button, ButtonProgress, ColorRGBA(0.88f, 0.18f, 0.56f, 1.0f), FontIcon::XMARK, Localize("Quit"))) || (UsedEscape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)) || CheckHotKey(KEY_Q))
	{
		if(UsedEscape || GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
		{
			GameClient()->m_Menus.ShowQuitPopup();
		}
		else
		{
			Client()->Quit();
		}
	}

	static CButtonContainer s_SettingsButton;
	ButtonProgress = PrepareAnimatedButton(false, 0, &Button);
	if((ButtonProgress > 0.0f && RenderRibbonButton(&s_SettingsButton, Button, ButtonProgress, ColorRGBA(0.35f, 0.35f, 0.38f, 1.0f), FontIcon::GEAR, Localize("Settings"))) || CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;

	static CButtonContainer s_LocalServerButton;
	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	ButtonProgress = PrepareAnimatedButton(true, 1, &Button);
	if(ButtonProgress > 0.0f)
	{
		const ColorRGBA ServerColor = LocalServerRunning ? ColorRGBA(0.15f, 0.73f, 0.34f, 1.0f) : ColorRGBA(0.92f, 0.66f, 0.08f, 1.0f);
		if(RenderRibbonButton(&s_LocalServerButton, Button, ButtonProgress, ServerColor, FontIcon::NETWORK_WIRED, LocalServerRunning ? Localize("Stop server") : Localize("Run server")) || (CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
		{
			if(LocalServerRunning)
			{
				GameClient()->m_LocalServer.KillServer();
			}
			else
			{
				GameClient()->m_LocalServer.RunServer({});
			}
		}
	}

	static CButtonContainer s_DemoButton;
	ButtonProgress = PrepareAnimatedButton(true, 2, &Button);
	if((ButtonProgress > 0.0f && RenderRibbonButton(&s_DemoButton, Button, ButtonProgress, ColorRGBA(0.63f, 0.81f, 0.02f, 1.0f), FontIcon::CLAPPERBOARD, Localize("Demos"))) || CheckHotKey(KEY_D))
	{
		NewPage = CMenus::PAGE_DEMOS;
	}

	static CButtonContainer s_PlayButton;
	ButtonProgress = PrepareAnimatedButton(true, 3, &Button);
	if((ButtonProgress > 0.0f && RenderRibbonButton(&s_PlayButton, Button, ButtonProgress, ColorRGBA(0.43f, 0.29f, 0.88f, 1.0f), FontIcon::CIRCLE_PLAY, Localize("Play", "Start menu"))) ||
		Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
	{
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
	}

	// render version
	CUIRect CurVersion, ConsoleButton;
	View.HSplitBottom(45.0f, nullptr, &CurVersion);
	CurVersion.VSplitRight(40.0f, &CurVersion, nullptr);
	CurVersion.HSplitTop(20.0f, &ConsoleButton, &CurVersion);
	CurVersion.HSplitTop(5.0f, nullptr, &CurVersion);
	ConsoleButton.VSplitRight(40.0f, nullptr, &ConsoleButton);
	Ui()->DoLabel(&CurVersion, GAME_RELEASE_VERSION, 14.0f, TEXTALIGN_MR);

	CUIRect TClientVersion;
	View.HSplitTop(15.0f, &TClientVersion, nullptr);
	TClientVersion.VSplitRight(40.0f, &TClientVersion, nullptr);
	char aTBuf[64];
	char aRBuf[64];
	str_format(aTBuf, sizeof(aTBuf), "TClient %s", TCLIENT_VERSION);
	if(GameClient()->m_TClient.NeedUpdate())
		TextRender()->TextColor(1.0f, 0.2f, 0.2f, 1.0f);
	Ui()->DoLabel(&TClientVersion, aTBuf, 14.0f, TEXTALIGN_MR);
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	// Position RClient below TClient
	{
		CUIRect RClientVersion;
		View.HSplitTop(25.0f, &RClientVersion, nullptr);
		RClientVersion.VSplitRight(42.5f, &RClientVersion, nullptr);
		str_format(aRBuf, sizeof(aRBuf), "RClient %s", RCLIENT_VERSION);
		if(GameClient()->m_RClient.NeedUpdate())
			TextRender()->TextColor(1.0f, 0.2f, 0.2f, 1.0f);
		Ui()->DoLabel(&RClientVersion, aRBuf, 14.0f, TEXTALIGN_MR);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}

	{
		CUIRect Version;
		View.HSplitTop(40.0f, &Version, nullptr);
		char aBuf[128] = "Based on Tater client. Thanks Pulse and Entity clients for some functions";
		Ui()->DoLabel(&Version, aBuf, 14.0f, TEXTALIGN_CENTER);
	}
	static CButtonContainer s_ConsoleButton;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(GameClient()->m_Menus.DoButton_Menu(&s_ConsoleButton, FontIcon::TERMINAL, 0, &ConsoleButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f)))
	{
		GameClient()->m_GameConsole.Toggle(CGameConsole::CONSOLETYPE_LOCAL);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	CUIRect VersionUpdate;
	View.HSplitBottom(20.0f, nullptr, &VersionUpdate);
	VersionUpdate.VMargin(VMargin, &VersionUpdate);
	if(GameClient()->m_RClient.NeedUpdate())
	{
		CUIRect UpdateButton;
		VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &UpdateButton);
		VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Download"), 0, &UpdateButton, BUTTONFLAG_LEFT, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(1.0f, 0.2f, 0.2f, 0.25f)))
		{
			Client()->ViewLink(CRClient::RCLIENT_URL);
		}

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), Localize("Rushie client %s is out!"), GameClient()->m_RClient.m_aVersionStr);
		TextRender()->TextColor(1.0f, 0.2f, 0.2f, 1.0f);
		Ui()->DoLabel(&VersionUpdate, aBuf, 14.0f, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStartRClient::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() &&
	       Input()->KeyPress(Key) &&
	       !GameClient()->m_GameConsole.IsActive();
}
