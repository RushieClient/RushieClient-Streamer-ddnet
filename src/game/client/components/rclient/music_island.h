#ifndef RCLIENT_MUSIC_ISLAND_H
#define RCLIENT_MUSIC_ISLAND_H
#include "game/client/component.h"
#include "game/client/ui_rect.h"
#if defined(CONF_FAMILY_WINDOWS)
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.Core.h>
#endif


class CMusicIsland : public CComponent
{
	bool m_Extended = false;

	CUIRect m_Rect;



	void RIMusicReset()
	{
	}

	void RenderMusicIsland();

public:
	CMusicIsland();
	int Sizeof() const override { return sizeof(*this); }

	void SetExtended(bool Extended);

	void OnReset() override;
	void OnRender() override;
	void OnConsoleInit() override;
	void OnRelease() override;

	bool IsActive() const;
};

#endif //RCLIENT_MUSIC_ISLAND_H
