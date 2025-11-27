#include "rclient_indicator.h"
#include "engine/client.h"
#include "game/client/gameclient.h"

void CRClientIndicator::OnInit()
{
}

void CRClientIndicator::OnShutdown()
{
	DisconnectFromServer();
}

void CRClientIndicator::OnRender()
{
	if(!g_Config.m_RiShowRclientIndicator)
		return;

	if(m_PrevClientState != IClient::STATE_ONLINE && Client()->State() == IClient::STATE_ONLINE)
	{
		if(!m_IsConnected)
		{
			ConnectToServer();
		}
		else if(m_TokenReceived)
		{
			RegisterPlayer();
		}
	}
	else if(m_PrevClientState == IClient::STATE_ONLINE && Client()->State() != IClient::STATE_ONLINE)
	{
		// Unregister from old server
		if(m_IsConnected && m_Socket.socket())
		{
			m_Socket.socket()->emit("unregister_player");
			dbg_msg("RClient", "Unregistering from server");
		}
	}
	else if(Client()->State() == IClient::STATE_ONLINE && m_IsConnected && m_TokenReceived)
	{
		if(GameClient()->m_aLocalIds[0] != m_PlayerId)
		{
			m_PlayerId = GameClient()->m_aLocalIds[0];
			RegisterPlayer();
		}
		if(Client()->DummyConnected() && GameClient()->m_aLocalIds[1] != m_DummyId)
		{
			m_DummyId = GameClient()->m_aLocalIds[1];
			RegisterPlayer();
		}
	}

	m_PrevClientState = Client()->State();
}

void CRClientIndicator::ConnectToServer()
{
	if(m_IsConnected)
		return;

	m_Socket.set_open_listener([this]() {
		m_IsConnected = true;
		dbg_msg("RClient", "Connected to RClient server");
		SetupSocketListeners();
		if(m_Socket.socket())
			m_Socket.socket()->emit("request_token");
	});
	m_Socket.set_close_listener([this](sio::client::close_reason const &Reason) {
		m_IsConnected = false;
		m_TokenReceived = false;
		dbg_msg("RClient", "Disconnected from RClient server (reason: %i)", static_cast<int>(Reason));
	});
	m_Socket.set_fail_listener([this]() {
		m_IsConnected = false;
		m_TokenReceived = false;
		dbg_msg("RClient", "Connection to RClient server failed");
	});

	// Connect to server
	m_Socket.connect(RCLIENT_SERVER_URL);

	dbg_msg("RClient", "Connecting to RClient server...");
}

void CRClientIndicator::DisconnectFromServer()
{
	if(m_IsConnected)
	{
		m_Socket.close();
	}

	m_IsConnected = false;
	m_TokenReceived = false;
	m_aAuthToken[0] = '\0';
}

void CRClientIndicator::SetupSocketListeners()
{
	if(!m_Socket.socket())
		return;

	// Setup event listeners
	m_Socket.socket()->on("token_response", [this](sio::event &Event) {
		OnTokenReceived(Event);
	});
	m_Socket.socket()->on("registration_success", [this](sio::event &Event) {
		OnRegistrationSuccess(Event);
	});
	m_Socket.socket()->on("unregister_success", [this](sio::event &Event) {
		OnUnregisterSuccess(Event);
	});
	m_Socket.socket()->on("players_update", [this](sio::event &Event) {
		OnPlayersUpdate(Event);
	});
	m_Socket.socket()->on("error", [this](sio::event &Event) {
		OnError(Event);
	});
}

void CRClientIndicator::RegisterPlayer()
{

	if(!m_IsConnected || !m_TokenReceived)
		return;

	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	if(!m_Socket.socket())
		return;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	int LocalClientId = GameClient()->m_aLocalIds[0];
	if(LocalClientId < 0)
		return;

	int DummyClientId = -1;
	if(Client()->DummyConnected())
	{
		DummyClientId = GameClient()->m_aLocalIds[1];
	}

	// Build registration data
	sio::message::ptr RegistrationData = sio::object_message::create();
	auto &DataMap = RegistrationData->get_map();

	DataMap["server_address"] = sio::string_message::create(CurrentServerInfo.m_aAddress);
	DataMap["player_id"] = sio::int_message::create(LocalClientId);
	DataMap["auth_token"] = sio::string_message::create(m_aAuthToken);

	if(DummyClientId >= 0)
	{
		DataMap["dummy_id"] = sio::int_message::create(DummyClientId);
	}

	m_Socket.socket()->emit("register_player", RegistrationData);

	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", "Registering player on server...");
}

void CRClientIndicator::OnTokenReceived(sio::event &Event)
{
	auto Data = Event.get_message();
	if(!Data || Data->get_flag() != sio::message::flag_object)
		return;

	auto &DataMap = Data->get_map();
	if(DataMap.find("token") == DataMap.end())
		return;

	std::string Token = DataMap["token"]->get_string();
	str_copy(m_aAuthToken, Token.c_str(), sizeof(m_aAuthToken));
	m_TokenReceived = true;

	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "RClient", "Received auth token");
}

void CRClientIndicator::OnRegistrationSuccess(sio::event &Event)
{
	auto Data = Event.get_message();
	if(!Data || Data->get_flag() != sio::message::flag_object)
		return;

	auto &DataMap = Data->get_map();

	std::string ServerAddr = DataMap["server_address"]->get_string();
	int PlayerId = DataMap["player_id"]->get_int();

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Successfully registered on %s as player %d", ServerAddr.c_str(), PlayerId);
	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", aBuf);
}

void CRClientIndicator::OnUnregisterSuccess(sio::event &Event)
{
	auto Data = Event.get_message();
	if(!Data || Data->get_flag() != sio::message::flag_object)
		return;

	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "RClient", "Successfully unregistered from server");
}

void CRClientIndicator::OnPlayersUpdate(sio::event &Event)
{
	auto Data = Event.get_message();
	if(!Data || Data->get_flag() != sio::message::flag_object)
		return;

	auto &DataMap = Data->get_map();

	// Get server address
	if(DataMap.find("server_address") == DataMap.end())
		return;

	std::string ServerAddr = DataMap["server_address"]->get_string();

	// Get players object
	if(DataMap.find("players") == DataMap.end())
		return;

	auto PlayersData = DataMap["players"];
	if(PlayersData->get_flag() != sio::message::flag_object)
		return;

	auto &PlayersMap = PlayersData->get_map();

	// Update RClient users list
	std::lock_guard<std::mutex> Lock(m_RClientUsersMutex);

	// Clear old data for this server
	m_RClientUsers[ServerAddr].clear();

	// Parse each player
	for(auto &PlayerEntry : PlayersMap)
	{
		int PlayerId = std::stoi(PlayerEntry.first);
		auto PlayerData = PlayerEntry.second;

		if(PlayerData->get_flag() != sio::message::flag_object)
			continue;

		auto &PlayerDataMap = PlayerData->get_map();

		// Add main player
		m_RClientUsers[ServerAddr][PlayerId] = true;

		// Check for dummy
		if(PlayerDataMap.find("dummy_id") != PlayerDataMap.end())
		{
			int DummyId = PlayerDataMap["dummy_id"]->get_int();
			m_RClientUsers[ServerAddr][DummyId] = true;
		}
	}

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Players update for %s: %d players", ServerAddr.c_str(), (int)PlayersMap.size());
	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "RClient", aBuf);
}

void CRClientIndicator::OnError(sio::event &Event)
{
	auto Data = Event.get_message();
	if(!Data || Data->get_flag() != sio::message::flag_object)
		return;

	auto &DataMap = Data->get_map();
	if(DataMap.find("message") == DataMap.end())
		return;

	std::string ErrorMsg = DataMap["message"]->get_string();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "Error: %s", ErrorMsg.c_str());
	GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", aBuf);
}

bool CRClientIndicator::IsPlayerRClient(int ClientId)
{
	if(Client()->State() != IClient::STATE_ONLINE || !m_IsConnected)
		return false;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	std::string ServerKey(CurrentServerInfo.m_aAddress);

	std::lock_guard<std::mutex> Lock(m_RClientUsersMutex);

	// Check if this server and player are in our RClient users map
	auto ServerIt = m_RClientUsers.find(ServerKey);
	if(ServerIt != m_RClientUsers.end())
	{
		auto PlayerIt = ServerIt->second.find(ClientId);
		if(PlayerIt != ServerIt->second.end())
		{
			return true;
		}
	}

	return false;
}