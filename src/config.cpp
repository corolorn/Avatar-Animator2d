#include "config.h"
#include <shlwapi.h>
#include <wchar.h>

static void GetIniPath(wchar_t* outPath, DWORD size) {
	GetModuleFileName(nullptr, outPath, size);
	PathRemoveExtension(outPath);
	PathAddExtension(outPath, L".ini");
}

bool LoadConfig(AppConfig& cfg) {
	wchar_t ini[MAX_PATH]{}; GetIniPath(ini, MAX_PATH);
	cfg.chromaKey = (COLORREF)GetPrivateProfileInt(L"app", L"chroma", (int)cfg.chromaKey, ini);
	wchar_t buf[64]{};
	GetPrivateProfileString(L"app", L"thOn", L"0.15", buf, 64, ini); cfg.thOn = (float)_wtof(buf);
	GetPrivateProfileString(L"app", L"thOff", L"0.10", buf, 64, ini); cfg.thOff = (float)_wtof(buf);
	GetPrivateProfileString(L"app", L"idle", L"", cfg.idlePath, MAX_PATH, ini);
	GetPrivateProfileString(L"app", L"talk", L"", cfg.talkPath, MAX_PATH, ini);
	return true;
}

bool SaveConfig(const AppConfig& cfg) {
	wchar_t ini[MAX_PATH]{}; GetIniPath(ini, MAX_PATH);
	wchar_t v[64]{};
	wsprintf(v, L"%u", (unsigned)cfg.chromaKey);
	WritePrivateProfileString(L"app", L"chroma", v, ini);
    swprintf_s(v, 64, L"%.3f", cfg.thOn); WritePrivateProfileString(L"app", L"thOn", v, ini);
    swprintf_s(v, 64, L"%.3f", cfg.thOff); WritePrivateProfileString(L"app", L"thOff", v, ini);
	WritePrivateProfileString(L"app", L"idle", cfg.idlePath, ini);
	WritePrivateProfileString(L"app", L"talk", cfg.talkPath, ini);
	return true;
}


