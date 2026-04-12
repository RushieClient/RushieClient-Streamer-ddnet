#ifndef RCLIENT_RCLIENT_INDICATOR_H
#define RCLIENT_RCLIENT_INDICATOR_H

#include <game/client/component.h>

#include <engine/shared/console.h>
#include <engine/shared/http.h>

#include <memory>
#include <atomic>
#include <string>
#include <utility>
#include <vector>

class CRClientIndicator : public CComponent
{
	std::shared_ptr<CHttpRequest> m_pRClientUsersTaskSend = nullptr;
	std::shared_ptr<CHttpRequest> m_pRClientUsersTask = nullptr;

	void SendPlayerData(const char *pServerAddress, int ClientId, int DummyClientId, bool Online);
	void FetchRClientUsers(const char *pServerAddress, int ClientId, int DummyClientId);
	void FinishRClientUsers();
	void FinishRClientUsersSend();
	void ResetRClientUsers();
	void ResetRClientUsersSend();

	void ApplyPollHeaders(CHttpRequest &Request, const char *pServerAddress, int ClientId, int DummyClientId);
	void ClearVoiceAuth();
	void ClearUsers();

	struct SRClientUserInfo
	{
		std::string m_ServerAddress;
		int m_PlayerId;
		bool m_VoiceEnabled;
		bool m_VoiceMuted;
	};

	std::vector<SRClientUserInfo> m_vRClientUsers; // server address, player id, voice enabled

	int64_t m_LastPollAttempt = 0;
	std::atomic<uint32_t> m_VoiceAuthTimestamp{0};
	std::atomic<uint64_t> m_VoiceAuthHash{0};

	bool m_WasOnline = false;
	std::string m_LastServerAddress;
	int m_LastLocalId = -1;
	int m_LastDummyId = -1;
	int m_ServerRev = 0;

public:
	CRClientIndicator();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnRender() override;

	bool IsPlayerRClient(int ClientId);
	bool IsPlayerRClientVoiceEnabled(int ClientId);
	bool IsPlayerRClientVoiceMuted(int ClientId);
	bool GetCachedVoiceAuth(uint32_t &Timestamp, uint64_t &Hash) const;
};

#endif // RCLIENT_RCLIENT_INDICATOR_H
