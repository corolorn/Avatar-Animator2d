#include "audio_capture.h"
#include <functiondiscoverykeys.h>
#include <mmreg.h>
#include <avrt.h>
#include <cmath>

static float compute_peak_mono(const BYTE* data, UINT frames, const WAVEFORMATEX* wfx) {
	if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)wfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
		const int ch = wfx->nChannels;
		const float* f = (const float*)data;
		float peak = 0.f;
		for (UINT i = 0; i < frames; ++i) {
			float sum = 0.f;
			for (int c = 0; c < ch; ++c) sum += std::fabs(f[i * ch + c]);
			sum /= (float)ch;
			if (sum > peak) peak = sum;
		}
		return peak;
	}
	if (wfx->wBitsPerSample == 16) {
		const int ch = wfx->nChannels;
		const short* s = (const short*)data;
		float peak = 0.f;
		for (UINT i = 0; i < frames; ++i) {
			float sum = 0.f;
			for (int c = 0; c < ch; ++c) sum += std::fabs(s[i * ch + c]) / 32768.0f;
			sum /= (float)ch;
			if (sum > peak) peak = sum;
		}
		return peak;
	}
	return 0.f;
}

WasapiCapture::WasapiCapture() {}
WasapiCapture::~WasapiCapture() { stop(); if (mix_) CoTaskMemFree(mix_); if (capture_) capture_->Release(); if (client_) client_->Release(); if (device_) device_->Release(); if (event_) CloseHandle(event_); }

bool WasapiCapture::initializeDefaultCapture() {
	IMMDeviceEnumerator* en = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&en));
	if (FAILED(hr)) return false;
	hr = en->GetDefaultAudioEndpoint(eCapture, eCommunications, &device_);
	en->Release();
	if (FAILED(hr)) return false;
	hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr, (void**)&client_);
	if (FAILED(hr)) return false;
	hr = client_->GetMixFormat(&mix_);
	if (FAILED(hr)) return false;
	hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 20 * 10000 /* 20ms */, 0, mix_, nullptr);
	if (FAILED(hr)) return false;
	event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	client_->SetEventHandle(event_);
	hr = client_->GetService(__uuidof(IAudioCaptureClient), (void**)&capture_);
	return SUCCEEDED(hr);
}

bool WasapiCapture::initializeById(LPCWSTR deviceId) {
	if (client_) { stop(); capture_->Release(); client_->Release(); client_ = nullptr; capture_ = nullptr; }
	if (device_) { device_->Release(); device_ = nullptr; }
	if (mix_) { CoTaskMemFree(mix_); mix_ = nullptr; }
	IMMDeviceEnumerator* en = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&en));
	if (FAILED(hr)) return false;
	hr = en->GetDevice(deviceId, &device_);
	en->Release();
	if (FAILED(hr)) return false;
	hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr, (void**)&client_);
	if (FAILED(hr)) return false;
	hr = client_->GetMixFormat(&mix_);
	if (FAILED(hr)) return false;
	hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 20 * 10000, 0, mix_, nullptr);
	if (FAILED(hr)) return false;
	if (!event_) event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	client_->SetEventHandle(event_);
	hr = client_->GetService(__uuidof(IAudioCaptureClient), (void**)&capture_);
	return SUCCEEDED(hr);
}

bool WasapiCapture::EnumerateCaptureNames(HWND combo) {
	SendMessage(combo, CB_RESETCONTENT, 0, 0);
	IMMDeviceEnumerator* en = nullptr; IMMDeviceCollection* col = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&en)))) return false;
	if (FAILED(en->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &col))) { en->Release(); return false; }
	UINT count = 0; col->GetCount(&count);
	for (UINT i = 0; i < count; ++i) {
		IMMDevice* dev = nullptr; if (FAILED(col->Item(i, &dev))) continue;
		LPWSTR id = nullptr; dev->GetId(&id);
		IPropertyStore* store = nullptr; dev->OpenPropertyStore(STGM_READ, &store);
		PROPVARIANT name; PropVariantInit(&name);
		if (store && SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &name))) {
			int idx = (int)SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)name.pwszVal);
			SendMessage(combo, CB_SETITEMDATA, idx, (LPARAM)id);
			PropVariantClear(&name);
		}
		if (store) store->Release();
		dev->Release();
	}
	col->Release(); en->Release();
	return true;
}

bool WasapiCapture::start() {
	if (!client_) return false;
	return SUCCEEDED(client_->Start());
}

void WasapiCapture::stop() {
	if (client_) client_->Stop();
}

AudioLevel WasapiCapture::pollLevel() {
	AudioLevel lvl{ 0.f };
	if (!capture_) return lvl;
	UINT32 packet = 0;
	if (FAILED(capture_->GetNextPacketSize(&packet))) return lvl;
	while (packet) {
		BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0;
		auto hr = capture_->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
		if (SUCCEEDED(hr)) {
			float p = compute_peak_mono(data, frames, mix_);
			lvl.peak = p > lvl.peak ? p : lvl.peak;
			capture_->ReleaseBuffer(frames);
		}
		if (FAILED(capture_->GetNextPacketSize(&packet))) break;
	}
	// simple smoothing
	const float attack = 0.3f, release = 0.05f; // instantaneous attack, slower fall
	if (lvl.peak > smoothed_) smoothed_ = smoothed_ + (lvl.peak - smoothed_) * attack; else smoothed_ = smoothed_ + (lvl.peak - smoothed_) * release;
	lvl.peak = smoothed_;
	return lvl;
}


