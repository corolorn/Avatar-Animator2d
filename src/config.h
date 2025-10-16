#pragma once
#include <windows.h>

struct AppConfig {
	COLORREF chromaKey = RGB(0,255,0);
	float thOn = 0.15f;
	float thOff = 0.10f;
	wchar_t idlePath[MAX_PATH]{};
	wchar_t talkPath[MAX_PATH]{};
};

bool LoadConfig(AppConfig& cfg);
bool SaveConfig(const AppConfig& cfg);


