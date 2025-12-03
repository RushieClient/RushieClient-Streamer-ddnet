#ifndef RCLIENT_RCLIENT_INDICATOR_H
#define RCLIENT_RCLIENT_INDICATOR_H

#include "engine/client.h"

#include <engine/shared/console.h>

#include <game/client/component.h>

#include <sio_client.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class CRClientIndicator : public CComponent
{
	static constexpr const char *DEFAULT_RCLIENT_SERVER_URL = "https://server.rushie-client.ru";

	// Socket.IO client
	sio::client m_Socket;
	bool m_IsConnecting = false;
	int64_t m_LastConnectAttempt = 0;
	std::string m_ServerUrl;

	// Authentication
	char m_aAuthToken[128] = {0};
	bool m_TokenReceived = false;

	int m_DummyId = -1;
	int m_PlayerId = -1;
	bool m_IsConnected = false;
	int m_PrevClientState = IClient::STATE_OFFLINE;

	std::map<std::string, std::map<int, bool>> m_RClientUsers;
	std::mutex m_RClientUsersMutex;

	void OnTokenReceived(sio::event &Event);
	void OnRegistrationSuccess(sio::event &Event);
	void OnUnregisterSuccess(sio::event &Event);
	void OnPlayersUpdate(sio::event &Event);
	void OnAllPlayersResponse(sio::event &Event);
	void OnError(sio::event &Event);

	// Connection management
	void ConnectToServer();
	void DisconnectFromServer();
	void RegisterPlayer();
	void UpdateServerInfo();
	void SetupSocketListeners();

public:

	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnShutdown() override;
	void OnRender() override;

	bool IsPlayerRClient(int ClientId);
	bool IsConnected() const { return m_IsConnected; }

	sio::client* GetSocket() { return &m_Socket; }
};

#endif // RCLIENT_RCLIENT_INDICATOR_H
