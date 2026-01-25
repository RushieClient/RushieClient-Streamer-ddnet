#include "voice.h"

#include <base/log.h>
#include <base/system.h>
#include <base/str.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/shared/config.h>
#include <engine/shared/console.h>

#include <game/client/gameclient.h>

#include <game/gamecore.h>

#include <opus/opus.h>

#include <algorithm>
#include <cctype>
#include <chrono>

static bool VoiceListMatch(const char *pList, const char *pName)
{
	if(!pList || pList[0] == '\0')
		return false;

	const char *p = pList;
	while(*p)
	{
		while(*p == ',' || *p == ' ' || *p == '\t')
			p++;
		if(*p == '\0')
			break;

		const char *pStart = p;
		while(*p && *p != ',')
			p++;
		int Len = (int)(p - pStart);
		while(Len > 0 && std::isspace((unsigned char)pStart[Len - 1]))
			Len--;
		if(Len <= 0)
			continue;

		char aToken[MAX_NAME_LENGTH];
		str_truncate(aToken, sizeof(aToken), pStart, Len);
		if(str_comp_nocase(aToken, pName) == 0)
			return true;
	}

	return false;
}

static constexpr int VOICE_SAMPLE_RATE = 48000;
static constexpr int VOICE_CHANNELS = 1;
static constexpr int VOICE_FRAME_SAMPLES = 960;
static constexpr int VOICE_FRAME_BYTES = VOICE_FRAME_SAMPLES * sizeof(int16_t);
static constexpr int VOICE_MAX_PACKET = 1500;

static constexpr char VOICE_MAGIC[4] = {'R', 'V', '0', '1'};
static constexpr uint8_t VOICE_VERSION = 1;
static constexpr uint8_t VOICE_TYPE_AUDIO = 1;
static constexpr uint8_t VOICE_TYPE_PING = 2;

static void WriteU16(uint8_t *pBuf, uint16_t Value)
{
	pBuf[0] = Value & 0xff;
	pBuf[1] = (Value >> 8) & 0xff;
}

static void WriteU32(uint8_t *pBuf, uint32_t Value)
{
	pBuf[0] = Value & 0xff;
	pBuf[1] = (Value >> 8) & 0xff;
	pBuf[2] = (Value >> 16) & 0xff;
	pBuf[3] = (Value >> 24) & 0xff;
}

static void WriteFloat(uint8_t *pBuf, float Value)
{
	static_assert(sizeof(float) == 4, "float must be 4 bytes");
	mem_copy(pBuf, &Value, sizeof(Value));
}

static uint16_t ReadU16(const uint8_t *pBuf)
{
	return (uint16_t)pBuf[0] | ((uint16_t)pBuf[1] << 8);
}

static uint32_t ReadU32(const uint8_t *pBuf)
{
	return (uint32_t)pBuf[0] | ((uint32_t)pBuf[1] << 8) | ((uint32_t)pBuf[2] << 16) | ((uint32_t)pBuf[3] << 24);
}

static float ReadFloat(const uint8_t *pBuf)
{
	float Value = 0.0f;
	mem_copy(&Value, pBuf, sizeof(Value));
	return Value;
}

static bool ParseHostPort(const char *pAddrStr, char *pHost, size_t HostSize, int &Port)
{
	const char *pColon = str_rchr(pAddrStr, ':');
	if(!pColon || pColon == pAddrStr || *(pColon + 1) == '\0')
		return false;

	str_truncate(pHost, HostSize, pAddrStr, pColon - pAddrStr);
	if(pHost[0] == '[')
	{
		const int Len = str_length(pHost);
		if(Len >= 2 && pHost[Len - 1] == ']')
		{
			mem_move(pHost, pHost + 1, Len - 2);
			pHost[Len - 2] = '\0';
		}
	}

	Port = str_toint(pColon + 1);
	return Port > 0 && Port <= 65535;
}

void CRClientVoice::Init(CGameClient *pGameClient, IClient *pClient, IConsole *pConsole)
{
	m_pGameClient = pGameClient;
	m_pClient = pClient;
	m_pConsole = pConsole;
}

void CRClientVoice::SetPttActive(bool Active)
{
	m_PttActive = Active;
}

bool CRClientVoice::EnsureSocket()
{
	if(m_Socket)
		return true;

	NETADDR BindAddr = NETADDR_ZEROED;
	BindAddr.type = NETTYPE_IPV4 | NETTYPE_IPV6;
	m_Socket = net_udp_create(BindAddr);
	if(!m_Socket)
	{
		log_error("voice", "Failed to open UDP socket");
		return false;
	}
	return true;
}

bool CRClientVoice::EnsureAudio()
{
	if(m_CaptureDevice && m_OutputDevice && m_pEncoder)
		return true;

	SDL_AudioSpec Want = {};
	Want.freq = VOICE_SAMPLE_RATE;
	Want.format = AUDIO_S16;
	Want.channels = VOICE_CHANNELS;
	Want.samples = VOICE_FRAME_SAMPLES;
	Want.callback = nullptr;

	if(str_comp(m_aInputDeviceName, g_Config.m_RiVoiceInputDevice) != 0)
	{
		if(m_CaptureDevice)
		{
			SDL_CloseAudioDevice(m_CaptureDevice);
			m_CaptureDevice = 0;
		}
		str_copy(m_aInputDeviceName, g_Config.m_RiVoiceInputDevice, sizeof(m_aInputDeviceName));
	}

	if(str_comp(m_aOutputDeviceName, g_Config.m_RiVoiceOutputDevice) != 0)
	{
		if(m_OutputDevice)
		{
			SDL_CloseAudioDevice(m_OutputDevice);
			m_OutputDevice = 0;
		}
		str_copy(m_aOutputDeviceName, g_Config.m_RiVoiceOutputDevice, sizeof(m_aOutputDeviceName));
	}

	const char *pInputName = FindDeviceName(true, m_aInputDeviceName);
	if(m_aInputDeviceName[0] != '\0' && pInputName == nullptr)
	{
		log_error("voice", "Input device not found: '%s'", m_aInputDeviceName);
		return false;
	}

	const char *pOutputName = FindDeviceName(false, m_aOutputDeviceName);
	if(m_aOutputDeviceName[0] != '\0' && pOutputName == nullptr)
	{
		log_error("voice", "Output device not found: '%s'", m_aOutputDeviceName);
		return false;
	}

	if(!m_CaptureDevice)
	{
		m_CaptureDevice = SDL_OpenAudioDevice(pInputName, 1, &Want, &m_CaptureSpec, 0);
		if(!m_CaptureDevice)
		{
			log_error("voice", "Failed to open capture device: %s", SDL_GetError());
			return false;
		}
		SDL_PauseAudioDevice(m_CaptureDevice, 0);
	}

	if(!m_OutputDevice)
	{
		m_OutputDevice = SDL_OpenAudioDevice(pOutputName, 0, &Want, &m_OutputSpec, 0);
		if(!m_OutputDevice)
		{
			log_error("voice", "Failed to open output device: %s", SDL_GetError());
			SDL_CloseAudioDevice(m_CaptureDevice);
			m_CaptureDevice = 0;
			return false;
		}
		SDL_PauseAudioDevice(m_OutputDevice, 0);
	}

	if(!m_pEncoder)
	{
		int Error = 0;
		m_pEncoder = opus_encoder_create(VOICE_SAMPLE_RATE, VOICE_CHANNELS, OPUS_APPLICATION_VOIP, &Error);
		if(!m_pEncoder || Error != OPUS_OK)
		{
			log_error("voice", "Failed to create Opus encoder: %d", Error);
			return false;
		}
		opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(24000));
	}

	return true;
}

const char *CRClientVoice::FindDeviceName(bool Capture, const char *pDesired) const
{
	if(!pDesired || pDesired[0] == '\0')
		return nullptr;

	const int Num = SDL_GetNumAudioDevices(Capture ? 1 : 0);
	for(int i = 0; i < Num; i++)
	{
		const char *pName = SDL_GetAudioDeviceName(i, Capture ? 1 : 0);
		if(pName && str_comp_nocase(pName, pDesired) == 0)
			return pName;
	}
	return nullptr;
}

void CRClientVoice::ListDevices()
{
	if(!m_pConsole)
		return;

	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voice", "Input devices:");
	const int NumInputs = SDL_GetNumAudioDevices(1);
	for(int i = 0; i < NumInputs; i++)
	{
		const char *pName = SDL_GetAudioDeviceName(i, 1);
		if(pName)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voice", pName);
	}

	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voice", "Output devices:");
	const int NumOutputs = SDL_GetNumAudioDevices(0);
	for(int i = 0; i < NumOutputs; i++)
	{
		const char *pName = SDL_GetAudioDeviceName(i, 0);
		if(pName)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voice", pName);
	}
}

void CRClientVoice::Shutdown()
{
	if(m_CaptureDevice)
	{
		SDL_CloseAudioDevice(m_CaptureDevice);
		m_CaptureDevice = 0;
	}
	if(m_OutputDevice)
	{
		SDL_CloseAudioDevice(m_OutputDevice);
		m_OutputDevice = 0;
	}
	if(m_pEncoder)
	{
		opus_encoder_destroy(m_pEncoder);
		m_pEncoder = nullptr;
	}
	for(auto &Peer : m_aPeers)
	{
		if(Peer.m_pDecoder)
		{
			opus_decoder_destroy(Peer.m_pDecoder);
			Peer.m_pDecoder = nullptr;
		}
	}
	if(m_Socket)
	{
		net_udp_close(m_Socket);
		m_Socket = nullptr;
	}
	m_ServerAddrValid = false;
	m_aServerAddrStr[0] = '\0';
}

void CRClientVoice::UpdateServerAddr()
{
	if(str_comp(m_aServerAddrStr, g_Config.m_RiVoiceServer) == 0)
		return;

	str_copy(m_aServerAddrStr, g_Config.m_RiVoiceServer, sizeof(m_aServerAddrStr));
	m_ServerAddrValid = false;
	if(m_aServerAddrStr[0] == '\0')
		return;

	if(net_addr_from_str(&m_ServerAddr, m_aServerAddrStr) == 0)
	{
		m_ServerAddrValid = true;
		return;
	}

	char aHost[128];
	int Port = 0;
	if(!ParseHostPort(m_aServerAddrStr, aHost, sizeof(aHost), Port))
	{
		log_error("voice", "Invalid voice server address '%s'", m_aServerAddrStr);
		return;
	}

	if(net_host_lookup(aHost, &m_ServerAddr, NETTYPE_ALL) == 0)
	{
		m_ServerAddr.port = Port;
		m_ServerAddrValid = true;
		return;
	}

	log_error("voice", "Failed to resolve voice server '%s'", m_aServerAddrStr);
}

void CRClientVoice::UpdateContext()
{
	if(!m_pClient || m_pClient->State() != IClient::STATE_ONLINE)
	{
		m_ContextHash = 0;
		return;
	}
	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(&m_pClient->ServerAddress(), aAddr, sizeof(aAddr), true);
	m_ContextHash = str_quickhash(aAddr);
}

void CRClientVoice::ProcessCapture()
{
	if(!m_CaptureDevice || !m_pEncoder || !m_ServerAddrValid || !m_Socket)
		return;

	const int64_t Now = time_get();
	if(m_pClient->State() == IClient::STATE_ONLINE && !m_PttActive)
	{
		if(m_LastKeepalive == 0 || Now - m_LastKeepalive > time_freq() * 2)
		{
			uint8_t aPacket[VOICE_MAX_PACKET];
			size_t Offset = 0;
			mem_copy(aPacket + Offset, VOICE_MAGIC, sizeof(VOICE_MAGIC));
			Offset += sizeof(VOICE_MAGIC);
			aPacket[Offset++] = VOICE_VERSION;
			aPacket[Offset++] = VOICE_TYPE_PING;
			WriteU16(aPacket + Offset, 0);
			Offset += sizeof(uint16_t);
			WriteU32(aPacket + Offset, m_ContextHash);
			Offset += sizeof(uint32_t);
			WriteU16(aPacket + Offset, 0);
			Offset += sizeof(uint16_t);
			WriteU16(aPacket + Offset, m_Sequence++);
			Offset += sizeof(uint16_t);
			WriteFloat(aPacket + Offset, 0.0f);
			Offset += sizeof(float);
			WriteFloat(aPacket + Offset, 0.0f);
			Offset += sizeof(float);
			net_udp_send(m_Socket, &m_ServerAddr, aPacket, (int)Offset);
			m_LastKeepalive = Now;
		}
	}

	if(!m_PttActive || m_pClient->State() != IClient::STATE_ONLINE)
	{
		SDL_ClearQueuedAudio(m_CaptureDevice);
		return;
	}

	static int64_t s_TxLastLog = 0;
	static int s_TxPackets = 0;

	const int ClientId = m_pGameClient->m_Snap.m_LocalClientId;
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;

	m_aLastHeard[ClientId] = time_get();

	const vec2 Pos = m_pGameClient->m_aClients[ClientId].m_RenderPos;
	uint8_t aPacket[VOICE_MAX_PACKET];
	uint8_t aPayload[VOICE_MAX_PACKET];

	while(SDL_GetQueuedAudioSize(m_CaptureDevice) >= VOICE_FRAME_BYTES)
	{
		int16_t aPcm[VOICE_FRAME_SAMPLES];
		SDL_DequeueAudio(m_CaptureDevice, aPcm, VOICE_FRAME_BYTES);

		const int EncSize = opus_encode(m_pEncoder, aPcm, VOICE_FRAME_SAMPLES, aPayload, (int)sizeof(aPayload));
		if(EncSize <= 0)
			continue;

		size_t Offset = 0;
		mem_copy(aPacket + Offset, VOICE_MAGIC, sizeof(VOICE_MAGIC));
		Offset += sizeof(VOICE_MAGIC);
		aPacket[Offset++] = VOICE_VERSION;
		aPacket[Offset++] = VOICE_TYPE_AUDIO;
		WriteU16(aPacket + Offset, (uint16_t)EncSize);
		Offset += sizeof(uint16_t);
		WriteU32(aPacket + Offset, m_ContextHash);
		Offset += sizeof(uint32_t);
		WriteU16(aPacket + Offset, (uint16_t)ClientId);
		Offset += sizeof(uint16_t);
		WriteU16(aPacket + Offset, m_Sequence++);
		Offset += sizeof(uint16_t);
		WriteFloat(aPacket + Offset, Pos.x);
		Offset += sizeof(float);
		WriteFloat(aPacket + Offset, Pos.y);
		Offset += sizeof(float);
		mem_copy(aPacket + Offset, aPayload, EncSize);
		Offset += EncSize;

		net_udp_send(m_Socket, &m_ServerAddr, aPacket, (int)Offset);
		if(g_Config.m_RiVoiceDebug)
		{
			s_TxPackets++;
			if(Now - s_TxLastLog > time_freq())
			{
				log_info("voice", "tx packets=%d ctx=0x%08x", s_TxPackets, m_ContextHash);
				s_TxLastLog = Now;
				s_TxPackets = 0;
			}
		}
	}
}

void CRClientVoice::ProcessIncoming()
{
	if(!m_OutputDevice || !m_Socket)
		return;

	static int64_t s_RxLastLog = 0;
	static int s_RxPackets = 0;
	static int s_RxDropContext = 0;
	static int s_RxDropRadius = 0;

	while(net_socket_read_wait(m_Socket, std::chrono::nanoseconds(0)) > 0)
	{
		NETADDR Addr;
		unsigned char *pData = nullptr;
		int Bytes = net_udp_recv(m_Socket, &Addr, &pData);
		if(Bytes <= 0 || !pData)
			break;

		if(Bytes < (int)(sizeof(VOICE_MAGIC) + 2 + 2 + 4 + 2 + 2 + 8))
			continue;

		size_t Offset = 0;
		if(mem_comp(pData, VOICE_MAGIC, sizeof(VOICE_MAGIC)) != 0)
			continue;
		Offset += sizeof(VOICE_MAGIC);

		const uint8_t Version = pData[Offset++];
		const uint8_t Type = pData[Offset++];
		if(Version != VOICE_VERSION || Type != VOICE_TYPE_AUDIO)
			continue;

		const uint16_t PayloadSize = ReadU16(pData + Offset);
		Offset += sizeof(uint16_t);
		const uint32_t ContextHash = ReadU32(pData + Offset);
		Offset += sizeof(uint32_t);
		const uint16_t SenderId = ReadU16(pData + Offset);
		Offset += sizeof(uint16_t);
		Offset += sizeof(uint16_t); // sequence
		const float PosX = ReadFloat(pData + Offset);
		Offset += sizeof(float);
		const float PosY = ReadFloat(pData + Offset);
		Offset += sizeof(float);

		if(ContextHash == 0 || ContextHash != m_ContextHash)
		{
			s_RxDropContext++;
			continue;
		}
		if(SenderId >= MAX_CLIENTS)
			continue;

		const int LocalId = m_pGameClient->m_Snap.m_LocalClientId;
		if(SenderId == LocalId)
			continue;
		if(g_Config.m_ClShowOthers != SHOW_OTHERS_ON && m_pGameClient->IsOtherTeam(SenderId))
			continue;
		if(g_Config.m_RiVoiceListMode == 1 && !VoiceListMatch(g_Config.m_RiVoiceWhitelist, m_pGameClient->m_aClients[SenderId].m_aName))
			continue;
		if(g_Config.m_RiVoiceListMode == 2 && VoiceListMatch(g_Config.m_RiVoiceBlacklist, m_pGameClient->m_aClients[SenderId].m_aName))
			continue;

		m_aLastHeard[SenderId] = time_get();

		if(Offset + PayloadSize > (size_t)Bytes)
			continue;

		const vec2 LocalPos = m_pGameClient->m_aClients[LocalId].m_RenderPos;
		const vec2 SenderPos = vec2(PosX, PosY);
		const float Radius = std::max(1, g_Config.m_RiVoiceRadius) * 32.0f;
		const float Dist = distance(LocalPos, SenderPos);
		if(!g_Config.m_RiVoiceIgnoreDistance && Dist > Radius)
		{
			s_RxDropRadius++;
			continue;
		}

		OpusDecoder *pDecoder = m_aPeers[SenderId].m_pDecoder;
		if(!pDecoder)
		{
			int Error = 0;
			pDecoder = opus_decoder_create(VOICE_SAMPLE_RATE, VOICE_CHANNELS, &Error);
			if(!pDecoder || Error != OPUS_OK)
			{
				log_error("voice", "Failed to create Opus decoder: %d", Error);
				continue;
			}
			m_aPeers[SenderId].m_pDecoder = pDecoder;
		}

		int16_t aPcm[VOICE_FRAME_SAMPLES];
		const int Samples = opus_decode(pDecoder, pData + Offset, PayloadSize, aPcm, VOICE_FRAME_SAMPLES, 0);
		if(Samples <= 0)
			continue;

		const float RadiusFactor = g_Config.m_RiVoiceIgnoreDistance ? 1.0f : (1.0f - (Dist / Radius));
		const float Volume = std::clamp(RadiusFactor * (g_Config.m_RiVoiceVolume / 100.0f), 0.0f, 2.0f);
		if(Volume <= 0.0f)
			continue;

		for(int i = 0; i < Samples; i++)
		{
			const int Sample = (int)(aPcm[i] * Volume);
			aPcm[i] = (int16_t)std::clamp(Sample, -32768, 32767);
		}

		const int MaxQueued = VOICE_SAMPLE_RATE * sizeof(int16_t) * 2;
		if((int)SDL_GetQueuedAudioSize(m_OutputDevice) > MaxQueued)
			SDL_ClearQueuedAudio(m_OutputDevice);

		SDL_QueueAudio(m_OutputDevice, aPcm, Samples * sizeof(int16_t));

		if(g_Config.m_RiVoiceDebug)
		{
			const int64_t Now = time_get();
			s_RxPackets++;
			if(Now - s_RxLastLog > time_freq())
			{
				log_info("voice", "rx packets=%d drop_ctx=%d drop_radius=%d", s_RxPackets, s_RxDropContext, s_RxDropRadius);
				s_RxLastLog = Now;
				s_RxPackets = 0;
				s_RxDropContext = 0;
				s_RxDropRadius = 0;
			}
		}
	}
}

bool CRClientVoice::IsVoiceActive(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;
	const int64_t LastHeard = m_aLastHeard[ClientId];
	if(LastHeard == 0)
		return false;
	return time_get() - LastHeard < time_freq() / 2;
}

void CRClientVoice::OnRender()
{
	if(!g_Config.m_RiVoiceEnable || !m_pGameClient || !m_pClient)
	{
		Shutdown();
		return;
	}

	UpdateServerAddr();
	UpdateContext();

	if(!m_ServerAddrValid)
		return;
	if(!EnsureSocket() || !EnsureAudio())
		return;

	ProcessIncoming();
	ProcessCapture();
}
