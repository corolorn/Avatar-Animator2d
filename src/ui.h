#pragma once
#include <windows.h>
#include <commctrl.h>

struct UiControls {
	HWND btnIdle = nullptr;
	HWND btnTalk = nullptr;
	HWND comboMic = nullptr;
	HWND sliderTh = nullptr;
	HWND btnColor = nullptr;
	HWND chkTop = nullptr;
	HWND sliderDelay = nullptr;
	HWND sliderFade = nullptr;
	HWND btnCanvas = nullptr;
};

inline void CreateBasicControls(HWND parent, UiControls& ui) {
	HWND h = parent;
	ui.btnIdle = CreateWindowEx(0, L"BUTTON", L"Idle…", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		8, 8, 100, 28, h, (HMENU)1001, nullptr, nullptr);
	ui.btnTalk = CreateWindowEx(0, L"BUTTON", L"Talking…", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		116, 8, 100, 28, h, (HMENU)1002, nullptr, nullptr);
	ui.comboMic = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
		228, 8, 260, 300, h, (HMENU)1003, nullptr, nullptr);
	ui.sliderTh = CreateWindowEx(0, L"msctls_trackbar32", L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		8, 44, 300, 30, h, (HMENU)1004, nullptr, nullptr);
	SendMessage(ui.sliderTh, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
	SendMessage(ui.sliderTh, TBM_SETPOS, TRUE, 15);
	ui.btnColor = CreateWindowEx(0, L"BUTTON", L"Chroma…", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		320, 44, 80, 28, h, (HMENU)1005, nullptr, nullptr);
	ui.chkTop = CreateWindowEx(0, L"BUTTON", L"Topmost", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		410, 44, 80, 24, h, (HMENU)1006, nullptr, nullptr);
	ui.sliderDelay = CreateWindowEx(0, L"msctls_trackbar32", L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		8, 78, 300, 30, h, (HMENU)1007, nullptr, nullptr);
	SendMessage(ui.sliderDelay, TBM_SETRANGE, TRUE, MAKELPARAM(0, 500)); // ms
	SendMessage(ui.sliderDelay, TBM_SETPOS, TRUE, 50);
	ui.sliderFade = CreateWindowEx(0, L"msctls_trackbar32", L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		8, 112, 300, 30, h, (HMENU)1008, nullptr, nullptr);
	SendMessage(ui.sliderFade, TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000)); // ms
	SendMessage(ui.sliderFade, TBM_SETPOS, TRUE, 150);
	ui.btnCanvas = CreateWindowEx(0, L"BUTTON", L"Open Canvas", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		320, 78, 170, 28, h, (HMENU)1009, nullptr, nullptr);

	// Set a modern UI font (Segoe UI 9pt)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf{}; GetObject(hFont, sizeof(lf), &lf); wcscpy_s(lf.lfFaceName, L"Segoe UI"); lf.lfHeight = -12; // ~9pt at 96 DPI
	HFONT hSegoe = CreateFontIndirect(&lf);
	SendMessage(ui.btnIdle, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.btnTalk, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.comboMic, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.sliderTh, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.btnColor, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.chkTop, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.sliderDelay, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.sliderFade, WM_SETFONT, (WPARAM)hSegoe, TRUE);
	SendMessage(ui.btnCanvas, WM_SETFONT, (WPARAM)hSegoe, TRUE);
}


