#ifndef GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H
#define GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H

#include <base/system.h>

#include <SDL_audio.h>

#include <engine/shared/protocol.h>

#include <array>
#include <cstdint>

class CGameClient;
class IClient;
class IConsole;
struct OpusDecoder;
struct OpusEncoder;

class CRClientVoice
{
	struct SVoicePeer
	{
		OpusDecoder *m_pDecoder = nullptr;
	};

	CGameClient *m_pGameClient = nullptr;
	IClient *m_pClient = nullptr;
	IConsole *m_pConsole = nullptr;

	NETSOCKET m_Socket = nullptr;
	NETADDR m_ServerAddr = NETADDR_ZEROED;
	bool m_ServerAddrValid = false;
	char m_aServerAddrStr[128] = {0};

	SDL_AudioDeviceID m_CaptureDevice = 0;
	SDL_AudioDeviceID m_OutputDevice = 0;
	SDL_AudioSpec m_CaptureSpec = {};
	SDL_AudioSpec m_OutputSpec = {};
	char m_aInputDeviceName[128] = {0};
	char m_aOutputDeviceName[128] = {0};

	OpusEncoder *m_pEncoder = nullptr;
	std::array<SVoicePeer, MAX_CLIENTS> m_aPeers = {};
	std::array<int64_t, MAX_CLIENTS> m_aLastHeard = {};

	bool m_PttActive = false;
	uint16_t m_Sequence = 0;
	uint32_t m_ContextHash = 0;
	int64_t m_LastKeepalive = 0;

	bool EnsureSocket();
	bool EnsureAudio();
	void Shutdown();
	void UpdateServerAddr();
	void UpdateContext();
	void ProcessCapture();
	void ProcessIncoming();
	const char *FindDeviceName(bool Capture, const char *pDesired) const;

public:
	void Init(CGameClient *pGameClient, IClient *pClient, IConsole *pConsole);
	void OnRender();
	void SetPttActive(bool Active);
	void ListDevices();
	bool IsVoiceActive(int ClientId) const;
};

#endif // GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H
