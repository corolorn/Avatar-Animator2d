#include <windows.h>
#include <commdlg.h>
#include <wrl.h>
#include "image_loader.h"
#include "render.h"
#include "audio_capture.h"
#include "ui.h"
#include "config.h"
#include <commctrl.h>

struct AppState {
	COLORREF chromaKey = RGB(0, 255, 0);
	BitmapWithSize idle{};
	BitmapWithSize talk{};
	bool showTalk = false; // will be driven by audio threshold
	WasapiCapture audio{};
	float thresholdOn = 0.15f;  // default activation
	float thresholdOff = 0.10f; // hysteresis
	// timing and fade
	DWORD switchDelayMs = 50; // debounce delay before switching
	DWORD fadeMs = 150;       // crossfade duration
	DWORD stateChangeAtMs = 0;
	float fadeT = 0.0f;       // 0..1 crossfade progress
};

static AppState g_state;
static UiControls g_ui;
static AppConfig g_cfg;
static HWND g_canvasWnd = nullptr;

static LRESULT CALLBACK CanvasProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		SetTimer(hWnd, 1, 16, nullptr);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT: {
		PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc; GetClientRect(hWnd, &rc);
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HGDIOBJ old = SelectObject(memDC, memBmp);
		FillChroma(memDC, g_state.chromaKey, rc);
		if (g_state.fadeMs > 0 && g_state.fadeT < 1.0f) {
			if (g_state.showTalk) DrawCrossfade(memDC, rc, g_state.idle, g_state.talk, g_state.fadeT);
			else DrawCrossfade(memDC, rc, g_state.talk, g_state.idle, g_state.fadeT);
		} else {
			const BitmapWithSize& img = g_state.showTalk && g_state.talk.bmp ? g_state.talk : g_state.idle;
			DrawCenteredBitmap(memDC, rc, img);
		}
		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, old); DeleteObject(memBmp); DeleteDC(memDC);
		EndPaint(hWnd, &ps); return 0;
	}
	case WM_TIMER:
		InvalidateRect(hWnd, nullptr, FALSE); return 0;
	case WM_DESTROY:
		KillTimer(hWnd, 1); if (g_canvasWnd == hWnd) g_canvasWnd = nullptr; return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		SetTimer(hWnd, 1, 16, nullptr); // ~60 FPS updates
		g_state.audio.initializeDefaultCapture();
		g_state.audio.start();
		CreateBasicControls(hWnd, g_ui);
		WasapiCapture::EnumerateCaptureNames(g_ui.comboMic);
		return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc; GetClientRect(hWnd, &rc);
		// Backbuffer
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HGDIOBJ old = SelectObject(memDC, memBmp);
		FillChroma(memDC, g_state.chromaKey, rc);
		if (g_state.fadeMs > 0 && g_state.fadeT < 1.0f) {
			if (g_state.showTalk) DrawCrossfade(memDC, rc, g_state.idle, g_state.talk, g_state.fadeT);
			else DrawCrossfade(memDC, rc, g_state.talk, g_state.idle, g_state.fadeT);
		} else {
			const BitmapWithSize& img = g_state.showTalk && g_state.talk.bmp ? g_state.talk : g_state.idle;
			DrawCenteredBitmap(memDC, rc, img);
		}
		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, old);
		DeleteObject(memBmp);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1; // prevent default background erase to reduce flicker
	case WM_TIMER: {
		if (wParam == 1) {
			AudioLevel lvl = g_state.audio.pollLevel();
			bool talking = g_state.showTalk;
			if (!talking && lvl.peak >= g_state.thresholdOn) talking = true;
			else if (talking && lvl.peak <= g_state.thresholdOff) talking = false;
			DWORD now = GetTickCount();
			if (talking != g_state.showTalk) {
				if (g_state.stateChangeAtMs == 0) g_state.stateChangeAtMs = now; // start debounce
				if (now - g_state.stateChangeAtMs >= g_state.switchDelayMs) {
					g_state.showTalk = talking; g_state.stateChangeAtMs = 0; g_state.fadeT = 0.0f;
				}
			} else {
				g_state.stateChangeAtMs = 0;
			}
			// advance fade
			if (g_state.fadeMs > 0 && g_state.fadeT < 1.0f) {
				g_state.fadeT += 16.0f / (float)g_state.fadeMs; if (g_state.fadeT > 1.0f) g_state.fadeT = 1.0f;
			}
			InvalidateRect(hWnd, nullptr, FALSE);
		}
		return 0;
	}
	case WM_SIZE:
		InvalidateRect(hWnd, nullptr, TRUE);
		return 0;
	case WM_COMMAND: {
		const int id = LOWORD(wParam);
		if (id == 1001 || id == 1002) {
			wchar_t path[MAX_PATH]{};
			OPENFILENAME ofn{}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hWnd; ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.jpeg;*.bmp\0\0"; ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileName(&ofn)) {
				HBITMAP bmp{}; int w=0,h=0;
				if (LoadImageFromFile(path, &bmp, &w, &h)) {
					BitmapWithSize b{ bmp, w, h };
					if (id == 1001) { if (g_state.idle.bmp) DeleteObject(g_state.idle.bmp); g_state.idle = b; wcscpy_s(g_cfg.idlePath, path); }
					else { if (g_state.talk.bmp) DeleteObject(g_state.talk.bmp); g_state.talk = b; wcscpy_s(g_cfg.talkPath, path); }
					InvalidateRect(hWnd, nullptr, TRUE);
				}
			}
		}
		if (id == 1005) {
			CHOOSECOLOR cc{}; COLORREF cust[16]{}; cc.lStructSize = sizeof(cc); cc.hwndOwner = hWnd; cc.lpCustColors = cust; cc.rgbResult = g_state.chromaKey; cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc)) { g_state.chromaKey = cc.rgbResult; InvalidateRect(hWnd, nullptr, TRUE); }
		}
		if (id == 1006) {
			BOOL checked = (SendMessage(g_ui.chkTop, BM_GETCHECK, 0, 0) == BST_CHECKED);
			SetWindowPos(hWnd, checked ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
		}
		if (id == 1009) {
			if (!g_canvasWnd) {
				DWORD s = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN; DWORD ex = WS_EX_APPWINDOW;
				g_canvasWnd = CreateWindowEx(ex, L"GavGifCanvasWnd", L"Canvas", s, CW_USEDEFAULT, CW_USEDEFAULT, 900, 600, nullptr, nullptr, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), nullptr);
				ShowWindow(g_canvasWnd, SW_SHOW);
			} else {
				ShowWindow(g_canvasWnd, SW_SHOW);
				SetForegroundWindow(g_canvasWnd);
			}
		}
		if (id == 1003 && HIWORD(wParam) == CBN_SELCHANGE) {
			int sel = (int)SendMessage(g_ui.comboMic, CB_GETCURSEL, 0, 0);
			if (sel >= 0) {
				LPWSTR idStr = (LPWSTR)SendMessage(g_ui.comboMic, CB_GETITEMDATA, sel, 0);
				if (idStr) { g_state.audio.stop(); g_state.audio.initializeById(idStr); g_state.audio.start(); }
			}
		}
		return 0;
	}
	case WM_HSCROLL: {
		if ((HWND)lParam == g_ui.sliderTh) {
			int v = (int)SendMessage(g_ui.sliderTh, TBM_GETPOS, 0, 0);
			g_state.thresholdOn = v / 100.0f;
			g_state.thresholdOff = g_state.thresholdOn * 0.66f;
		}
		if ((HWND)lParam == g_ui.sliderDelay) {
			int v = (int)SendMessage(g_ui.sliderDelay, TBM_GETPOS, 0, 0);
			g_state.switchDelayMs = (DWORD)v;
		}
		if ((HWND)lParam == g_ui.sliderFade) {
			int v = (int)SendMessage(g_ui.sliderFade, TBM_GETPOS, 0, 0);
			g_state.fadeMs = (DWORD)v;
		}
		return 0;
	}
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		g_state.audio.stop();
		if (g_state.idle.bmp) { DeleteObject(g_state.idle.bmp); g_state.idle.bmp = nullptr; }
		if (g_state.talk.bmp) { DeleteObject(g_state.talk.bmp); g_state.talk.bmp = nullptr; }
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
	// Initialize common controls and COM
	INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_BAR_CLASSES };
	InitCommonControlsEx(&icc);
	// Initialize COM for WIC and WASAPI
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) return 1;
	const wchar_t* kClass = L"GavGifAvatarAnimatorWnd";
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = kClass;
	RegisterClassEx(&wc);

	// Canvas window class (no controls)
	WNDCLASSEX wcc{}; wcc.cbSize = sizeof(wcc); wcc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcc.lpfnWndProc = CanvasProc; wcc.hInstance = hInstance; wcc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wcc.lpszClassName = L"GavGifCanvasWnd";
	RegisterClassEx(&wcc);

	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD exStyle = WS_EX_APPWINDOW;
	HWND hWnd = CreateWindowEx(
		exStyle,
		kClass,
		L"Avatar Animator",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
		nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) return 1;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Load config and images
	LoadConfig(g_cfg);
	g_state.chromaKey = g_cfg.chromaKey;
	g_state.thresholdOn = g_cfg.thOn; g_state.thresholdOff = g_cfg.thOff;
	if (g_cfg.idlePath[0]) { HBITMAP b; int w,h; if (LoadImageFromFile(g_cfg.idlePath, &b, &w, &h)) g_state.idle = { b,w,h }; }
	if (g_cfg.talkPath[0]) { HBITMAP b; int w,h; if (LoadImageFromFile(g_cfg.talkPath, &b, &w, &h)) g_state.talk = { b,w,h }; }
	if (g_ui.sliderTh) SendMessage(g_ui.sliderTh, TBM_SETPOS, TRUE, (LPARAM)(int)(g_state.thresholdOn * 100.0f));

	// Demo: load placeholder via file dialogs is not yet wired; remain chroma background only

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// Save config
	g_cfg.chromaKey = g_state.chromaKey; g_cfg.thOn = g_state.thresholdOn; g_cfg.thOff = g_state.thresholdOff;
	SaveConfig(g_cfg);
	CoUninitialize();
	return (int)msg.wParam;
}


