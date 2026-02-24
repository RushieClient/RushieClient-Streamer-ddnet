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
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

// !!WARNING!!
// Voice full wrote by AI don't use that pls

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

static bool VoiceNameVolume(const char *pList, const char *pName, int &OutPercent)
{
	if(!pList || pList[0] == '\0' || !pName || pName[0] == '\0')
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
		const char *pEnd = p;
		while(pEnd > pStart && std::isspace((unsigned char)pEnd[-1]))
			pEnd--;
		if(pEnd <= pStart)
			continue;

		const char *pSep = nullptr;
		for(const char *q = pStart; q < pEnd; q++)	
		{
			if(*q == '=' || *q == ':')
			{
				pSep = q;
				break;
			}
		}
		if(!pSep)
			continue;

		const char *pNameEnd = pSep;
		while(pNameEnd > pStart && std::isspace((unsigned char)pNameEnd[-1]))
			pNameEnd--;
		const char *pValueStart = pSep + 1;
		while(pValueStart < pEnd && std::isspace((unsigned char)*pValueStart))
			pValueStart++;

		const int NameLen = (int)(pNameEnd - pStart);
		const int ValueLen = (int)(pEnd - pValueStart);
		if(NameLen <= 0 || ValueLen <= 0)
			continue;

		char aToken[MAX_NAME_LENGTH];
		str_truncate(aToken, sizeof(aToken), pStart, NameLen);
		if(str_comp_nocase(aToken, pName) != 0)
			continue;

		char aValue[16];
		str_truncate(aValue, sizeof(aValue), pValueStart, ValueLen);
		int Percent = str_toint(aValue);
		Percent = std::clamp(Percent, 0, 200);
		OutPercent = Percent;
		return true;
	}

	return false;
}

static constexpr char VOICE_MAGIC[4] = {'R', 'V', '0', '1'};
static constexpr uint8_t VOICE_VERSION = 2;
static constexpr uint8_t VOICE_TYPE_AUDIO = 1;
static constexpr uint8_t VOICE_TYPE_PING = 2;
static constexpr int VOICE_SAMPLE_RATE = 48000;
static constexpr int VOICE_CHANNELS = 1;
static constexpr int VOICE_FRAME_SAMPLES = 960;
static constexpr int VOICE_FRAME_BYTES = VOICE_FRAME_SAMPLES * sizeof(int16_t);
static constexpr int VOICE_MAX_PACKET = 1200;
static constexpr int VOICE_HEADER_SIZE = sizeof(VOICE_MAGIC) + 1 + 1 + 2 + 4 + 4 + 2 + 2 + 4 + 4;
static constexpr int VOICE_MAX_PAYLOAD = VOICE_MAX_PACKET - VOICE_HEADER_SIZE;

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
	uint32_t Bits = 0;
	mem_copy(&Bits, &Value, sizeof(Bits));
	WriteU32(pBuf, Bits);
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
	uint32_t Bits = ReadU32(pBuf);
	float Value = 0.0f;
	mem_copy(&Value, &Bits, sizeof(Value));
	return Value;
}

static float SanitizeFloat(float Value)
{
	if(!std::isfinite(Value))
		return 0.0f;
	if(Value > 1000000.0f)
		return 1000000.0f;
	if(Value < -1000000.0f)
		return -1000000.0f;
	return Value;
}

static void ApplyHpfCompressor(const SRClientVoiceConfigSnapshot &Config, int16_t *pSamples, int Count, float &PrevIn, float &PrevOut, float &Env)
{
	if(!Config.m_RiVoiceFilterEnable)
		return;

	const float CutoffHz = 120.0f;
	const float Rc = 1.0f / (2.0f * 3.14159265f * CutoffHz);
	const float Dt = 1.0f / VOICE_SAMPLE_RATE;
	const float Alpha = Rc / (Rc + Dt);

	const float Threshold = std::clamp(Config.m_RiVoiceCompThreshold / 100.0f, 0.01f, 1.0f);
	const float Ratio = std::max(1.0f, Config.m_RiVoiceCompRatio / 10.0f);
	const float AttackSec = std::max(0.001f, Config.m_RiVoiceCompAttackMs / 1000.0f);
	const float ReleaseSec = std::max(0.001f, Config.m_RiVoiceCompReleaseMs / 1000.0f);
	const float MakeupGain = std::max(0.0f, Config.m_RiVoiceCompMakeup / 100.0f);
	const float NoiseFloor = 0.02f;
	const float Limiter = std::clamp(Config.m_RiVoiceLimiter / 100.0f, 0.05f, 1.0f);
	const float AttackCoeff = 1.0f - std::exp(-1.0f / (AttackSec * VOICE_SAMPLE_RATE));
	const float ReleaseCoeff = 1.0f - std::exp(-1.0f / (ReleaseSec * VOICE_SAMPLE_RATE));

	for(int i = 0; i < Count; i++)
	{
		const float x = pSamples[i] / 32768.0f;
		const float y = Alpha * (PrevOut + x - PrevIn);
		PrevIn = x;
		PrevOut = y;

		const float AbsY = std::fabs(y);
		if(AbsY > Env)
			Env += (AbsY - Env) * AttackCoeff;
		else
			Env += (AbsY - Env) * ReleaseCoeff;

		float Gain = 1.0f;
		if(Env > Threshold)
			Gain = (Threshold + (Env - Threshold) / Ratio) / Env;
		if(Env > NoiseFloor)
			Gain *= MakeupGain;

		const float Out = std::clamp(y * Gain, -Limiter, Limiter);
		const int Sample = (int)std::clamp(Out * 32767.0f, -32768.0f, 32767.0f);
		pSamples[i] = (int16_t)Sample;
	}
}

void CRClientVoice::SDLAudioCallback(void *pUserData, Uint8 *pStream, int Len)
{
	auto *pThis = static_cast<CRClientVoice *>(pUserData);
	if(!pThis || Len <= 0)
		return;

	const int OutputChannels = std::max(1, pThis->m_OutputChannels.load());
	const int Samples = Len / (int)(sizeof(int16_t) * OutputChannels);
	if(Samples <= 0)
	{
		mem_zero(pStream, Len);
		return;
	}

	pThis->MixAudio(reinterpret_cast<int16_t *>(pStream), Samples, OutputChannels);
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
	m_ShutdownDone = false;
}

void CRClientVoice::SetPttActive(bool Active)
{
	m_PttActive.store(Active);
}

static int ClampJitterTarget(float JitterMs)
{
	if(JitterMs <= 8.0f)
		return 2;
	if(JitterMs <= 14.0f)
		return 3;
	if(JitterMs <= 22.0f)
		return 4;
	if(JitterMs <= 32.0f)
		return 5;
	return 6;
}

static int SeqDelta(uint16_t NewSeq, uint16_t OldSeq)
{
	return (int)(uint16_t)(NewSeq - OldSeq);
}

static bool SeqLess(uint16_t A, uint16_t B)
{
	return (int16_t)(A - B) < 0;
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
	SDL_AudioSpec WantCapture = {};
	WantCapture.freq = VOICE_SAMPLE_RATE;
	WantCapture.format = AUDIO_S16;
	WantCapture.channels = VOICE_CHANNELS;
	WantCapture.samples = VOICE_FRAME_SAMPLES;
	WantCapture.callback = nullptr;

	const bool WantStereo = g_Config.m_RiVoiceStereo != 0;
	const int DesiredOutputChannels = WantStereo ? 2 : 1;

	SDL_AudioSpec WantOutput = {};
	WantOutput.freq = VOICE_SAMPLE_RATE;
	WantOutput.format = AUDIO_S16;
	WantOutput.channels = DesiredOutputChannels;
	WantOutput.samples = VOICE_FRAME_SAMPLES;
	WantOutput.callback = SDLAudioCallback;
	WantOutput.userdata = this;

	const bool HadCapture = m_CaptureDevice != 0;
	const bool HadOutput = m_OutputDevice != 0;
	const bool HadEncoder = m_pEncoder != nullptr;

	if(str_comp(m_aInputDeviceName, g_Config.m_RiVoiceInputDevice) != 0)
	{
		if(m_CaptureDevice)
		{
			SDL_CloseAudioDevice(m_CaptureDevice);
			m_CaptureDevice = 0;
		}
		str_copy(m_aInputDeviceName, g_Config.m_RiVoiceInputDevice, sizeof(m_aInputDeviceName));
		m_LogDeviceChange = true;
	}

	if(str_comp(m_aOutputDeviceName, g_Config.m_RiVoiceOutputDevice) != 0)
	{
		if(m_OutputDevice)
		{
			SDL_CloseAudioDevice(m_OutputDevice);
			m_OutputDevice = 0;
		}
		str_copy(m_aOutputDeviceName, g_Config.m_RiVoiceOutputDevice, sizeof(m_aOutputDeviceName));
		m_LogDeviceChange = true;
	}

	if(m_OutputStereo != WantStereo)
	{
		if(m_OutputDevice)
		{
			SDL_CloseAudioDevice(m_OutputDevice);
			m_OutputDevice = 0;
		}
		m_OutputStereo = WantStereo;
		m_LogDeviceChange = true;
	}

	if(HadCapture && HadOutput && HadEncoder && m_CaptureDevice && m_OutputDevice && m_pEncoder)
	{
		return true;
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
		m_CaptureDevice = SDL_OpenAudioDevice(pInputName, 1, &WantCapture, &m_CaptureSpec, 0);
		if(!m_CaptureDevice)
		{
			log_error("voice", "Failed to open capture device: %s", SDL_GetError());
			return false;
		}
		SDL_PauseAudioDevice(m_CaptureDevice, 0);
	}

	if(!m_OutputDevice)
	{
		m_OutputDevice = SDL_OpenAudioDevice(pOutputName, 0, &WantOutput, &m_OutputSpec, 0);
		if(!m_OutputDevice)
		{
			log_error("voice", "Failed to open output device: %s", SDL_GetError());
			SDL_CloseAudioDevice(m_CaptureDevice);
			m_CaptureDevice = 0;
			return false;
		}
		const int Channels = m_OutputSpec.channels > 0 ? m_OutputSpec.channels : (WantStereo ? 2 : 1);
		m_OutputChannels.store(Channels);
		m_MixBuffer.resize((size_t)m_OutputSpec.samples * Channels);
		SDL_PauseAudioDevice(m_OutputDevice, 0);
		ClearPeerFrames();
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
		m_EncBitrate = 24000;
		m_EncLossPerc = 0;
		m_EncFec = false;
		m_LastEncUpdate = 0;
		opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(m_EncBitrate));
		opus_encoder_ctl(m_pEncoder, OPUS_SET_PACKET_LOSS_PERC(m_EncLossPerc));
		opus_encoder_ctl(m_pEncoder, OPUS_SET_INBAND_FEC(m_EncFec ? 1 : 0));
		opus_encoder_ctl(m_pEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	}

	if(m_LogDeviceChange)
	{
		const char *pInputReq = m_aInputDeviceName[0] ? m_aInputDeviceName : "<default>";
		const char *pOutputReq = m_aOutputDeviceName[0] ? m_aOutputDeviceName : "<default>";
		const char *pInputResolved = pInputName ? pInputName : "<default>";
		const char *pOutputResolved = pOutputName ? pOutputName : "<default>";
		log_info("voice", "audio devices set input='%s' resolved='%s' output='%s' resolved='%s' capture=%dch@%d output=%dch@%d",
			pInputReq, pInputResolved, pOutputReq, pOutputResolved,
			m_CaptureSpec.channels, m_CaptureSpec.freq,
			m_OutputSpec.channels, m_OutputSpec.freq);
		m_LogDeviceChange = false;
	}

	return true;
}

void CRClientVoice::PushPeerFrame(int PeerId, const int16_t *pPcm, int Samples, float LeftGain, float RightGain)
{
	if(PeerId < 0 || PeerId >= MAX_CLIENTS)
		return;
	if(Samples <= 0)
		return;

	SVoicePeer &Peer = m_aPeers[PeerId];
	if(Peer.m_FrameCount >= SVoicePeer::MAX_FRAMES)
	{
		Peer.m_FrameHead = (Peer.m_FrameHead + 1) % SVoicePeer::MAX_FRAMES;
		Peer.m_FrameCount--;
		Peer.m_FrameReadPos = 0;
	}

	SVoicePeer::SVoiceFrame &Frame = Peer.m_aFrames[Peer.m_FrameTail];
	const int CopySamples = std::min(Samples, VOICE_FRAME_SAMPLES);
	mem_copy(Frame.m_aPcm, pPcm, CopySamples * sizeof(int16_t));
	Frame.m_Samples = CopySamples;
	Frame.m_LeftGain = LeftGain;
	Frame.m_RightGain = RightGain;
	Peer.m_FrameTail = (Peer.m_FrameTail + 1) % SVoicePeer::MAX_FRAMES;
	Peer.m_FrameCount++;
}

void CRClientVoice::MixAudio(int16_t *pOut, int Samples, int OutputChannels)
{
	if(Samples <= 0 || OutputChannels <= 0)
		return;

	const int Needed = Samples * OutputChannels;
	if((int)m_MixBuffer.size() < Needed)
	{
		mem_zero(pOut, (size_t)Needed * sizeof(int16_t));
		return;
	}
	std::fill(m_MixBuffer.begin(), m_MixBuffer.begin() + Needed, 0);

	for(auto &Peer : m_aPeers)
	{
		int FrameIdx = Peer.m_FrameHead;
		int FrameCount = Peer.m_FrameCount;
		int ReadPos = Peer.m_FrameReadPos;
		if(FrameCount <= 0)
			continue;

		for(int i = 0; i < Samples; i++)
		{
			if(FrameCount <= 0)
				break;

			SVoicePeer::SVoiceFrame &Frame = Peer.m_aFrames[FrameIdx];
			const int16_t Pcm = Frame.m_aPcm[ReadPos];
			const float LeftGain = Frame.m_LeftGain;
			const float RightGain = Frame.m_RightGain;

			const int Base = i * OutputChannels;
			if(OutputChannels == 1)
			{
				const float MonoGain = 0.5f * (LeftGain + RightGain);
				m_MixBuffer[Base] += (int32_t)(Pcm * MonoGain);
			}
			else
			{
				m_MixBuffer[Base] += (int32_t)(Pcm * LeftGain);
				m_MixBuffer[Base + 1] += (int32_t)(Pcm * RightGain);
				if(OutputChannels > 2)
				{
					const int32_t Center = (int32_t)(Pcm * 0.5f * (LeftGain + RightGain));
					for(int ch = 2; ch < OutputChannels; ch++)
						m_MixBuffer[Base + ch] += Center;
				}
			}

			ReadPos++;
			if(ReadPos >= Frame.m_Samples)
			{
				ReadPos = 0;
				FrameIdx = (FrameIdx + 1) % SVoicePeer::MAX_FRAMES;
				FrameCount--;
			}
		}

		Peer.m_FrameHead = FrameIdx;
		Peer.m_FrameCount = FrameCount;
		Peer.m_FrameReadPos = ReadPos;
	}

	for(int i = 0; i < Needed; i++)
	{
		pOut[i] = (int16_t)std::clamp(m_MixBuffer[i], -32768, 32767);
	}
}

void CRClientVoice::ClearPeerFrames()
{
	if(m_OutputDevice)
		SDL_LockAudioDevice(m_OutputDevice);
	for(auto &Peer : m_aPeers)
	{
		for(auto &Pkt : Peer.m_aPackets)
		{
			Pkt.m_Valid = false;
			Pkt.m_Size = 0;
			Pkt.m_Seq = 0;
			Pkt.m_LeftGain = 1.0f;
			Pkt.m_RightGain = 1.0f;
		}
		Peer.m_QueuedPackets = 0;
		Peer.m_LastSeq = 0;
		Peer.m_HasSeq = false;
		Peer.m_HasNextSeq = false;
		Peer.m_NextSeq = 0;
		Peer.m_HasLastRecvSeq = false;
		Peer.m_LastRecvSeq = 0;
		Peer.m_LastRecvTime = 0;
		Peer.m_JitterMs = 0.0f;
		Peer.m_TargetFrames = 3;
		Peer.m_LastGainLeft = 1.0f;
		Peer.m_LastGainRight = 1.0f;
		Peer.m_LossEwma = 0.0f;
		if(Peer.m_pDecoder)
			opus_decoder_ctl(Peer.m_pDecoder, OPUS_RESET_STATE);
		Peer.m_FrameHead = 0;
		Peer.m_FrameTail = 0;
		Peer.m_FrameCount = 0;
		Peer.m_FrameReadPos = 0;
	}
	if(m_OutputDevice)
		SDL_UnlockAudioDevice(m_OutputDevice);
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
	if(m_ShutdownDone)
		return;
	m_ShutdownDone = true;

	StopWorker();

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
	m_OutputChannels.store(0);
	m_MixBuffer.clear();
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
	ClearPeerFrames();
	m_ServerAddrValid.store(false);
	m_aServerAddrStr[0] = '\0';
	m_HpfPrevIn = 0.0f;
	m_HpfPrevOut = 0.0f;
	m_CompEnv = 0.0f;
}

void CRClientVoice::UpdateServerAddr()
{
	if(str_comp(m_aServerAddrStr, g_Config.m_RiVoiceServer) == 0)
		return;

	str_copy(m_aServerAddrStr, g_Config.m_RiVoiceServer, sizeof(m_aServerAddrStr));
	m_ServerAddrValid.store(false);
	if(m_aServerAddrStr[0] == '\0')
		return;

	NETADDR NewAddr = NETADDR_ZEROED;
	if(net_addr_from_str(&NewAddr, m_aServerAddrStr) == 0)
	{
		{
			std::lock_guard<std::mutex> Guard(m_ServerAddrMutex);
			m_ServerAddr = NewAddr;
		}
		m_ServerAddrValid.store(true);
		return;
	}

	char aHost[128];
	int Port = 0;
	if(!ParseHostPort(m_aServerAddrStr, aHost, sizeof(aHost), Port))
	{
		log_error("voice", "Invalid voice server address '%s'", m_aServerAddrStr);
		return;
	}

	if(net_host_lookup(aHost, &NewAddr, NETTYPE_ALL) == 0)
	{
		NewAddr.port = Port;
		{
			std::lock_guard<std::mutex> Guard(m_ServerAddrMutex);
			m_ServerAddr = NewAddr;
		}
		m_ServerAddrValid.store(true);
		return;
	}

	log_error("voice", "Failed to resolve voice server '%s'", m_aServerAddrStr);
}

bool CRClientVoice::UpdateContext()
{
	const uint32_t Old = m_ContextHash.load();
	if(!m_pClient || m_pClient->State() != IClient::STATE_ONLINE)
	{
		m_ContextHash.store(0);
		return Old != 0;
	}
	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(&m_pClient->ServerAddress(), aAddr, sizeof(aAddr), true);
	const uint32_t NewHash = str_quickhash(aAddr);
	m_ContextHash.store(NewHash);
	return NewHash != Old;
}

void CRClientVoice::UpdateClientSnapshot()
{
	std::lock_guard<std::mutex> Guard(m_SnapshotMutex);
	if(!m_pClient || !m_pGameClient || m_pClient->State() != IClient::STATE_ONLINE)
	{
		m_OnlineSnap = false;
		m_LocalClientIdSnap = -1;
		return;
	}

	m_OnlineSnap = true;
	m_LocalClientIdSnap = m_pGameClient->m_Snap.m_LocalClientId;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aClientPosSnap[i] = m_pGameClient->m_aClients[i].m_RenderPos;
		str_copy(m_aClientNameSnap[i].data(), m_pGameClient->m_aClients[i].m_aName, MAX_NAME_LENGTH);
		m_aClientOtherTeamSnap[i] = m_pGameClient->IsOtherTeam(i) ? 1 : 0;
	}
}

void CRClientVoice::ProcessCapture()
{
	if(!m_CaptureDevice || !m_pEncoder || !m_ServerAddrValid.load() || !m_Socket)
		return;

	SRClientVoiceConfigSnapshot Config;
	GetConfigSnapshot(Config);

	int LocalClientId = -1;
	vec2 LocalPos = vec2(0.0f, 0.0f);
	bool Online = false;
	{
		std::lock_guard<std::mutex> Guard(m_SnapshotMutex);
		Online = m_OnlineSnap;
		LocalClientId = m_LocalClientIdSnap;
		if(LocalClientId >= 0 && LocalClientId < MAX_CLIENTS)
			LocalPos = m_aClientPosSnap[LocalClientId];
	}
	if(!Online || LocalClientId < 0 || LocalClientId >= MAX_CLIENTS)
		return;

	const int64_t Now = time_get();
	if(!m_PttActive.load())
	{
		if(m_LastKeepalive == 0 || Now - m_LastKeepalive > time_freq() * 2)
		{
			NETADDR ServerAddrLocal = NETADDR_ZEROED;
			{
				std::lock_guard<std::mutex> Guard(m_ServerAddrMutex);
				ServerAddrLocal = m_ServerAddr;
			}
			uint8_t aPacket[VOICE_MAX_PACKET];
			size_t Offset = 0;
			mem_copy(aPacket + Offset, VOICE_MAGIC, sizeof(VOICE_MAGIC));
			Offset += sizeof(VOICE_MAGIC);
			aPacket[Offset++] = VOICE_VERSION;
			aPacket[Offset++] = VOICE_TYPE_PING;
			WriteU16(aPacket + Offset, 0);
			Offset += sizeof(uint16_t);
			WriteU32(aPacket + Offset, m_ContextHash.load());
			Offset += sizeof(uint32_t);
			WriteU32(aPacket + Offset, Config.m_RiVoiceTokenHash);
			Offset += sizeof(uint32_t);
			WriteU16(aPacket + Offset, 0);
			Offset += sizeof(uint16_t);
			WriteU16(aPacket + Offset, m_Sequence++);
			Offset += sizeof(uint16_t);
			WriteFloat(aPacket + Offset, 0.0f);
			Offset += sizeof(float);
			WriteFloat(aPacket + Offset, 0.0f);
			Offset += sizeof(float);
			net_udp_send(m_Socket, &ServerAddrLocal, aPacket, (int)Offset);
			m_LastKeepalive = Now;
		}
	}

	if(!m_PttActive.load())
	{
		SDL_ClearQueuedAudio(m_CaptureDevice);
		return;
	}

	static int64_t s_TxLastLog = 0;
	static int s_TxPackets = 0;

	const int ClientId = LocalClientId;
	m_aLastHeard[ClientId].store(time_get());

	const vec2 Pos = LocalPos;
		uint8_t aPacket[VOICE_MAX_PACKET];
		uint8_t aPayload[VOICE_MAX_PAYLOAD];

	while(SDL_GetQueuedAudioSize(m_CaptureDevice) >= VOICE_FRAME_BYTES)
	{
		int16_t aPcm[VOICE_FRAME_SAMPLES];
		SDL_DequeueAudio(m_CaptureDevice, aPcm, VOICE_FRAME_BYTES);
		ApplyHpfCompressor(Config, aPcm, VOICE_FRAME_SAMPLES, m_HpfPrevIn, m_HpfPrevOut, m_CompEnv);

		const int EncSize = opus_encode(m_pEncoder, aPcm, VOICE_FRAME_SAMPLES, aPayload, (int)sizeof(aPayload));
		if(EncSize <= 0)
			continue;
		if(EncSize > VOICE_MAX_PAYLOAD)
			continue;

		size_t Offset = 0;
		mem_copy(aPacket + Offset, VOICE_MAGIC, sizeof(VOICE_MAGIC));
		Offset += sizeof(VOICE_MAGIC);
		aPacket[Offset++] = VOICE_VERSION;
		aPacket[Offset++] = VOICE_TYPE_AUDIO;
		WriteU16(aPacket + Offset, (uint16_t)EncSize);
		Offset += sizeof(uint16_t);
		WriteU32(aPacket + Offset, m_ContextHash.load());
		Offset += sizeof(uint32_t);
		WriteU32(aPacket + Offset, Config.m_RiVoiceTokenHash);
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

		NETADDR ServerAddrLocal = NETADDR_ZEROED;
		{
			std::lock_guard<std::mutex> Guard(m_ServerAddrMutex);
			ServerAddrLocal = m_ServerAddr;
		}
		net_udp_send(m_Socket, &ServerAddrLocal, aPacket, (int)Offset);
		if(Config.m_RiVoiceDebug)
		{
			s_TxPackets++;
			if(Now - s_TxLastLog > time_freq())
			{
				log_info("voice", "tx packets=%d ctx=0x%08x", s_TxPackets, m_ContextHash.load());
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

	SRClientVoiceConfigSnapshot Config;
	GetConfigSnapshot(Config);

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

		NETADDR ServerAddrLocal = NETADDR_ZEROED;
		{
			std::lock_guard<std::mutex> Guard(m_ServerAddrMutex);
			ServerAddrLocal = m_ServerAddr;
		}
		if(net_addr_comp(&Addr, &ServerAddrLocal) != 0)
			continue;

		if(Bytes < (int)(sizeof(VOICE_MAGIC) + 2 + 2 + 4 + 4 + 2 + 2 + 8))
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
		const uint32_t TokenHash = ReadU32(pData + Offset);
		Offset += sizeof(uint32_t);
		const uint16_t SenderId = ReadU16(pData + Offset);
		Offset += sizeof(uint16_t);
		const uint16_t Sequence = ReadU16(pData + Offset);
		Offset += sizeof(uint16_t);
		const float PosX = SanitizeFloat(ReadFloat(pData + Offset));
		Offset += sizeof(float);
		const float PosY = SanitizeFloat(ReadFloat(pData + Offset));
		Offset += sizeof(float);

		const uint32_t LocalContextHash = m_ContextHash.load();
		if(ContextHash == 0 || ContextHash != LocalContextHash)
		{
			s_RxDropContext++;
			continue;
		}
		if(Config.m_RiVoiceTokenHash != 0 && TokenHash != Config.m_RiVoiceTokenHash)
			continue;
		if(SenderId >= MAX_CLIENTS)
			continue;

		int LocalId = -1;
		vec2 LocalPos = vec2(0.0f, 0.0f);
		char aSenderName[MAX_NAME_LENGTH];
		bool SenderOtherTeam = false;
		{
			std::lock_guard<std::mutex> Guard(m_SnapshotMutex);
			if(!m_OnlineSnap)
				continue;
			LocalId = m_LocalClientIdSnap;
			if(LocalId < 0 || LocalId >= MAX_CLIENTS)
				continue;
			LocalPos = m_aClientPosSnap[LocalId];
			str_copy(aSenderName, m_aClientNameSnap[SenderId].data(), sizeof(aSenderName));
			SenderOtherTeam = m_aClientOtherTeamSnap[SenderId] != 0;
		}

		if(SenderId == LocalId)
			continue;
		if(Config.m_ClShowOthers != SHOW_OTHERS_ON && SenderOtherTeam)
			continue;
		const char *pSenderName = aSenderName;
		if(VoiceListMatch(Config.m_aRiVoiceMute, pSenderName))
			continue;
		if(Config.m_RiVoiceListMode == 1 && !VoiceListMatch(Config.m_aRiVoiceWhitelist, pSenderName))
			continue;
		if(Config.m_RiVoiceListMode == 2 && VoiceListMatch(Config.m_aRiVoiceBlacklist, pSenderName))
			continue;
		m_aLastHeard[SenderId].store(time_get());

		if(PayloadSize > VOICE_MAX_PAYLOAD)
			continue;
		if(Offset + PayloadSize > (size_t)Bytes)
			continue;

		const vec2 SenderPos = vec2(PosX, PosY);
		const float Radius = std::max(1, Config.m_RiVoiceRadius) * 32.0f;
		const float Dist = distance(LocalPos, SenderPos);
		if(!Config.m_RiVoiceIgnoreDistance && Dist > Radius)
		{
			s_RxDropRadius++;
			continue;
		}

		const float RadiusFactor = Config.m_RiVoiceIgnoreDistance ? 1.0f : (1.0f - (Dist / Radius));
		float Volume = std::clamp(RadiusFactor * (Config.m_RiVoiceVolume / 100.0f), 0.0f, 2.0f);
		if(Volume <= 0.0f)
			continue;

		int NameVolume = 100;
		if(VoiceNameVolume(Config.m_aRiVoiceNameVolumes, pSenderName, NameVolume))
		{
			Volume *= (NameVolume / 100.0f);
			if(Volume <= 0.0f)
				continue;
		}

		const bool StereoEnabled = Config.m_RiVoiceStereo != 0;
		const float Pan = StereoEnabled ? std::clamp((SenderPos.x - LocalPos.x) / Radius, -1.0f, 1.0f) : 0.0f;
		const float LeftGain = Volume * (Pan <= 0.0f ? 1.0f : (1.0f - Pan));
		const float RightGain = Volume * (Pan >= 0.0f ? 1.0f : (1.0f + Pan));

		SVoicePeer &Peer = m_aPeers[SenderId];
		const int64_t Now = time_get();
		if(Peer.m_LastRecvTime != 0)
		{
			const float DeltaMs = (float)((Now - Peer.m_LastRecvTime) * 1000.0 / (double)time_freq());
			const float Deviation = std::fabs(DeltaMs - 20.0f);
			Peer.m_JitterMs = 0.9f * Peer.m_JitterMs + 0.1f * Deviation;
		}
		Peer.m_LastRecvTime = Now;

		int Target = ClampJitterTarget(Peer.m_JitterMs);
		if(Peer.m_HasLastRecvSeq)
		{
			const uint16_t Expected = (uint16_t)(Peer.m_LastRecvSeq + 1);
			if(Sequence != Expected)
				Target = std::min(Target + 1, 6);
		}
		Peer.m_TargetFrames = Target;
		if(Peer.m_HasLastRecvSeq)
		{
			const int Delta = SeqDelta(Sequence, Peer.m_LastRecvSeq);
			if(Delta > 0 && Delta < 1000)
			{
				const int Lost = std::max(0, Delta - 1);
				const float LossRatio = std::clamp(Lost / (float)Delta, 0.0f, 1.0f);
				Peer.m_LossEwma = 0.9f * Peer.m_LossEwma + 0.1f * LossRatio;
			}
		}
		Peer.m_LastRecvSeq = Sequence;
		Peer.m_HasLastRecvSeq = true;
		Peer.m_LastGainLeft = LeftGain;
		Peer.m_LastGainRight = RightGain;

		const int Slot = Sequence % SVoicePeer::MAX_JITTER_PACKETS;
		SVoicePeer::SJitterPacket &Pkt = Peer.m_aPackets[Slot];
		if(Pkt.m_Valid && Pkt.m_Seq != Sequence)
			Peer.m_QueuedPackets = std::max(0, Peer.m_QueuedPackets - 1);
		if(!Pkt.m_Valid || Pkt.m_Seq != Sequence)
			Peer.m_QueuedPackets = std::min(Peer.m_QueuedPackets + 1, SVoicePeer::MAX_JITTER_PACKETS);
		Pkt.m_Valid = true;
		Pkt.m_Seq = Sequence;
		Pkt.m_Size = PayloadSize;
		Pkt.m_LeftGain = LeftGain;
		Pkt.m_RightGain = RightGain;
		mem_copy(Pkt.m_aData, pData + Offset, PayloadSize);

		if(Config.m_RiVoiceDebug)
		{
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

void CRClientVoice::UpdateConfigSnapshot()
{
	std::lock_guard<std::mutex> Guard(m_ConfigMutex);
	m_ConfigSnapshot.m_RiVoiceFilterEnable = g_Config.m_RiVoiceFilterEnable;
	m_ConfigSnapshot.m_RiVoiceCompThreshold = g_Config.m_RiVoiceCompThreshold;
	m_ConfigSnapshot.m_RiVoiceCompRatio = g_Config.m_RiVoiceCompRatio;
	m_ConfigSnapshot.m_RiVoiceCompAttackMs = g_Config.m_RiVoiceCompAttackMs;
	m_ConfigSnapshot.m_RiVoiceCompReleaseMs = g_Config.m_RiVoiceCompReleaseMs;
	m_ConfigSnapshot.m_RiVoiceCompMakeup = g_Config.m_RiVoiceCompMakeup;
	m_ConfigSnapshot.m_RiVoiceLimiter = g_Config.m_RiVoiceLimiter;
	m_ConfigSnapshot.m_RiVoiceStereo = g_Config.m_RiVoiceStereo;
	m_ConfigSnapshot.m_RiVoiceRadius = g_Config.m_RiVoiceRadius;
	m_ConfigSnapshot.m_RiVoiceVolume = g_Config.m_RiVoiceVolume;
	m_ConfigSnapshot.m_RiVoiceIgnoreDistance = g_Config.m_RiVoiceIgnoreDistance;
	m_ConfigSnapshot.m_RiVoiceListMode = g_Config.m_RiVoiceListMode;
	m_ConfigSnapshot.m_RiVoiceDebug = g_Config.m_RiVoiceDebug;
	m_ConfigSnapshot.m_ClShowOthers = g_Config.m_ClShowOthers;
	m_ConfigSnapshot.m_RiVoiceTokenHash = g_Config.m_RiVoiceToken[0] != '\0' ? str_quickhash(g_Config.m_RiVoiceToken) : 0;
	str_copy(m_ConfigSnapshot.m_aRiVoiceWhitelist, g_Config.m_RiVoiceWhitelist, sizeof(m_ConfigSnapshot.m_aRiVoiceWhitelist));
	str_copy(m_ConfigSnapshot.m_aRiVoiceBlacklist, g_Config.m_RiVoiceBlacklist, sizeof(m_ConfigSnapshot.m_aRiVoiceBlacklist));
	str_copy(m_ConfigSnapshot.m_aRiVoiceMute, g_Config.m_RiVoiceMute, sizeof(m_ConfigSnapshot.m_aRiVoiceMute));
	str_copy(m_ConfigSnapshot.m_aRiVoiceNameVolumes, g_Config.m_RiVoiceNameVolumes, sizeof(m_ConfigSnapshot.m_aRiVoiceNameVolumes));
}

void CRClientVoice::GetConfigSnapshot(SRClientVoiceConfigSnapshot &Out) const
{
	std::lock_guard<std::mutex> Guard(m_ConfigMutex);
	Out = m_ConfigSnapshot;
}

void CRClientVoice::UpdateEncoderParams()
{
	if(!m_pEncoder)
		return;

	const int64_t Now = time_get();
	if(m_LastEncUpdate != 0 && Now - m_LastEncUpdate < time_freq())
		return;

	float LossAvg = 0.0f;
	float JitterMax = 0.0f;
	int Count = 0;
	for(const auto &Peer : m_aPeers)
	{
		if(Peer.m_LastRecvTime == 0)
			continue;
		if(Now - Peer.m_LastRecvTime > time_freq() * 5)
			continue;
		LossAvg += Peer.m_LossEwma;
		JitterMax = std::max(JitterMax, Peer.m_JitterMs);
		Count++;
	}
	if(Count > 0)
		LossAvg /= (float)Count;

	const int LossPerc = (int)std::clamp(LossAvg * 100.0f, 0.0f, 30.0f);

	int TargetBitrate = 24000;
	int TargetLoss = 0;
	bool TargetFec = false;

	if(LossPerc <= 2 && JitterMax < 8.0f)
	{
		TargetBitrate = 32000;
		TargetLoss = 0;
		TargetFec = false;
	}
	else if(LossPerc <= 5)
	{
		TargetBitrate = 24000;
		TargetLoss = 5;
		TargetFec = true;
	}
	else if(LossPerc <= 10)
	{
		TargetBitrate = 20000;
		TargetLoss = 10;
		TargetFec = true;
	}
	else
	{
		TargetBitrate = 16000;
		TargetLoss = 20;
		TargetFec = true;
	}

	if(TargetBitrate != m_EncBitrate)
	{
		opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(TargetBitrate));
		m_EncBitrate = TargetBitrate;
	}
	if(TargetLoss != m_EncLossPerc)
	{
		opus_encoder_ctl(m_pEncoder, OPUS_SET_PACKET_LOSS_PERC(TargetLoss));
		m_EncLossPerc = TargetLoss;
	}
	if(TargetFec != m_EncFec)
	{
		opus_encoder_ctl(m_pEncoder, OPUS_SET_INBAND_FEC(TargetFec ? 1 : 0));
		m_EncFec = TargetFec;
	}

	m_LastEncUpdate = Now;
}

void CRClientVoice::DecodeJitter()
{
	if(!m_OutputDevice)
		return;

	for(int PeerId = 0; PeerId < MAX_CLIENTS; PeerId++)
	{
		SVoicePeer &Peer = m_aPeers[PeerId];
		if(Peer.m_QueuedPackets <= 0)
			continue;

		if(!Peer.m_HasNextSeq)
		{
			if(Peer.m_QueuedPackets < Peer.m_TargetFrames)
				continue;
			bool Found = false;
			uint16_t StartSeq = 0;
			for(const auto &Pkt : Peer.m_aPackets)
			{
				if(!Pkt.m_Valid)
					continue;
				if(!Found)
				{
					StartSeq = Pkt.m_Seq;
					Found = true;
					continue;
				}
				if(SeqLess(Pkt.m_Seq, StartSeq))
					StartSeq = Pkt.m_Seq;
			}
			if(!Found)
				continue;
			Peer.m_NextSeq = StartSeq;
			Peer.m_HasNextSeq = true;
		}

		int FrameCount = 0;
		SDL_LockAudioDevice(m_OutputDevice);
		FrameCount = Peer.m_FrameCount;
		SDL_UnlockAudioDevice(m_OutputDevice);
		if(FrameCount >= SVoicePeer::MAX_FRAMES)
			continue;

		const int Slot = Peer.m_NextSeq % SVoicePeer::MAX_JITTER_PACKETS;
		SVoicePeer::SJitterPacket *pPkt = nullptr;
		if(Peer.m_aPackets[Slot].m_Valid && Peer.m_aPackets[Slot].m_Seq == Peer.m_NextSeq)
			pPkt = &Peer.m_aPackets[Slot];
		const int NextSlot = (uint16_t)(Peer.m_NextSeq + 1) % SVoicePeer::MAX_JITTER_PACKETS;
		SVoicePeer::SJitterPacket *pNextPkt = nullptr;
		if(Peer.m_aPackets[NextSlot].m_Valid && Peer.m_aPackets[NextSlot].m_Seq == (uint16_t)(Peer.m_NextSeq + 1))
			pNextPkt = &Peer.m_aPackets[NextSlot];

		if(!Peer.m_pDecoder)
		{
			int Error = 0;
			Peer.m_pDecoder = opus_decoder_create(VOICE_SAMPLE_RATE, VOICE_CHANNELS, &Error);
			if(!Peer.m_pDecoder || Error != OPUS_OK)
			{
				log_error("voice", "Failed to create Opus decoder: %d", Error);
				continue;
			}
			Peer.m_HasSeq = false;
		}

		int16_t aPcm[VOICE_FRAME_SAMPLES];
		int Samples = 0;
		float LeftGain = Peer.m_LastGainLeft;
		float RightGain = Peer.m_LastGainRight;
		if(pPkt)
		{
			Samples = opus_decode(Peer.m_pDecoder, pPkt->m_aData, pPkt->m_Size, aPcm, VOICE_FRAME_SAMPLES, 0);
			if(Samples > 0)
			{
				LeftGain = pPkt->m_LeftGain;
				RightGain = pPkt->m_RightGain;
			}
			pPkt->m_Valid = false;
			Peer.m_QueuedPackets = std::max(0, Peer.m_QueuedPackets - 1);
		}
		else if(pNextPkt && Peer.m_LossEwma > 0.02f)
		{
			Samples = opus_decode(Peer.m_pDecoder, pNextPkt->m_aData, pNextPkt->m_Size, aPcm, VOICE_FRAME_SAMPLES, 1);
		}
		else if(Peer.m_HasSeq)
		{
			Samples = opus_decode(Peer.m_pDecoder, nullptr, 0, aPcm, VOICE_FRAME_SAMPLES, 1);
		}

		if(Samples > 0)
		{
			SDL_LockAudioDevice(m_OutputDevice);
			PushPeerFrame(PeerId, aPcm, Samples, LeftGain, RightGain);
			SDL_UnlockAudioDevice(m_OutputDevice);
		}

		Peer.m_LastSeq = Peer.m_NextSeq;
		Peer.m_HasSeq = true;
		Peer.m_NextSeq = (uint16_t)(Peer.m_NextSeq + 1);
	}
}

bool CRClientVoice::IsVoiceActive(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;
	const int64_t LastHeard = m_aLastHeard[ClientId].load();
	if(LastHeard == 0)
		return false;
	return time_get() - LastHeard < time_freq() / 2;
}

void CRClientVoice::StartWorker()
{
	if(m_Worker.joinable())
		return;
	m_WorkerStop.store(false);
	m_WorkerEnabled.store(true);
	m_Worker = std::thread(&CRClientVoice::WorkerLoop, this);
}

void CRClientVoice::StopWorker()
{
	m_WorkerEnabled.store(false);
	if(m_Worker.joinable())
	{
		m_WorkerStop.store(true);
		m_Worker.join();
	}
	m_WorkerStop.store(false);
}

void CRClientVoice::WorkerLoop()
{
	using namespace std::chrono_literals;
	while(!m_WorkerStop.load())
	{
		if(!m_WorkerEnabled.load())
		{
			std::this_thread::sleep_for(10ms);
			continue;
		}

		ProcessIncoming();
		DecodeJitter();
		UpdateEncoderParams();
		ProcessCapture();

		std::this_thread::sleep_for(2ms);
	}
}

void CRClientVoice::OnRender()
{
	if(!g_Config.m_RiVoiceEnable || !m_pGameClient || !m_pClient)
	{
		Shutdown();
		return;
	}
	m_ShutdownDone = false;

	UpdateServerAddr();
	const bool ContextChanged = UpdateContext();
	UpdateClientSnapshot();
	UpdateConfigSnapshot();

	const bool WantStereo = g_Config.m_RiVoiceStereo != 0;
	const int DesiredChannels = WantStereo ? 2 : 1;
	bool NeedReinit = false;
	if(str_comp(m_aInputDeviceName, g_Config.m_RiVoiceInputDevice) != 0)
		NeedReinit = true;
	if(str_comp(m_aOutputDeviceName, g_Config.m_RiVoiceOutputDevice) != 0)
		NeedReinit = true;
	if(m_OutputStereo != WantStereo)
		NeedReinit = true;
	if(m_OutputDevice && m_OutputSpec.channels > 0 && m_OutputSpec.channels != DesiredChannels)
	{
		SDL_CloseAudioDevice(m_OutputDevice);
		m_OutputDevice = 0;
		m_OutputSpec = {};
		m_OutputChannels.store(0);
		m_MixBuffer.clear();
		NeedReinit = true;
	}
	if(!m_CaptureDevice || !m_OutputDevice || !m_pEncoder)
		NeedReinit = true;
	if(ContextChanged)
	{
		StopWorker();
		ClearPeerFrames();
	}

	if(!m_ServerAddrValid.load())
	{
		StopWorker();
		return;
	}
	if(NeedReinit)
		StopWorker();
	if(!EnsureSocket() || !EnsureAudio())
	{
		StopWorker();
		return;
	}

	StartWorker();
}
