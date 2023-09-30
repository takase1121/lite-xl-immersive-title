#ifndef _WINRT_UISETTINGS_H
#define _WINRT_UISETTINGS_H

#include <stdint.h>
#include <windows.h>

typedef enum {
	COLOR_TYPE_MIN = -1,
	COLOR_TYPE_ACCENT = 5,
	COLOR_TYPE_ACCENT_DARK_1 = 4,
	COLOR_TYPE_ACCENT_DARK_2 = 3,
	COLOR_TYPE_ACCENT_DARK_3 = 2,
	COLOR_TYPE_ACCENT_LIGHT_1 = 6,
	COLOR_TYPE_ACCENT_LIGHT_2 = 7,
	COLOR_TYPE_ACCENT_LIGHT_3 = 8,
	COLOR_TYPE_BACKGROUND = 0,
	COLOR_TYPE_FOREGROUND = 1,
	COLOR_TYPE_MAX = 9,
} color_type_e;

typedef struct winrt_ui_settings_s winrt_ui_settings_t;

#ifdef __cplusplus
extern "C" {
#endif

HRESULT winrt_initialize();
HRESULT winrt_ui_settings_new(winrt_ui_settings_t **settings);
HRESULT winrt_ui_settings_get_color(winrt_ui_settings_t *settings, int type, uint32_t *color);
void winrt_deinitialize();
void winrt_ui_settings_free(winrt_ui_settings_t *settings);

#ifdef __cplusplus
}
#endif

#endif