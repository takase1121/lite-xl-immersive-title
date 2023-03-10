#include <new>
#include <cstdint>
#include <windows.h>
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>
#include <windows.ui.viewmanagement.h>

#include "winrt_ui_settings.h"


namespace wrl = Microsoft::WRL;
namespace abi_vm = ABI::Windows::UI::ViewManagement;
namespace foundation = Windows::Foundation;
using namespace Windows;


struct winrt_ui_settings_s {
	wrl::ComPtr<abi_vm::IUISettings3> ptr;
};


HRESULT winrt_initialize() {
	return RoInitialize(RO_INIT_MULTITHREADED);
}


void winrt_deinitialize() {
	RoUninitialize();
}


HRESULT winrt_ui_settings_new(winrt_ui_settings_t **settings) {
	HRESULT hr;
	wrl::ComPtr<abi_vm::IUISettings3> ptr;
	hr = foundation::ActivateInstance(
		wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(),
		&ptr
	);
	if (FAILED(hr))
		return hr;

	try {
		*settings = new winrt_ui_settings_t;
		(*settings)->ptr = ptr.Get();
	} catch (std::bad_alloc&) {
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	return S_OK;
}


void winrt_ui_settings_free(winrt_ui_settings_t *settings) {
	if (settings)
		delete settings;
}


HRESULT winrt_ui_settings_get_color(winrt_ui_settings_t *settings, int type, uint32_t *color) {
	HRESULT hr;
	ABI::Windows::UI::Color c;
	hr = settings->ptr->GetColorValue(
		static_cast<enum ABI::Windows::UI::ViewManagement::UIColorType>(type),
		&c
	);
	if (FAILED(hr))
		return hr;

	*color = (c.R << 24) | (c.G << 16) | (c.B << 8) | c.A;
	return S_OK;
}
