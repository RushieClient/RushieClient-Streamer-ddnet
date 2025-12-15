#include <base/log.h>

#include <game/client/animstate.h>
#include <game/client/components/chat.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

#include <game/localization.h>
#include <game/version.h>
#include <generated/protocol.h>

#include <engine/shared/config.h>
#include <engine/shared/json.h>

#include "rclient.h"

CRClient::CRClient()
{
	OnReset();
}

void CRClient::OnInit()
{
	FetchRclientVersionCheck();
}

void CRClient::OnConsoleInit()
{
	Console()->Register("ri_find_player_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConFindPlayerFromDdstats, this, "Fetch player from DDstats");
	Console()->Register("ri_find_skin_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConFindSkinFromDdstats, this, "Fetch player's skin from DDstats");
	Console()->Register("ri_copy_skin_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConCopySkinFromDdstats, this, "Fetch and copy player's skin from DDstats");
	Console()->Register("ri_backup_player_profile", "", CFGFLAG_CLIENT, ConBackupPlayerProfile, this, "Backup player profile");
	Console()->Register("ri_tracker_spectator", "", CFGFLAG_CLIENT, ConSpectatorAddTracker, this, "Backup player profile");
	Console()->Register("ri_find_time_on_map", "s[nickname] s[map] ?s[map1] ?s[map2] ?s[map3] ?s[map4] ?s[map5]", CFGFLAG_CLIENT, ConFindTimeMap, this, "Search time on map. Example: ri_find_time_on_map \"[D] Voix\" Grandma");
	Console()->Register("ri_search_map_info", "s[map] ?s[map1] ?s[map2] ?s[map3] ?s[map4] ?s[map5]", CFGFLAG_CLIENT, ConFindMapInfo, this, "Search time on map. Example: ri_search_map_info Grandma");
	Console()->Register("find_hours", "s[nickname] ?s[Wchatflag]", CFGFLAG_CLIENT, ConFindHours, this, "Find hours");
	Console()->Register("+ri_45_degrees", "", CFGFLAG_CLIENT, ConToggle45Degrees, this, "45° bind");
	Console()->Register("+ri_small_sens", "", CFGFLAG_CLIENT, ConToggleSmallSens, this, "Small sens bind");
	Console()->Register("ri_deepfly_toggle", "", CFGFLAG_CLIENT, ConToggleDeepfly, this, "Deep fly toggle");
	Console()->Register("ri_nameplates_editor_update", "", CFGFLAG_CLIENT, ConUpdateNameplatesEditor, this, "Update nameplates. Use after change ri_nameplate_scheme");
	Console()->Register("add_white_list", "s[nickname]", CFGFLAG_CLIENT, ConAddWhiteList, this, "Add player to white list of censor list");
	Console()->Register("find_skin", "r[player]", CFGFLAG_CLIENT, ConFindSkin, this, "Find skin");
	Console()->Register("copy_skin", "r[player]", CFGFLAG_CLIENT, ConCopySkin, this, "Copy skin");
	Console()->Register("find_player", "r[player]", CFGFLAG_CLIENT, ConFindPlayer, this, "Find Player");
	Console()->Register("copy_color", "r[player]", CFGFLAG_CLIENT, ConCopyColor, this, "Copy Color skin");
	Console()->Register("tracker", "r[player]", CFGFLAG_CLIENT, ConTargetPlayerPos, this, "Track player pos");
	Console()->Register("tracker_reset", "", CFGFLAG_CLIENT, ConTargetPlayerPosReset, this, "Reset tracker pos");
	Console()->Register("tracker_remove", "r[player]", CFGFLAG_CLIENT, ConTargetPlayerPosRemove, this, "Remove tracker pos of player");
	Console()->Register("add_censor_list", "r[word]", CFGFLAG_CLIENT, ConAddCensorList, this, "Reset tracker pos");
	Console()->Register("+ri_spec_left", "", CFGFLAG_CLIENT, ConSpecLeft, this, "move camera left in spec");
	Console()->Register("+ri_spec_right", "", CFGFLAG_CLIENT, ConSpecRight, this, "move camera left in spec");
	Console()->Register("+ri_spec_up", "", CFGFLAG_CLIENT, ConSpecUp, this, "move camera left in spec");
	Console()->Register("+ri_spec_down", "", CFGFLAG_CLIENT, ConSpecDown, this, "move camera left in spec");
	Console()->Chain(
		"ri_regex_player_whitelist", [](IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData) {
			if(pResult->NumArguments() == 1)
			{
				auto Re = Regex(pResult->GetString(0));
				if(!Re.error().empty())
				{
					log_error("rclient", "Invalid regex: %s", Re.error().c_str());
					return;
				}
				((CRClient *)pUserData)->m_RegexSplitPlayer = std::move(Re);
			}
			pfnCallback(pResult, pCallbackUserData);
		},
		this);
}

void CRClient::OnRender()
{
	if(m_pRClientDDstatsTask)
	{
		if(m_pRClientDDstatsTask && m_pRClientDDstatsTask->State() == EHttpState::DONE)
		{
			FinishRclientDDstatsProfile();
			ResetRclientDDstatsProfile();
		}
	}
	if(m_pRClientVersionCheck)
	{
		if(m_pRClientVersionCheck && m_pRClientVersionCheck->State() == EHttpState::DONE)
		{
			FinishRclientVersionCheck();
			ResetRclientVersionCheck();
		}
	}
	if(m_pSearchRankOnMapTask)
	{
		if(m_pSearchRankOnMapTask && m_pSearchRankOnMapTask->State() == EHttpState::DONE)
		{
			FinishSearchRankOnMap();
			ResetSearchRankOnMap();
		}
	}
	if(m_pSearchMapInfoTask)
	{
		if(m_pSearchMapInfoTask && m_pSearchMapInfoTask->State() == EHttpState::DONE)
		{
			FinishSearchMapInfo();
			ResetSearchMapInfo();
		}
	}

	if(m_pFindHoursTask)
	{
		if(m_pFindHoursTask && m_pFindHoursTask->State() == EHttpState::DONE)
		{
			FinishFindHours();
			ResetFindHours();
		}
	}

	if(GameClient()->m_Snap.m_SpecInfo.m_Active && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW && !GameClient()->m_AdminPanelRi.IsActive() && g_Config.m_RiSpectatorMoveEnable)
	{
		float Speed = 75.0f * 32.0f * (GameClient()->m_Camera.m_Zoom * 6 / g_Config.m_ClDefaultZoom) * (g_Config.m_RiSpectatorMoveSpeed / 100.0f); // Adjusted for frame-time independence
		float FrameTime = Client()->RenderFrameTime();
		if(m_SpecMoveUp)
			GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].y -= Speed * FrameTime;
		if(m_SpecMoveDown)
			GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].y += Speed * FrameTime;
		if(m_SpecMoveLeft)
			GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].x -= Speed * FrameTime;
		if(m_SpecMoveRight)
			GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].x += Speed * FrameTime;
	}
}

void CRClient::FetchRclientDDstatsProfile()
{
	if(m_pRClientDDstatsTask && !m_pRClientDDstatsTask->Done())
	{
		return;
	}
	char aUrl[256];
	char aEncodedNickname[256];
	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), RclientSearchingNickname);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/profile/json?player=%s", aEncodedNickname);
	m_pRClientDDstatsTask = HttpGet(aUrl);
	m_pRClientDDstatsTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pRClientDDstatsTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientDDstatsTask);
}
void CRClient::FinishRclientDDstatsProfile()
{
	json_value *pJson = m_pRClientDDstatsTask->ResultJson();
	if(!pJson)
	{
		GameClient()->Echo("No that player");
		// Reset all DDstats search flags if JSON parsing fails
		RclientFindPlayerDDstatsSearch = 0;
		RclientFindSkinDDstatsSearch = 0;
		RclientCopySkinDDstatsSearch = 0;
		return;
	}
	const json_value &Json = *pJson;
	const json_value &Nickname = Json["name"];
	const json_value &Clan = Json["clan"];
	const json_value &Country = Json["country"];
	const json_value &Skin = Json["skin_name"];
	const json_value &Skin_color_body = Json["skin_color_body"];
	const json_value &Skin_color_feet = Json["skin_color_feet"];

	if(Nickname.type == json_string)
	{
		char aCountry[32];
		char aBodycolor[32];
		char aFeetcolor[32];
		char aBuf[32];
		int Countryint = Country.u.integer;
		int Skin_color_bodyint = Skin_color_body.u.integer;
		int Skin_color_feetint = Skin_color_feet.u.integer;
		str_format(aCountry, sizeof(aCountry), "%d", Countryint);
		str_format(aBodycolor, sizeof(aBodycolor), "%d", Skin_color_bodyint);
		str_format(aFeetcolor, sizeof(aFeetcolor), "%d", Skin_color_feetint);
		int CustomColor = (str_length(aFeetcolor) > 1 ? 1 : 0);
		if(RclientFindPlayerDDstatsSearch == 1)
		{
			RclientFindPlayerDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Nickname: %s", Nickname.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Clan: %s", Clan.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Country: %s", aCountry);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
		}
		if(RclientFindSkinDDstatsSearch == 1)
		{
			RclientFindSkinDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
		}
		if(RclientCopySkinDDstatsSearch == 1)
		{
			RclientCopySkinDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
			if(g_Config.m_ClDummy == 1)
			{
				str_copy(DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(DummySkinBeforeCopyPlayer));
				DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				str_copy(g_Config.m_ClDummySkin, Skin.u.string.ptr, sizeof(g_Config.m_ClDummySkin));
				g_Config.m_ClDummyUseCustomColor = CustomColor;
				g_Config.m_ClDummyColorBody = Skin_color_bodyint;
				g_Config.m_ClDummyColorFeet = Skin_color_feetint;
				GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(PlayerSkinBeforeCopyPlayer));
				PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				str_copy(g_Config.m_ClPlayerSkin, Skin.u.string.ptr, sizeof(g_Config.m_ClPlayerSkin));
				g_Config.m_ClPlayerUseCustomColor = CustomColor;
				g_Config.m_ClPlayerColorBody = Skin_color_bodyint;
				g_Config.m_ClPlayerColorFeet = Skin_color_feetint;
				GameClient()->SendInfo(false);
			}
		}
	}
	json_value_free(pJson);
}
void CRClient::ResetRclientDDstatsProfile()
{
	if(m_pRClientDDstatsTask)
	{
		m_pRClientDDstatsTask->Abort();
		m_pRClientDDstatsTask = NULL;
	}
}
void CRClient::ConFindPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientFindPlayerDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}
void CRClient::ConFindSkinFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientFindSkinDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}
void CRClient::ConCopySkinFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientCopySkinDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}

bool CRClient::NeedUpdate()
{
	return str_comp(m_aVersionStr, "0") != 0;
}

void CRClient::FetchRclientVersionCheck()
{
	if(m_pRClientVersionCheck && !m_pRClientVersionCheck->Done())
		return;
	char aUrl[256];
	str_copy(aUrl, RCLIENT_VERSION_URL);
	m_pRClientVersionCheck = HttpGet(aUrl);
	m_pRClientVersionCheck->Timeout(CTimeout{20000, 0, 500, 10});
	m_pRClientVersionCheck->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientVersionCheck);
}
void CRClient::ResetRclientVersionCheck()
{
	if(m_pRClientVersionCheck)
	{
		m_pRClientVersionCheck->Abort();
		m_pRClientVersionCheck = NULL;
	}
}
typedef std::tuple<int, int, int> TVersion;
static const TVersion gs_InvalidTCVersion = std::make_tuple(-1, -1, -1);
static TVersion ToTCVersion(char *pStr)
{
	int aVersion[3] = {0, 0, 0};
	const char *p = strtok(pStr, ".");

	for(int i = 0; i < 3 && p; ++i)
	{
		if(!str_isallnum(p))
			return gs_InvalidTCVersion;

		aVersion[i] = str_toint(p);
		p = strtok(NULL, ".");
	}

	if(p)
		return gs_InvalidTCVersion;

	return std::make_tuple(aVersion[0], aVersion[1], aVersion[2]);
}
void CRClient::FinishRclientVersionCheck()
{
	json_value *pJson = m_pRClientVersionCheck->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &CurrentVersion = Json["version"];

	if(CurrentVersion.type == json_string)
	{
		char aNewVersionStr[64];
		str_copy(aNewVersionStr, CurrentVersion);
		char aCurVersionStr[64];
		str_copy(aCurVersionStr, RCLIENT_VERSION);
		if(ToTCVersion(aNewVersionStr) > ToTCVersion(aCurVersionStr))
		{
			str_copy(m_aVersionStr, CurrentVersion);
		}
		else
		{
			m_aVersionStr[0] = '0';
			m_aVersionStr[1] = '\0';
		}
	}
	json_value_free(pJson);
}

//Backup player profile
void CRClient::ConBackupPlayerProfile(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	if(g_Config.m_ClDummy == 1)
	{
		if(str_length(pSelf->DummySkinBeforeCopyPlayer) > 0)
		{
			str_copy(g_Config.m_ClDummySkin, pSelf->DummySkinBeforeCopyPlayer, sizeof(g_Config.m_ClDummySkin));
			g_Config.m_ClDummyUseCustomColor = pSelf->DummyUseCustomColorBeforeCopyPlayer;
			g_Config.m_ClDummyColorBody = pSelf->DummyBodyColorBeforeCopyPlayer;
			g_Config.m_ClDummyColorFeet = pSelf->DummyFeetColorBeforeCopyPlayer;
			pSelf->GameClient()->SendDummyInfo(false);
		}
		else
		{
			pSelf->GameClient()->Echo("There no info of player/skin copy");
		}
	}
	if(g_Config.m_ClDummy == 0)
	{
		if(str_length(pSelf->PlayerSkinBeforeCopyPlayer) > 0)
		{
			str_copy(g_Config.m_ClPlayerSkin, pSelf->PlayerSkinBeforeCopyPlayer, sizeof(g_Config.m_ClPlayerSkin));
			g_Config.m_ClPlayerUseCustomColor = pSelf->PlayerUseCustomColorBeforeCopyPlayer;
			g_Config.m_ClPlayerColorBody = pSelf->PlayerBodyColorBeforeCopyPlayer;
			g_Config.m_ClPlayerColorFeet = pSelf->PlayerFeetColorBeforeCopyPlayer;
			pSelf->GameClient()->SendInfo(false);
		}
		else
		{
			pSelf->GameClient()->Echo("There no info of player/skin copy");
		}
	}
}

// Find map rank
void CRClient::ConFindTimeMap(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *Nickname = pResult->GetString(0);

	char aMapName[256] = {0};
	for(int i = 1; i < pResult->NumArguments(); i++)
	{
		if(i > 1)
			str_append(aMapName, " ", sizeof(aMapName));
		str_append(aMapName, pResult->GetString(i), sizeof(aMapName));
	}

	str_copy(pThis->NicknameForSearch, Nickname, sizeof(pThis->NicknameForSearch));
	str_copy(pThis->MapForSearch, aMapName, sizeof(pThis->MapForSearch));
	pThis->FetchSearchRankOnMap();
}

void CRClient::FetchSearchRankOnMap()
{
	if(m_pSearchRankOnMapTask && !m_pSearchRankOnMapTask->Done())
		return;
	char aUrl[256];
	char aEncodedNickname[256];

	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), NicknameForSearch);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/player/json?player=%s", aEncodedNickname);
	m_pSearchRankOnMapTask = HttpGet(aUrl);
	m_pSearchRankOnMapTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pSearchRankOnMapTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pSearchRankOnMapTask);
}
void CRClient::FinishSearchRankOnMap()
{
	json_value *pJson = m_pSearchRankOnMapTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &Finishes = Json["finishes"];
	bool bFound = false;
	for(unsigned int i = 0; i < Finishes.u.object.length; ++i)
	{
		const json_value &Finish = Finishes[i];
		const json_value &MapInfo = Finish["map"];
		const json_value &MapName = MapInfo["map"];
		const char *pMapName = MapName;
		if(str_find_nocase(pMapName, MapForSearch))
		{
			const json_value &Time = Finish["time"];
			char aBuf[128];
			char TimeStr[32];
			if(Time.type == json_double || Time.type == json_integer)
			{
				double FinishTimeSec = (Time.type == json_double) ? Time.u.dbl : (double)Time.u.integer;
				int hours = static_cast<int>(FinishTimeSec) / 3600;
				int minutes = (static_cast<int>(FinishTimeSec) % 3600) / 60;
				int seconds = static_cast<int>(FinishTimeSec) % 60;
				str_format(TimeStr, sizeof(TimeStr), "%02d:%02d:%02d", hours, minutes, seconds);
				str_format(aBuf, sizeof(aBuf), "%s finished %s for %s", NicknameForSearch, pMapName, TimeStr);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
				GameClient()->Echo(aBuf);
			}
			else
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", "Finish time not found");
				GameClient()->Echo("Finish time not found");
			}
			bFound = true;
			break;
		}
		if(bFound)
			break;
	}
	if(!bFound)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Map '%s' not found for %s", MapForSearch, NicknameForSearch);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
		GameClient()->Echo(aBuf);
	}
	json_value_free(pJson);
}
void CRClient::ResetSearchRankOnMap()
{
	if(m_pSearchRankOnMapTask)
	{
		m_pSearchRankOnMapTask->Abort();
		m_pSearchRankOnMapTask = NULL;
	}
}

// Search map info
void CRClient::ConFindMapInfo(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	char aMapName[256] = {0};
	for(int i = 0; i < pResult->NumArguments(); i++)
	{
		if(i > 0)
			str_append(aMapName, " ", sizeof(aMapName));
		str_append(aMapName, pResult->GetString(i), sizeof(aMapName));
	}

	str_copy(pThis->MapForSearchMapInfo, aMapName, sizeof(pThis->MapForSearchMapInfo));
	pThis->FetchSearchMapInfo();
}

void CRClient::FetchSearchMapInfo()
{
	if(m_pSearchMapInfoTask && !m_pSearchMapInfoTask->Done())
		return;
	char aUrl[256];

	// URL encode the nickname
	str_copy(aUrl, "https://ddstats.tw/maps/json");
	m_pSearchMapInfoTask = HttpGet(aUrl);
	m_pSearchMapInfoTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pSearchMapInfoTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pSearchMapInfoTask);
}
void CRClient::FinishSearchMapInfo()
{
	json_value *pJson = m_pSearchMapInfoTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &Maps = Json;
	bool bFound = false;
	for(unsigned int i = 0; i < Maps.u.object.length; ++i)
	{
		const json_value &Map = Maps[i];
		const char *pMapName = Map["map"];
		if(str_find_nocase(pMapName, MapForSearchMapInfo))
		{
			const json_value &Points = Map["points"];
			const json_value &Stars = Map["stars"];
			const json_value &Difficulty = Map["server"];
			const char *pDifficulty = (Difficulty.type == json_string) ? Difficulty.u.string.ptr : "Unknown";
			char aBuf[256];
			if(Points.type == json_integer && Stars.type == json_integer)
			{
				int PointsFormatted = Points.u.integer;
				int StarsFormatted = Stars.u.integer;
				int StarsFormattedMinus = 5 - Stars.u.integer;
				char aStars[16] = {0};
				for(int s = 0; s < StarsFormatted; s++)
					str_append(aStars, "★", sizeof(aStars));
				for(int s = 0; s < StarsFormattedMinus; s++)
					str_append(aStars, "✰", sizeof(aStars));
				str_format(aBuf, sizeof(aBuf), "Map: %s, Points: %i, Difficulty: %s %s", pMapName, PointsFormatted, pDifficulty, aStars);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
				GameClient()->Echo(aBuf);
			}
			else
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", "Map info not found");
				GameClient()->Echo("Map info not found");
			}
			bFound = true;
			break;
		}
		if(bFound)
			break;
	}
	if(!bFound)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Map '%s' not found", MapForSearch);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
		GameClient()->Echo(aBuf);
	}
	json_value_free(pJson);
}
void CRClient::ResetSearchMapInfo()
{
	if(m_pSearchMapInfoTask)
	{
		m_pSearchMapInfoTask->Abort();
		m_pSearchMapInfoTask = NULL;
	}
}

// Tracker
bool CRClient::IsTracked(int ClientId)
{
	// Check if the client is being tracked
	for(int i = 0; i < TargetCount; i++)
	{
		if(ClientId == TargetPositionId[i])
		{
			return true;
		}
	}
	return false;
}

void CRClient::ConSpectatorAddTracker(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	char PlayerName[32];
	int PlayerId = -1;
	if(pThis->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW && pThis->GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		const auto &Player = pThis->GameClient()->m_aClients[pThis->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId];
		str_copy(PlayerName, Player.m_aName, sizeof(PlayerName));
		PlayerId = Player.ClientId();
	}
	else
		pThis->GameClient()->Echo("You're not spectating the player");
	if(PlayerId != -1)
	{
		if(!pThis->IsTracked(PlayerId))
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "tracker %s", PlayerName);
			pThis->GameClient()->Console()->ExecuteLine(aBuf);
		}
		else
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "tracker_remove %s", PlayerName);
			pThis->GameClient()->Console()->ExecuteLine(aBuf);
		}
	}
}

//Warlist
bool CRClient::IsInWarlist(int ClientId, int Index)
{
	CWarDataCache &WarData = GameClient()->m_WarList.m_WarPlayers[ClientId];
	for(int i = 0; i < (int)WarData.m_WarGroupMatches.size(); i++)
	{
		if(WarData.m_WarGroupMatches[i])
		{
			if(Index == i)
				return true;
		}
	}
	return false;
}

//Find hours
void CRClient::ConFindHours(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	// Return if a find_hours request is already in progress
	if(pSelf->m_pFindHoursTask && !pSelf->m_pFindHoursTask->Done())
	{
		pSelf->GameClient()->Echo("Request already in progress");
		return;
	}
	const char *pNickname = pResult->GetString(0);
	const char *pWriteinchat = pResult->GetString(1);

	pSelf->FetchFindHours(pNickname, pWriteinchat);
}

void CRClient::FetchFindHours(const char *pNickname, const char *pWriteinchat)
{
	if(m_pFindHoursTask && !m_pFindHoursTask->Done())
		return;

	if(!pNickname)
		return;
	else
		str_copy(m_aFindHoursPlayer, pNickname, sizeof(m_aFindHoursPlayer));

	if((str_comp("W", pWriteinchat) == 0) || ((str_comp("w", pWriteinchat) == 0)))
		m_WriteFindHoursInChat = true;
	else
		m_WriteFindHoursInChat = false;
	char aUrl[256];
	char aEncodedNickname[256];
	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), pNickname);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/player/json?player=%s", aEncodedNickname);
	m_pFindHoursTask = HttpGet(aUrl);
	m_pFindHoursTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pFindHoursTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pFindHoursTask);
}

// Added helper to finish async find_hours
void CRClient::FinishFindHours()
{
	json_value *pJson = m_pFindHoursTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &General = Json["general_activity"];
	const json_value &Profile = Json["profile"];
	if(General.type == json_object && Profile.type == json_object)
	{
		const json_value &Seconds = General["total_seconds_played"];
		const json_value &Points = Profile["points"];
		if(Seconds.type == json_integer && Points.type == json_integer)
		{
			int Hours = Seconds.u.integer / 3600;
			int Pointsfinal = Points.u.integer;
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Player %s has %d hours and %d points", m_aFindHoursPlayer, Hours, Pointsfinal);
			GameClient()->Echo(aBuf);
			if(m_WriteFindHoursInChat)
				GameClient()->m_Chat.SendChat(0, aBuf);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", aBuf);
		}
		else
		{
			GameClient()->Echo("Invalid 'total_seconds_played' in JSON");
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", "Invalid 'total_seconds_played' in JSON");
		}
	}
	else
	{
		GameClient()->Echo("Invalid 'general_activity' in JSON");
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", "Invalid 'general_activity' in JSON");
	}
	json_value_free(pJson);
}

void CRClient::ResetFindHours()
{
	if(m_pFindHoursTask)
	{
		m_pFindHoursTask->Abort();
		m_pFindHoursTask = NULL;
	}
}

//45 degrees
void CRClient::ConToggle45Degrees(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_45degreestoggle = pResult->GetInteger(0) != 0;
	if(!g_Config.m_RiToggle45degrees)
	{
		if(pSelf->m_45degreestoggle && !pSelf->m_45degreestogglelastinput)
		{
			pSelf->GameClient()->Echo("[[green]] 45° on");
			pSelf->m_45degreesEnabled = 1;
			g_Config.m_RiPrevInpMousesens45degrees = (pSelf->m_SmallsensEnabled == 1 ? g_Config.m_RiPrevInpMousesensSmallsens : g_Config.m_InpMousesens);
			g_Config.m_RiPrevMouseMaxDistance45degrees = g_Config.m_ClMouseMaxDistance;
			g_Config.m_ClMouseMaxDistance = 2;
			g_Config.m_InpMousesens = 4;
		}
		else if(!pSelf->m_45degreestoggle)
		{
			pSelf->m_45degreesEnabled = 0;
			pSelf->GameClient()->Echo("[[red]] 45° off");
			g_Config.m_ClMouseMaxDistance = g_Config.m_RiPrevMouseMaxDistance45degrees;
			g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesens45degrees;
		}
		pSelf->m_45degreestogglelastinput = pSelf->m_45degreestoggle;
	}

	if(g_Config.m_RiToggle45degrees)
	{
		if(pSelf->m_45degreestoggle && !pSelf->m_45degreestogglelastinput)
		{
			if(g_Config.m_ClMouseMaxDistance == 2)
			{
				pSelf->m_45degreesEnabled = 0;
				pSelf->GameClient()->Echo("[[red]] 45° off");
				g_Config.m_ClMouseMaxDistance = g_Config.m_RiPrevMouseMaxDistance45degrees;
				g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesens45degrees;
			}
			else
			{
				pSelf->m_45degreesEnabled = 1;
				pSelf->GameClient()->Echo("[[green]] 45° on");
				g_Config.m_RiPrevInpMousesens45degrees = (pSelf->m_SmallsensEnabled == 1 ? g_Config.m_RiPrevInpMousesensSmallsens : g_Config.m_InpMousesens);
				g_Config.m_RiPrevMouseMaxDistance45degrees = g_Config.m_ClMouseMaxDistance;
				g_Config.m_ClMouseMaxDistance = 2;
				g_Config.m_InpMousesens = 4;
			}
		}
		pSelf->m_45degreestogglelastinput = pSelf->m_45degreestoggle;
	}
}
//Small sens toggle
void CRClient::ConToggleSmallSens(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_Smallsenstoggle = pResult->GetInteger(0) != 0;
	if(!g_Config.m_RiToggleSmallSens)
	{
		if(pSelf->m_Smallsenstoggle && !pSelf->m_Smallsenstogglelastinput)
		{
			pSelf->m_SmallsensEnabled = 1;
			pSelf->GameClient()->Echo("[[green]] small sens on");
			g_Config.m_RiPrevInpMousesensSmallsens = (pSelf->m_45degreesEnabled == 1 ? g_Config.m_RiPrevInpMousesens45degrees : g_Config.m_InpMousesens);
			g_Config.m_InpMousesens = 1;
		}
		else if(!pSelf->m_Smallsenstoggle)
		{
			pSelf->m_SmallsensEnabled = 0;
			pSelf->GameClient()->Echo("[[red]] small sens off");
			g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesensSmallsens;
		}
		pSelf->m_Smallsenstogglelastinput = pSelf->m_Smallsenstoggle;
	}

	if(g_Config.m_RiToggleSmallSens)
	{
		if(pSelf->m_Smallsenstoggle && !pSelf->m_Smallsenstogglelastinput)
		{
			if(g_Config.m_InpMousesens == 1)
			{
				pSelf->m_SmallsensEnabled = 0;
				pSelf->GameClient()->Echo("[[red]] small sens off");
				g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesensSmallsens;
			}
			else
			{
				pSelf->m_SmallsensEnabled = 1;
				pSelf->GameClient()->Echo("[[green]] small sens on");
				g_Config.m_RiPrevInpMousesensSmallsens = (pSelf->m_45degreesEnabled == 1 ? g_Config.m_RiPrevInpMousesens45degrees : g_Config.m_InpMousesens);
				g_Config.m_InpMousesens = 1;
			}
		}
		pSelf->m_Smallsenstogglelastinput = pSelf->m_Smallsenstoggle;
	}
}

//Deepfly
void CRClient::ConToggleDeepfly(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	char CurBind[128];
	str_copy(CurBind, pSelf->GameClient()->m_Binds.Get(291, 0), sizeof(CurBind));
	if(str_find_nocase(CurBind, "+toggle cl_dummy_hammer"))
	{
		pSelf->GameClient()->Echo("[[red]] Deepfly off");
		if(str_length(pSelf->m_Oldmouse1Bind) > 1)
			pSelf->GameClient()->m_Binds.Bind(291, pSelf->m_Oldmouse1Bind, false, 0);
		else
		{
			pSelf->GameClient()->Echo("[[red]] No old bind in memory. Binding +fire");
			pSelf->GameClient()->m_Binds.Bind(291, "+fire", false, 0);
		};
	}
	else
	{
		pSelf->GameClient()->Echo("[[green]] Deepfly on");
		str_copy(pSelf->m_Oldmouse1Bind, CurBind, sizeof(CurBind));
		pSelf->GameClient()->m_Binds.Bind(291, "+fire; +toggle cl_dummy_hammer 1 0", false, 0);
	}
}

//Nameplates
void CRClient::ConUpdateNameplatesEditor(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->GameClient()->m_NamePlates.RiResetNameplatesPos(*pSelf->GameClient(), g_Config.m_RiNamePlateScheme);
}

std::vector<std::string> CRClient::SplitRegex(const char *aboba)
{
	std::vector<std::string> parts;
	std::string str(aboba);

	size_t start = 0;
	size_t end = 0;

	while((end = str.find('|', start)) != std::string::npos)
	{
		parts.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(str.substr(start));

	return parts;
}

std::vector<std::string> CRClient::SplitWords(const char *aboba)
{
	std::vector<std::string> parts;
	std::string str(aboba);

	size_t start = 0;
	size_t end = 0;

	while((end = str.find(' ', start)) != std::string::npos)
	{
		parts.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(str.substr(start));

	return parts;
}

// void CRClient::FinishRClientUsersSend()
// {
// 	json_value *pJson = m_pRClientUsersTask->ResultJson();
// 	if(!pJson)
// 		return;
//
// 	json_value_free(pJson);
// }
void CRClient::ConAddWhiteList(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	char aBuf[256];
	if(aInput[0])
	{
		if(g_Config.m_RiRegexPlayerWhitelist[0])
		{
			char aNewRegex[512];
			str_format(aBuf, sizeof(aBuf), "Added to existing regex: %s", aInput);
			str_format(aNewRegex, sizeof(aNewRegex), "%s|%s", g_Config.m_RiRegexPlayerWhitelist, aInput);
			str_copy(g_Config.m_RiRegexPlayerWhitelist, aNewRegex, sizeof(g_Config.m_RiRegexPlayerWhitelist));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
		else
		{
			str_copy(g_Config.m_RiRegexPlayerWhitelist, aInput, sizeof(g_Config.m_RiRegexPlayerWhitelist));
			str_format(aBuf, sizeof(aBuf), "New regex added: %s", aInput);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", "No word in this");
	}
}

void CRClient::ConFindSkin(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}
	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin Name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::CopySkin(const char *Nickname)
{
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(GameClient()->m_aClients[i].m_aName, Nickname) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(Nickname);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin Name: %s", ClientData.m_aSkinName);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			if(g_Config.m_ClDummy == 1)
			{
				str_copy(DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(DummySkinBeforeCopyPlayer));
				DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				str_copy(g_Config.m_ClDummySkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClDummySkin));
				g_Config.m_ClDummyUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = ClientData.m_ColorBody;
				g_Config.m_ClDummyColorFeet = ClientData.m_ColorFeet;
				GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(PlayerSkinBeforeCopyPlayer));
				PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				str_copy(g_Config.m_ClPlayerSkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClPlayerSkin));
				g_Config.m_ClPlayerUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = ClientData.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = ClientData.m_ColorFeet;
				GameClient()->SendInfo(false);
			}
		}
		else
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConCopySkin(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pSelf->CopySkin(aInput);
}

void CRClient::ConFindPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Clan: %s", ClientData.m_aClan);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Country: %d",
				ClientData.m_Country);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConCopyColor(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			if(g_Config.m_ClDummy == 1)
			{
				str_copy(pSelf->DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(pSelf->DummySkinBeforeCopyPlayer));
				pSelf->DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				pSelf->DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				pSelf->DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				g_Config.m_ClDummyUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = ClientData.m_ColorBody;
				g_Config.m_ClDummyColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(pSelf->PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(pSelf->PlayerSkinBeforeCopyPlayer));
				pSelf->PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				pSelf->PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				pSelf->PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				g_Config.m_ClPlayerUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = ClientData.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendInfo(false);
			}
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::TargetPlayerPosAdd(const char *Nickname)
{
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(GameClient()->m_aClients[i].m_aName, Nickname) == 0)
		{
			ClientID = i;
			// Find first free slot
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(TargetPositionId[j] == -1)
				{
					str_copy(TargetPositionNickname[j], GameClient()->m_aClients[ClientID].m_aName);
					TargetPositionId[j] = ClientID;
					TargetCount++;
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%s added to target list", GameClient()->m_aClients[ClientID].m_aName);
					GameClient()->Echo(aBuf);
					break;
				}
			}
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(Nickname);
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && GameClient()->m_aClients[ClientID].m_aName[0] != '\0')
		{
			// Find first free slot
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(TargetPositionId[j] == -1)
				{
					str_copy(TargetPositionNickname[j], GameClient()->m_aClients[ClientID].m_aName);
					TargetPositionId[j] = ClientID;
					TargetCount++;
					break;
				}
			}
		}
		else
		{
			GameClient()->Echo("Invalid player ID or player not found");
			dbg_msg("Search player", "Invalid player ID or player not found");
		}
	}
}

void CRClient::TargetPlayerPosRemove(const char *Nickname)
{
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(GameClient()->m_aClients[i].m_aName, Nickname) == 0)
		{
			ClientID = i;
			// Find first free slot
			for(int j = 0; j < TargetCount; j++)
			{
				if(str_comp_nocase(TargetPositionNickname[j], GameClient()->m_aClients[ClientID].m_aName) == 0)
				{
					TargetCount--;

					// Shift remaining targets to fill the gap
					for(int k = j; k < TargetCount; k++)
					{
						TargetPositionId[k] = TargetPositionId[k + 1];
						str_copy(TargetPositionNickname[k], TargetPositionNickname[k + 1], sizeof(TargetPositionNickname[k]));
					}

					// Clear the last slot
					TargetPositionId[TargetCount] = -1;
					TargetPositionNickname[TargetCount][0] = '\0';
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%s removed from target list", GameClient()->m_aClients[ClientID].m_aName);
					GameClient()->Echo(aBuf);
					return;
				}
			}
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(Nickname);
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && GameClient()->m_aClients[ClientID].m_aName[0] != '\0')
		{
			// Find matching slot
			for(int j = 0; j < TargetCount; j++)
			{
				if(TargetPositionId[j] == ClientID)
				{
					TargetCount--;

					// Shift remaining targets to fill the gap
					for(int k = j; k < TargetCount; k++)
					{
						TargetPositionId[k] = TargetPositionId[k + 1];
						str_copy(TargetPositionNickname[k], TargetPositionNickname[k + 1], sizeof(TargetPositionNickname[k]));
					}

					// Clear the last slot
					TargetPositionId[TargetCount] = -1;
					TargetPositionNickname[TargetCount][0] = '\0';

					GameClient()->Echo("Player removed from target list");
					return;
				}
			}
			GameClient()->Echo("Player not in target list");
		}
		else
		{
			GameClient()->Echo("Invalid player ID or player not found");
			dbg_msg("Search player", "Invalid player ID or player not found");
		}
	}
}

void CRClient::TargetPlayerPosReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		TargetPositionId[i] = -1;
		TargetPositionNickname[i][0] = '\0';
	}
	TargetCount = 0;
	GameClient()->Echo("Target list reset");
}

void CRClient::ConTargetPlayerPos(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	if(pResult->NumArguments() == 0)
	{
		return;
	}
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pSelf->TargetPlayerPosAdd(aInput);
}

void CRClient::ConTargetPlayerPosReset(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	pSelf->TargetPlayerPosReset();
}

void CRClient::ConTargetPlayerPosRemove(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pSelf->TargetPlayerPosRemove(aInput);
}

void CRClient::ConAddCensorList(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	char aBuf[256];
	if(aInput[0])
	{
		if(g_Config.m_TcRegexChatIgnore[0])
		{
			char aNewRegex[1024];
			str_format(aBuf, sizeof(aBuf), "Added to existing regex: %s", aInput);
			str_format(aNewRegex, sizeof(aNewRegex), "%s|%s", g_Config.m_TcRegexChatIgnore, aInput);
			str_copy(g_Config.m_TcRegexChatIgnore, aNewRegex, sizeof(g_Config.m_TcRegexChatIgnore));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
		else
		{
			str_copy(g_Config.m_TcRegexChatIgnore, aInput, sizeof(g_Config.m_TcRegexChatIgnore));
			str_format(aBuf, sizeof(aBuf), "New regex added: %s", aInput);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", "No word in this");
	}
}

//Camera move
void CRClient::ConSpecLeft(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_SpecMoveLeft = pResult->GetInteger(0) != 0;
}
void CRClient::ConSpecRight(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_SpecMoveRight = pResult->GetInteger(0) != 0;
}
void CRClient::ConSpecUp(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_SpecMoveUp = pResult->GetInteger(0) != 0;
}
void CRClient::ConSpecDown(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_SpecMoveDown = pResult->GetInteger(0) != 0;
}

void CRClient::RclientOnDummyChange(bool DummyConnected)
{
	if(g_Config.m_PlayerClanAutoChange)
	{
		const char *pTargetClan = DummyConnected ? g_Config.m_PlayerClanWithDummy : g_Config.m_PlayerClanNoDummy;
		if(str_comp(g_Config.m_PlayerClan, pTargetClan) != 0)
		{
			str_copy(g_Config.m_PlayerClan, pTargetClan, sizeof(g_Config.m_PlayerClan));
			if(Client()->State() == IClient::STATE_ONLINE)
			{
				GameClient()->SendInfo(false);
			}
		}
	}
}

void CRClient::RclientOnPlayerChange(bool Connected)
{
	// Initialize all slots to -1 to mark them as free
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		TargetPositionId[i] = -1;
		TargetPositionNickname[i][0] = '\0';
	}
	TargetCount = 0;
	if(g_Config.m_PlayerClanAutoChange)
	{
		str_copy(g_Config.m_PlayerClan, g_Config.m_PlayerClanNoDummy, sizeof(g_Config.m_PlayerClan));
	}
}

std::string CRClient::encodeUTF8(const std::string &Input)
{
	std::ostringstream Encoded;
	const char *ptr = Input.c_str();

	while(*ptr != '\0')
	{
		int CodePoint = str_utf8_decode(&ptr);
		if(CodePoint < 0)
		{
			Encoded << "-ERROR-";
			break;
		}

		if(CodePoint <= 127 && std::isalnum(static_cast<unsigned char>(CodePoint)))
		{
			Encoded << static_cast<char>(CodePoint);
		}
		else
		{
			Encoded << '-' << CodePoint << '-';
		}
	}

	return Encoded.str();
}

float CRClient::GetScoreboardHeight(bool IsDefaultRender ,bool IsBigger, int ClientId)
{
	// Default: m_ScoreboardPopupContext.m_IsLocal ? 30.0f : 60.0f
	// Default: m_ScoreboardPopupContext.m_IsLocal ? 58.5f : 87.5f
	constexpr float OuterPopupPadding = 2.0f * (1.0f + 4.0f); // popup border + margin on both sides
	constexpr float InnerMargin = 10.0f; // View.Margin(5.0f) inside PopupScoreboard
	constexpr float LabelHeight = 12.0f;
	constexpr float ItemSpacing = 2.0f;
	constexpr float ButtonHeight = 17.5f;
	constexpr float QuickActionHeight = 25.0f + ItemSpacing * 2.0f; // height of one quick-action row including spacing

	const int LocalId = GameClient()->m_aLocalIds[g_Config.m_ClDummy];
	const int LocalTeam = GameClient()->m_Teams.Team(LocalId);
	const int TargetTeam = GameClient()->m_Teams.Team(ClientId);
	const bool LocalInTeam = LocalTeam != TEAM_FLOCK && LocalTeam != TEAM_SUPER;
	const bool TargetInTeam = TargetTeam != TEAM_FLOCK && TargetTeam != TEAM_SUPER;
	const bool LocalIsTarget = LocalId == ClientId;
	int ExtraButtonRows = 0;
	if(LocalInTeam && LocalTeam == TargetTeam)
		ExtraButtonRows++; // Exit
	if(TargetInTeam && LocalTeam != TargetTeam)
		ExtraButtonRows++; // Join
	if(LocalInTeam && TargetTeam != LocalTeam)
		ExtraButtonRows++; // Invite
	if(!LocalIsTarget && LocalInTeam && TargetTeam == LocalTeam)
		ExtraButtonRows++; // Kick
	if(LocalInTeam && LocalTeam == TargetTeam)
		ExtraButtonRows++; // Lock

	// Both popup entry points currently render the same stack of buttons.
	const int ButtonRows = (IsDefaultRender ? 8 : 7) + ExtraButtonRows;

	float ScoreboardHeight = OuterPopupPadding + InnerMargin + LabelHeight;
	if(IsBigger)
	{
		ScoreboardHeight += QuickActionHeight * 2.0f; // friend/mute/emote + tracker/team/war
	}
	ScoreboardHeight += ButtonRows * (ButtonHeight + ItemSpacing * 2.0f);

	if (ExtraButtonRows != 0)
		ScoreboardHeight += ItemSpacing * 4.0f;

	return ScoreboardHeight;
}
