#ifndef GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H
#define GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H

#include <base/system.h>
#include <base/vmath.h>

#include <SDL_audio.h>

#include <engine/shared/protocol.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

class CGameClient;
class IClient;
class IConsole;
struct OpusDecoder;
struct OpusEncoder;

struct SRClientVoiceConfigSnapshot
{
	int m_RiVoiceFilterEnable = 0;
	int m_RiVoiceCompThreshold = 0;
	int m_RiVoiceCompRatio = 0;
	int m_RiVoiceCompAttackMs = 0;
	int m_RiVoiceCompReleaseMs = 0;
	int m_RiVoiceCompMakeup = 0;
	int m_RiVoiceLimiter = 0;
	int m_RiVoiceStereo = 0;
	int m_RiVoiceRadius = 0;
	int m_RiVoiceVolume = 0;
	int m_RiVoiceIgnoreDistance = 0;
	int m_RiVoiceListMode = 0;
	int m_RiVoiceDebug = 0;
	int m_ClShowOthers = 0;
	uint32_t m_RiVoiceTokenHash = 0;
	char m_aRiVoiceWhitelist[512] = {};
	char m_aRiVoiceBlacklist[512] = {};
	char m_aRiVoiceMute[512] = {};
	char m_aRiVoiceNameVolumes[512] = {};
};

class CRClientVoice
{
	struct SVoicePeer
	{
		static constexpr int MAX_JITTER_PACKETS = 32;
		static constexpr int MAX_PACKET_BYTES = 1500;
		static constexpr int MAX_FRAMES = 8;
		struct SVoiceFrame
		{
			int16_t m_aPcm[960] = {};
			int m_Samples = 0;
			float m_LeftGain = 1.0f;
			float m_RightGain = 1.0f;
		};
		struct SJitterPacket
		{
			bool m_Valid = false;
			uint16_t m_Seq = 0;
			int m_Size = 0;
			float m_LeftGain = 1.0f;
			float m_RightGain = 1.0f;
			uint8_t m_aData[MAX_PACKET_BYTES] = {};
		};

		OpusDecoder *m_pDecoder = nullptr;
		uint16_t m_LastSeq = 0;
		bool m_HasSeq = false;
		uint16_t m_LastRecvSeq = 0;
		bool m_HasLastRecvSeq = false;
		float m_LossEwma = 0.0f;
		uint16_t m_NextSeq = 0;
		bool m_HasNextSeq = false;
		float m_LastGainLeft = 1.0f;
		float m_LastGainRight = 1.0f;
		int64_t m_LastRecvTime = 0;
		float m_JitterMs = 0.0f;
		int m_TargetFrames = 3;
		int m_QueuedPackets = 0;
		SJitterPacket m_aPackets[MAX_JITTER_PACKETS] = {};
		SVoiceFrame m_aFrames[MAX_FRAMES] = {};
		int m_FrameHead = 0;
		int m_FrameTail = 0;
		int m_FrameCount = 0;
		int m_FrameReadPos = 0;
	};

	CGameClient *m_pGameClient = nullptr;
	IClient *m_pClient = nullptr;
	IConsole *m_pConsole = nullptr;

	NETSOCKET m_Socket = nullptr;
	NETADDR m_ServerAddr = NETADDR_ZEROED;
	std::atomic<bool> m_ServerAddrValid = false;
	char m_aServerAddrStr[128] = {0};

	SDL_AudioDeviceID m_CaptureDevice = 0;
	SDL_AudioDeviceID m_OutputDevice = 0;
	SDL_AudioSpec m_CaptureSpec = {};
	SDL_AudioSpec m_OutputSpec = {};
	char m_aInputDeviceName[128] = {0};
	char m_aOutputDeviceName[128] = {0};
	bool m_OutputStereo = true;
	bool m_LogDeviceChange = false;
	float m_HpfPrevIn = 0.0f;
	float m_HpfPrevOut = 0.0f;
	float m_CompEnv = 0.0f;
	std::atomic<int> m_OutputChannels = 0;
	std::vector<int32_t> m_MixBuffer;

	OpusEncoder *m_pEncoder = nullptr;
	int m_EncBitrate = 24000;
	int m_EncLossPerc = 0;
	bool m_EncFec = false;
	int64_t m_LastEncUpdate = 0;
	std::array<SVoicePeer, MAX_CLIENTS> m_aPeers = {};
	std::array<std::atomic<int64_t>, MAX_CLIENTS> m_aLastHeard = {};

	std::atomic<bool> m_PttActive = false;
	uint16_t m_Sequence = 0;
	std::atomic<uint32_t> m_ContextHash = 0;
	int64_t m_LastKeepalive = 0;

	std::thread m_Worker;
	std::atomic<bool> m_WorkerStop = false;
	std::atomic<bool> m_WorkerEnabled = false;
	bool m_ShutdownDone = true;

	std::mutex m_ServerAddrMutex;

	mutable std::mutex m_ConfigMutex;
	SRClientVoiceConfigSnapshot m_ConfigSnapshot = {};

	std::mutex m_SnapshotMutex;
	int m_LocalClientIdSnap = -1;
	bool m_OnlineSnap = false;
	std::array<vec2, MAX_CLIENTS> m_aClientPosSnap = {};
	std::array<std::array<char, MAX_NAME_LENGTH>, MAX_CLIENTS> m_aClientNameSnap = {};
	std::array<uint8_t, MAX_CLIENTS> m_aClientOtherTeamSnap = {};

	bool EnsureSocket();
	bool EnsureAudio();
	void Shutdown();
	void UpdateServerAddr();
	bool UpdateContext();
	void UpdateClientSnapshot();
	void UpdateConfigSnapshot();
	void GetConfigSnapshot(SRClientVoiceConfigSnapshot &Out) const;
	void ProcessCapture();
	void ProcessIncoming();
	void DecodeJitter();
	void UpdateEncoderParams();
	void PushPeerFrame(int PeerId, const int16_t *pPcm, int Samples, float LeftGain, float RightGain);
	void MixAudio(int16_t *pOut, int Samples, int OutputChannels);
	void ClearPeerFrames();
	static void SDLAudioCallback(void *pUserData, Uint8 *pStream, int Len);
	const char *FindDeviceName(bool Capture, const char *pDesired) const;
	void StartWorker();
	void StopWorker();
	void WorkerLoop();

public:
	void Init(CGameClient *pGameClient, IClient *pClient, IConsole *pConsole);
	void OnRender();
	void SetPttActive(bool Active);
	void ListDevices();
	bool IsVoiceActive(int ClientId) const;
};

#endif // GAME_CLIENT_COMPONENTS_RCLIENT_VOICE_H
