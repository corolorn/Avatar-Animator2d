#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

struct AudioLevel {
	float peak; // 0..1
};

class WasapiCapture {
public:
	WasapiCapture();
	~WasapiCapture();
	bool initializeDefaultCapture();
	bool initializeById(LPCWSTR deviceId);
	bool start();
	void stop();
	AudioLevel pollLevel();
 
 	static bool EnumerateCaptureNames(HWND combo);

private:
	IMMDevice* device_ = nullptr;
	IAudioClient* client_ = nullptr;
	IAudioCaptureClient* capture_ = nullptr;
	HANDLE event_ = nullptr;
	WAVEFORMATEX* mix_ = nullptr;
	float smoothed_ = 0.0f;
};


