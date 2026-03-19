#ifndef RCLIENT_MUSIC_ISLAND_H
#define RCLIENT_MUSIC_ISLAND_H
#include "engine/graphics.h"
#include "engine/image.h"
#include "engine/console.h"
#include "game/client/component.h"
#include "game/client/ui_rect.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class CMusicIsland : public CComponent
{
	static void ConShowCurInfo(IConsole::IResult *pResult, void *pUserData);

	struct SMusicInfo
	{
		bool m_Available = false;
		bool m_Playing = false;
		std::string m_Title;
		std::string m_Artist;
		std::string m_Album;
	};

	bool m_Extended = false;

	CUIRect m_Rect;

	std::thread m_InfoWorker;
	std::atomic<bool> m_InfoWorkerRunning = false;
	std::atomic<bool> m_InfoWorkerStopRequested = false;
	std::thread m_ImageWorker;
	std::atomic<bool> m_ImageWorkerRunning = false;
	std::atomic<bool> m_ImageWorkerStopRequested = false;
	int64_t m_NextInfoUpdateTime = 0;
	mutable std::mutex m_MusicInfoMutex;
	SMusicInfo m_MusicInfo;
	CImageInfo m_PendingMusicImage;
	std::string m_CurrentArtworkKey;
	bool m_MusicImageDirty = false;
	IGraphics::CTextureHandle m_MusicImageTexture;
	int m_MusicImageWidth = 0;
	int m_MusicImageHeight = 0;

	void ResetMusicInfo();
	void ResetMusicImage();
	SMusicInfo GetMusicInfo() const;
	void RenderMusicIsland();
	void RenderMusicIslandImage(CUIRect *pBase);
	void RenderMusicIslandVisualizer(CUIRect *pBase);
	void RenderMusicIslandMain(CUIRect *pBase);
	void UpdateMusicImageTexture();
	void StartInfoWorker(int64_t Now);
	void StopInfoWorker();
	void StopImageWorker();
	void InfoWorkerLoop();
	void UpdateMusicInfo();

public:
	CMusicIsland();
	~CMusicIsland() override;
	int Sizeof() const override { return sizeof(*this); }

	void SetExtended(bool Extended);

	void OnReset() override;
	void OnRender() override;
	void OnConsoleInit() override;
	void OnShutdown() override;

	bool IsActive() const;
};

#endif //RCLIENT_MUSIC_ISLAND_H
