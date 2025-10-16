#pragma once
#include <windows.h>
 
struct BitmapWithSize {
	HBITMAP bmp = nullptr;
	int width = 0;
	int height = 0;
};

inline void FillChroma(HDC hdc, COLORREF color, const RECT& rc) {
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(hdc, &rc, brush);
	DeleteObject(brush);
}

inline void DrawCenteredBitmap(HDC hdc, const RECT& rc, const BitmapWithSize& b) {
	if (!b.bmp) return;
	HDC mem = CreateCompatibleDC(hdc);
	HGDIOBJ old = SelectObject(mem, b.bmp);
	int dstW = rc.right - rc.left;
	int dstH = rc.bottom - rc.top;
	int w = b.width;
	int h = b.height;
	// Fit preserving aspect
	float sx = (float)dstW / (float)w;
	float sy = (float)dstH / (float)h;
	float s = sx < sy ? sx : sy;
	int rw = (int)(w * s);
	int rh = (int)(h * s);
	int x = rc.left + (dstW - rw) / 2;
	int y = rc.top + (dstH - rh) / 2;
	BLENDFUNCTION bf{}; bf.BlendOp = AC_SRC_OVER; bf.SourceConstantAlpha = 255; bf.AlphaFormat = AC_SRC_ALPHA;
	// AlphaBlend ignores HALFTONE but supports per-pixel alpha (32bpp PBGRA)
	AlphaBlend(hdc, x, y, rw, rh, mem, 0, 0, w, h, bf);
	SelectObject(mem, old);
	DeleteDC(mem);
}

inline void DrawCrossfade(HDC hdc, const RECT& rc, const BitmapWithSize& a, const BitmapWithSize& b, float t01) {
	// t01=0 -> only a, t01=1 -> only b
	if (t01 <= 0.0f || !b.bmp) { DrawCenteredBitmap(hdc, rc, a); return; }
	if (t01 >= 1.0f || !a.bmp) { DrawCenteredBitmap(hdc, rc, b); return; }
	// Draw a with alpha (1-t), then b with alpha t
	HDC mem = CreateCompatibleDC(hdc);
	HGDIOBJ old;
	int dstW = rc.right - rc.left;
	int dstH = rc.bottom - rc.top;
	auto drawScaled = [&](const BitmapWithSize& img, BYTE alpha) {
		if (!img.bmp) return;
		old = SelectObject(mem, img.bmp);
		int w = img.width, h = img.height;
		float sx = (float)dstW / (float)w; float sy = (float)dstH / (float)h; float s = sx < sy ? sx : sy;
		int rw = (int)(w * s); int rh = (int)(h * s);
		int x = rc.left + (dstW - rw) / 2; int y = rc.top + (dstH - rh) / 2;
		BLENDFUNCTION bf{}; bf.BlendOp = AC_SRC_OVER; bf.SourceConstantAlpha = alpha; bf.AlphaFormat = AC_SRC_ALPHA;
		AlphaBlend(hdc, x, y, rw, rh, mem, 0, 0, w, h, bf);
		SelectObject(mem, old);
	};
	BYTE aAlpha = (BYTE)(255 * (1.0f - t01));
	BYTE bAlpha = (BYTE)(255 * t01);
	drawScaled(a, aAlpha);
	drawScaled(b, bAlpha);
	DeleteDC(mem);
}


