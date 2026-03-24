#ifndef GAME_CLIENT_COMPONENTS_RCLIENT_MENUS_RCLIENT_CLICKGUI_H
#define GAME_CLIENT_COMPONENTS_RCLIENT_MENUS_RCLIENT_CLICKGUI_H

#include <engine/console.h>
#include "game/client/render.h"
#include <game/client/component.h>

class CMenusRClientClickGui : public CComponent
{
	bool m_Active = false;

	static void ConToggleClickGui(IConsole::IResult *pResult, void *pUserData);

	void RenderDevSkin(vec2 RenderPos, float Size, const char *pSkinName, const char *pBackupSkin, bool CustomColors, int FeetColor, int BodyColor, int Emote, bool Cute,
			ColorRGBA ColorFeet = ColorRGBA(0, 0, 0, 0), ColorRGBA ColorBody = ColorRGBA(0, 0, 0, 0));
	void RenderTeeCute(const CAnimState *pAnim, const CTeeRenderInfo *pInfo, int Emote, vec2 Dir, vec2 Pos, bool CuteEyes, float Alpha = 1.0f);
public:
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnRender() override;

	void SetActive(bool Active);
	bool IsActive() const { return m_Active; }
};

#endif
