#ifndef GAME_CLIENT_COMPONENTS_RCLIENT_RUSHIE_H
#define GAME_CLIENT_COMPONENTS_RCLIENT_RUSHIE_H

#include "engine/external/regex.h"

#include <engine/shared/console.h>
#include <engine/shared/http.h>

#include <game/client/component.h>

class CRClient : public CComponent
{
	static void ConFindPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConFindSkinFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConCopySkinFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConBackupPlayerProfile(IConsole::IResult *pResult, void *pUserData);

	static void ConSpectatorAddTracker(IConsole::IResult *pResult, void *pUserData);

	static void ConFindTimeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConFindMapInfo(IConsole::IResult *pResult, void *pUserData);

	static void ConFindHours(IConsole::IResult *pResult, void *pUserData);

	static void ConToggle45Degrees(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleSmallSens(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleDeepfly(IConsole::IResult *pResult, void *pUserData);

	static void ConUpdateNameplatesEditor(IConsole::IResult *pResult, void *pUserData);

	static void ConAddWhiteList(IConsole::IResult *pResult, void *pUserData);

	//Camera move
	static void ConSpecLeft(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecRight(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecUp(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecDown(IConsole::IResult *pResult, void *pUserData);
	bool m_SpecMoveLeft = false;
	bool m_SpecMoveRight = false;
	bool m_SpecMoveUp = false;
	bool m_SpecMoveDown = false;


	//45 degrees
	int m_45degreestoggle = 0;
	int m_45degreestogglelastinput = 0;
	int m_45degreesEnabled = 0;
	// Small sens
	int m_Smallsenstoggle = 0;
	int m_Smallsenstogglelastinput = 0;
	int m_SmallsensEnabled = 0;
	//Deepfly
	char m_Oldmouse1Bind[128];

	// GetInfofromDDstats
	std::shared_ptr<CHttpRequest> m_pRClientDDstatsTask = nullptr;
	void FetchRclientDDstatsProfile();
	void FinishRclientDDstatsProfile();
	void ResetRclientDDstatsProfile();
	char RclientSearchingNickname[16];
	int RclientFindSkinDDstatsSearch = 0;
	int RclientCopySkinDDstatsSearch = 0;
	int RclientFindPlayerDDstatsSearch = 0;

	static void ConFindSkin(IConsole::IResult *pResult, void *pUserData);
	static void ConCopySkin(IConsole::IResult *pResult, void *pUserData);
	static void ConFindPlayer(IConsole::IResult *pResult, void *pUserData);
	static void ConCopyColor(IConsole::IResult *pResult, void *pUserData);
	static void ConTargetPlayerPos(IConsole::IResult *pResult, void *pUserData);
	static void ConTargetPlayerPosReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTargetPlayerPosRemove(IConsole::IResult *pResult, void *pUserData);
	static void ConAddCensorList(IConsole::IResult *pResult, void *pUserData);
public:
	CRClient();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnConsoleInit() override;
	void OnRender() override;

	static constexpr const char *RCLIENT_URL = "https://rushie-client.ru";
	static constexpr const char *RCLIENT_VERSION_URL = "https://server.rushie-client.ru/version";
	char m_aVersionStr[10] = "0";

	std::shared_ptr<CHttpRequest> m_pRClientVersionCheck = nullptr;
	void FetchRclientVersionCheck();
	void FinishRclientVersionCheck();
	void ResetRclientVersionCheck();
	int RclientVersionCheckDone = 0;
	bool NeedUpdate();

	// Find map rank
	std::shared_ptr<CHttpRequest> m_pSearchRankOnMapTask = nullptr;
	void FetchSearchRankOnMap();
	void FinishSearchRankOnMap();
	void ResetSearchRankOnMap();
	char NicknameForSearch[32];
	char MapForSearch[128];

	// Search map info
	std::shared_ptr<CHttpRequest> m_pSearchMapInfoTask = nullptr;
	void FetchSearchMapInfo();
	void FinishSearchMapInfo();
	void ResetSearchMapInfo();
	char MapForSearchMapInfo[128];

	//Back player profile after copy player
	char PlayerSkinBeforeCopyPlayer[42];
	int PlayerUseCustomColorBeforeCopyPlayer = 0;
	int PlayerBodyColorBeforeCopyPlayer = 0;
	int PlayerFeetColorBeforeCopyPlayer = 0;
	char DummySkinBeforeCopyPlayer[42];
	int DummyUseCustomColorBeforeCopyPlayer = 0;
	int DummyBodyColorBeforeCopyPlayer = 0;
	int DummyFeetColorBeforeCopyPlayer = 0;

	//Tracker
	int TargetPositionId[MAX_CLIENTS];
	char TargetPositionNickname[MAX_CLIENTS][32];
	int TargetCount = 0;
	bool IsTracked(int ClientId);

	//WarList
	bool IsInWarlist(int ClientId, int Index);

	//FindHours
	//Async find_hours task and data
	std::shared_ptr<CHttpRequest> m_pFindHoursTask;
	char m_aFindHoursPlayer[32];
	// Flag to indicate if FindHours output should be written in chat
	bool m_WriteFindHoursInChat;
	void FetchFindHours(const char *pNickname, const char *pWriteinchat);
	void FinishFindHours();
	void ResetFindHours();

	// Regex
	static std::vector<std::string> SplitRegex(const char *aboba);
	static std::vector<std::string> SplitWords(const char *MSG);
	Regex m_RegexSplitPlayer;
};

#endif
