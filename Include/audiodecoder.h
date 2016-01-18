#pragma once

#include <crtdbg.h>
#include <algorithm>

#include <vector>

#include "wrl.h"

#include "initguid.h"
#include <mfapi.h>
#include <mfidl.h>
#include "mfreadwrite.h"
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")


#ifdef _DEBUG
#define ASSERT(expression) _ASSERTE(expression)
#define VERIFY(expression) ASSERT(expression)
#define HR(expression) ASSERT(S_OK == (expression))

#include <stdio.h>
inline void TRACE(WCHAR const * const format, ...)
{
	va_list args;
	va_start(args, format);
	WCHAR output[512];
	vswprintf_s(output, format, args);
	OutputDebugStringW(output);
	va_end(args);
}
#else
#define TRACE __noop
#define VERIFY(expression) (expression)

inline void HR(HRESULT const hr)
{
	if (S_OK != hr) throw ComException(hr);
}

#endif

using namespace Microsoft::WRL;

struct NotAudioFileException
{
	
};

struct MediaFoundationInitialize
{
	MediaFoundationInitialize()
	{
		HR(MFStartup(MF_VERSION));
	}

	~MediaFoundationInitialize()
	{
		HR(MFShutdown());
	}
};


struct Audio
{
	WAVEFORMATEX * m_waveFormatEx;
	std::vector<BYTE> m_data;

	Audio(WAVEFORMATEX * waveformatex, std::vector<BYTE> data) :
		m_waveFormatEx{waveformatex},
		m_data{data}
	{}

	~Audio()
	{
		CoTaskMemFree(m_waveFormatEx);
	}
};


struct AudioDecoder
{
	MediaFoundationInitialize m_mediaFoundation;
	ComPtr<IMFSourceReader> m_sourceReader;
	WAVEFORMATEX * m_waveFormat;

	AudioDecoder() :
		m_mediaFoundation{},
		m_sourceReader{},
		m_waveFormat{}
	{ }


	std::vector<unsigned char> ReadAudioBytes()
	{
		std::vector<BYTE> bytes;
		// Get Sample
		ComPtr<IMFSample> sample;


		while (true)
		{
			DWORD flags{};
			HR(m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, sample.GetAddressOf()));

			// Check for eof
			if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				return bytes;
			}

			// Convert data to contiguous buffer
			ComPtr<IMFMediaBuffer> buffer;
			HR(sample->ConvertToContiguousBuffer(buffer.GetAddressOf()));

			// Lock Buffer & copy to local memory
			BYTE* audioData = nullptr;
			DWORD audioDataLength{};
			HR(buffer->Lock(&audioData, nullptr, &audioDataLength));

			for (size_t i = 0; i < audioDataLength; i++)
			{
				bytes.push_back(*(audioData + i));
			}

			// Unlock Buffer
			HR(buffer->Unlock());
		}
	}

	Audio LoadAudio(wchar_t * const fileUrl)
	{
		m_sourceReader = CreateSourceReader(fileUrl);
		ASSERT(m_sourceReader);

		auto nativeMediaType = GetMediaType(m_sourceReader);
		ValidateIsAudioFile(nativeMediaType);

		if (IsAudoFileCompressed(nativeMediaType))
		{
			ConfigureDecoder(m_sourceReader);
		}

		m_waveFormat = CreateWaveFormat(m_sourceReader);

		return Audio(m_waveFormat, ReadAudioBytes());

	}

private:
	inline ComPtr<IMFSourceReader> CreateSourceReader(wchar_t * const fileUrl)
	{
		// Create Attribute Store
		ComPtr<IMFAttributes> sourceReaderConfiguration;
		HR(MFCreateAttributes(sourceReaderConfiguration.GetAddressOf(), 1));

		HR(sourceReaderConfiguration->SetUINT32(MF_LOW_LATENCY, true));
		HR(sourceReaderConfiguration->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, true));

		// Create Source Reader
		ComPtr<IMFSourceReader> sourceReader;

		HR(MFCreateSourceReaderFromURL(fileUrl, sourceReaderConfiguration.Get(), sourceReader.GetAddressOf()));

		return sourceReader;
	}

	ComPtr<IMFMediaType> GetMediaType(ComPtr<IMFSourceReader> const & sourceReader)
	{
		ComPtr<IMFMediaType> nativeMediaType = nullptr;
		HR(sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nativeMediaType.GetAddressOf()));

		return nativeMediaType;
	}

	void ValidateIsAudioFile(ComPtr<IMFMediaType> const & mediaType)
	{

		GUID majorType{};
		HR(mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));
		if (MFMediaType_Audio != majorType)
		{
			throw NotAudioFileException{};
		}
	}

	bool IsAudoFileCompressed(ComPtr<IMFMediaType> const & mediaType)
	{
		GUID subType{};
		HR(mediaType->GetGUID(MF_MT_MAJOR_TYPE, &subType));
		if (MFAudioFormat_Float == subType || MFAudioFormat_PCM == subType)
		{
			return false;
		}
		return true;
	}

	WAVEFORMATEX * CreateWaveFormat(ComPtr<IMFSourceReader> const & sourceReader)
	{
		ComPtr<IMFMediaType> uncompressedAudioType = nullptr;
		HR(sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, uncompressedAudioType.GetAddressOf()));
		WAVEFORMATEX * waveformatex;
		unsigned int waveformatlength;
		HR(MFCreateWaveFormatExFromMFMediaType(uncompressedAudioType.Get(), &waveformatex, &waveformatlength));
		return waveformatex;
	}

	void ConfigureDecoder(ComPtr<IMFSourceReader> const & sourceReader)
	{
		ComPtr<IMFMediaType> partialType = nullptr;
		HR(MFCreateMediaType(partialType.GetAddressOf()));

		HR(partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
		HR(partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
		HR(sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, partialType.Get()));
	}

};
