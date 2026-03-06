#ifndef RCLIENT_MENUS_START_RCLIENT_H
#define RCLIENT_MENUS_START_RCLIENT_H

#include <game/client/component.h>
#include <game/client/ui_rect.h>

class CMenusStartRClient : public CComponentInterfaces
{
public:
	void RenderStartMenu(CUIRect MainView);

private:
	bool CheckHotKey(int Key) const;
	bool m_LogoMenuExpanded = false;
	float m_LogoMenuAnim = 0.0f;
};

#endif //RCLIENT_MENUS_START_RCLIENT_H
