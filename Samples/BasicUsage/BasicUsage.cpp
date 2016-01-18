// BasicUsage.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"




ComPtr<IXAudio2> CreateXAudioEngine()
{
	ComPtr<IXAudio2> audioEngine;
	// Aluminum Shower Commode Mobile 
	HR(XAudio2Create(audioEngine.GetAddressOf()));

#if defined _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfig{};
	debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	audioEngine->SetDebugConfiguration(&debugConfig);

#endif

	return audioEngine;
}


IXAudio2MasteringVoice * CreateMasteringVoice(ComPtr<IXAudio2> const & audioEngine)
{
	IXAudio2MasteringVoice * masteringVoice{};
	HR(audioEngine->CreateMasteringVoice(&masteringVoice));

	return masteringVoice;
}

IXAudio2SourceVoice * CreateSourceVoice(ComPtr<IXAudio2> const & audioEngine, WAVEFORMATEX const & waveFormatEx)
{
	IXAudio2SourceVoice * sourceVoice{};


	HR(audioEngine->CreateSourceVoice(&sourceVoice, &waveFormatEx));

	return sourceVoice;
}

int wmain(int argc, wchar_t ** argv)
{
	// Audio Decoder stuff
	AudioDecoder audioDecoder{};

	auto audio = audioDecoder.LoadAudio(argv[1]);

	// Create XAudio2 stuff
	auto xAudioEngine = CreateXAudioEngine();
	auto masteringVoice = CreateMasteringVoice(xAudioEngine);
	auto sourceVoice = CreateSourceVoice(xAudioEngine, *audio.m_waveFormatEx);


	XAUDIO2_BUFFER xAudioBuffer{};
	xAudioBuffer.AudioBytes = audio.m_data.size();
	xAudioBuffer.pAudioData = static_cast<BYTE* const>(&audio.m_data[0]);
	xAudioBuffer.pContext = nullptr;

	sourceVoice->Start();

	HR(sourceVoice->SubmitSourceBuffer(&xAudioBuffer));

	// Sleep for some time to hear to song by preventing the main thread from sleep
	// XAudio2 plays the sound on a seperate audio thread :)
	Sleep(1000000);

	return 0;
}
