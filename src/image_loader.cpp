#include "image_loader.h"
#include <wincodec.h>
#include <vector>

struct WicReleaser { template<typename T> void operator()(T* p) const { if (p) p->Release(); } };

static bool CreateHBITMAPFromWIC(IWICBitmapSource* src, HBITMAP* outBmp, int* outW, int* outH) {
	if (!src || !outBmp) return false;
	UINT w = 0, h = 0;
	if (FAILED(src->GetSize(&w, &h))) return false;

	// Convert to 32bpp pre-multiplied BGRA for GDI compatibility if needed
	IWICImagingFactory* factory = nullptr;
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory)))) return false;

	IWICFormatConverter* converter = nullptr;
	HRESULT hr = factory->CreateFormatConverter(&converter);
	if (SUCCEEDED(hr)) {
		hr = converter->Initialize(src, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
			nullptr, 0.0, WICBitmapPaletteTypeCustom);
	}
	if (FAILED(hr)) { if (converter) converter->Release(); factory->Release(); return false; }

	const int stride = (int)w * 4;
	std::vector<BYTE> pixels(stride * h);
	hr = converter->CopyPixels(nullptr, stride, (UINT)pixels.size(), pixels.data());
	converter->Release();
	if (FAILED(hr)) { factory->Release(); return false; }

	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (LONG)w;
	bmi.bmiHeader.biHeight = -(LONG)h; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* dibBits = nullptr;
	HDC screen = GetDC(nullptr);
	HBITMAP bmp = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
	ReleaseDC(nullptr, screen);
	if (!bmp || !dibBits) { if (bmp) DeleteObject(bmp); factory->Release(); return false; }

	memcpy(dibBits, pixels.data(), pixels.size());
	*outBmp = bmp;
	if (outW) *outW = (int)w;
	if (outH) *outH = (int)h;
	factory->Release();
	return true;
}

bool LoadImageFromFile(const wchar_t* path, HBITMAP* outBitmap, int* outW, int* outH) {
	if (!path || !outBitmap) return false;
	IWICImagingFactory* factory = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory));
	if (FAILED(hr)) return false;

	IWICBitmapDecoder* decoder = nullptr;
	hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
		WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr)) { factory->Release(); return false; }

	IWICBitmapFrameDecode* frame = nullptr;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) { decoder->Release(); factory->Release(); return false; }

	HBITMAP bmp = nullptr; int w = 0, h = 0;
	bool ok = CreateHBITMAPFromWIC(frame, &bmp, &w, &h);
	frame->Release();
	decoder->Release();
	factory->Release();
	if (!ok) return false;
	*outBitmap = bmp; if (outW) *outW = w; if (outH) *outH = h; return true;
}


