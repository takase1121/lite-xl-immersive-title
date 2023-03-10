#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <windows.h>
#include <dwmapi.h>


#ifdef ENABLE_ACCENT_REPORT
#include "winrt_ui_settings.h"
#endif


// definitions for DwmSetWindowAttribute
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_MICA
#define DWMWA_USE_MICA 1029
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif


uintptr_t _beginthreadex( // NATIVE CODE
   void *security,
   unsigned stack_size,
   unsigned ( __stdcall *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr
);


#define MAX_CLASS_SIZE 512
#define BUFFER_SIZE 512
#define CMD_CONFIG "config"
#define CMD_THEME "theme"
#define CMD_EXIT "exit"
#define CMD_ACCENT "accent"

#define WIN10_BUILD_NUMBER 18362
#define WIN11_BUILD_NUMBER 22000
#define WIN11_SYSTEMBACKDROP_SUPPORTED_BUILD_NUMBER 22621


typedef enum {
    BACKDROP_DEFAULT,
    BACKDROP_NONE,
    BACKDROP_MICA,
    BACKDROP_ACRYLIC,
    BACKDROP_TABBED,
    BACKDROP_MAX,
} window_backdrop_e;

typedef enum {
    CONFIG_DARK_MODE = 1,
    CONFIG_EXTEND_BORDER = 2,
    CONFIG_BACKDROP_TYPE = 4
} config_changed_e;

typedef struct window_config_s {
    DWORD pid;
    HWND window;
    HKEY regkey;
#ifdef ENABLE_ACCENT_REPORT
    winrt_ui_settings_t *ui_settings;
#endif
    int dark_mode, extend_border;
    window_backdrop_e backdrop_type;
    config_changed_e mask;
    CONDITION_VARIABLE config_changed;
    CRITICAL_SECTION mutex;
    char class[MAX_CLASS_SIZE];
} window_config_t;


#define log_ready() (printf("ready\n"))
#define log_response(cmd, fmt, ...) (printf(cmd " " fmt "\n", __VA_ARGS__))
#define log_theme(fmt, ...) (printf("themechange " fmt "\n", __VA_ARGS__))
#define log_error(fmt, ...) (printf("error " fmt "\n", __VA_ARGS__))


static void log_win32_error(const char *function_name, DWORD rc) {
    LPSTR msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    rc,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR) &msg,
                    0,
                    NULL);
    log_error("%s: %s", function_name, msg ? msg : "unknown error");
    LocalFree(msg);
}


static LSTATUS is_dark_mode(HKEY regkey, int *is_dark) {
    LSTATUS rc;
    HKEY key = NULL;
    DWORD type, value, size = 4;

    rc = RegQueryValueEx(regkey,
                            "AppsUseLightTheme",
                            NULL,
                            &type,
                            (LPBYTE)&value, &size);

    if (rc != ERROR_SUCCESS)
        return rc;
    if (type != REG_DWORD)
        return ERROR_INVALID_PARAMETER;

    *is_dark = !value;
    return ERROR_SUCCESS;
}


static unsigned __stdcall theme_monitor_proc(void *ud) {
    int value;
    HANDLE ev;
    LRESULT rc;
    window_config_t *config = (window_config_t *) ud;

    ev = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!ev) {
        log_win32_error("CreateEventA", GetLastError());
        return 0;
    }

    for (;;) {
        // we must call LeaveCriticalSection when we decided to break
        // Get an event so that we can unlock the mutex and wait
        EnterCriticalSection(&config->mutex);

        if (!config->regkey) {
            LeaveCriticalSection(&config->mutex);
            break;
        }

        rc = RegNotifyChangeKeyValue(config->regkey,
                                        FALSE,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        ev,
                                        TRUE);
        if (rc != ERROR_SUCCESS) {
            log_win32_error("RegNotifyChangeKeyValue", rc);
            LeaveCriticalSection(&config->mutex);
            break;
        }

        LeaveCriticalSection(&config->mutex);

        // wait for the mutex
        WaitForSingleObject(ev, INFINITE);

        // read the config
        EnterCriticalSection(&config->mutex);

        if (!config->regkey) {
            LeaveCriticalSection(&config->mutex);
            break;
        }

        rc = is_dark_mode(config->regkey, &value);
        if (rc != ERROR_SUCCESS) {
            log_win32_error("RegQueryValueEx", rc);
            LeaveCriticalSection(&config->mutex);
            break;
        }

        if (value != config->dark_mode) {
            config->dark_mode = value;
            config->mask |= CONFIG_DARK_MODE;
            log_theme("%d", config->dark_mode);
            WakeConditionVariable(&config->config_changed);
        }

        LeaveCriticalSection(&config->mutex);
    }
    return 0;
}


static unsigned __stdcall config_change_proc(void *ud) {
    HRESULT hr;
    OSVERSIONINFOEXA version;
    window_config_t *config = (window_config_t *) ud;

    // get the OS version so we know how to set the correct attribute later
    version.dwOSVersionInfoSize = sizeof(version);
    if (!GetVersionExA((LPOSVERSIONINFOA) &version)) {
        log_win32_error("GetVersionExA", GetLastError());
        return 0;
    }

    if (version.dwBuildNumber < WIN10_BUILD_NUMBER) {
        log_error("windows build unsupported: %ld", version.dwBuildNumber);
        return 0;
    }

    for (;;) {
        MARGINS m = { 0 };
        DWORD value;

        // once again, we must call LeaveCriticalSection when breaking!
        EnterCriticalSection(&config->mutex);

        while (config->window && !config->mask)
            SleepConditionVariableCS(&config->config_changed, &config->mutex, INFINITE);

        if (!config->window) {
            LeaveCriticalSection(&config->mutex);
            break;
        }

        // extend the frame
        if (config->mask & CONFIG_EXTEND_BORDER) {
            if (config->extend_border)
                m.cxLeftWidth = m.cxRightWidth = m.cyBottomHeight = m.cyTopHeight = -1;
            hr = DwmExtendFrameIntoClientArea(config->window, &m);
            if (FAILED(hr)) {
                log_win32_error("DwmExtendFrameIntoClientArea", HRESULT_CODE(hr));
                LeaveCriticalSection(&config->mutex);
                break;
            }
        }

        // set window light/dark theme
        if (config->mask & CONFIG_DARK_MODE) {
            value = config->dark_mode;
            hr = DwmSetWindowAttribute(config->window,
                                        DWMWA_USE_IMMERSIVE_DARK_MODE,
                                        &value,
                                        sizeof(DWORD));
            if (FAILED(hr)) {
                log_win32_error("DwmSetWindowAttribute(DWMMA_USE_IMMERSIVE_DARK_MODE)", HRESULT_CODE(hr));
                LeaveCriticalSection(&config->mutex);
                break;
            }
        }

        // set window backdrop
        if (config->mask & CONFIG_BACKDROP_TYPE) {
            if (version.dwBuildNumber >= WIN11_SYSTEMBACKDROP_SUPPORTED_BUILD_NUMBER) {
                value = config->backdrop_type;
                hr = DwmSetWindowAttribute(config->window,
                                            DWMWA_SYSTEMBACKDROP_TYPE,
                                            &value,
                                            sizeof(DWORD));
                if (FAILED(hr)) {
                    log_win32_error("DwmSetWindowAttribute(DWMWA_SYSTEMBACKDROP_TYPE)", HRESULT_CODE(hr));
                    LeaveCriticalSection(&config->mutex);
                    break;
                }
            } else if (version.dwBuildNumber < WIN11_SYSTEMBACKDROP_SUPPORTED_BUILD_NUMBER
                        && config->backdrop_type == BACKDROP_MICA) {
                // on older versions we should use another method that only supports mica
                value = 1;
                hr = DwmSetWindowAttribute(config->window,
                                            DWMWA_USE_MICA,
                                            &value,
                                            sizeof(DWORD));
                if (FAILED(hr)) {
                    log_win32_error("DwmSetWindowAttribute(DWMWA_USE_MICA)", HRESULT_CODE(hr));
                    LeaveCriticalSection(&config->mutex);
                    break;
                }
            } else {
                log_error("unsupported windows version for backdrop type %d", config->backdrop_type);
            }
        }

        // clear config mask
        config->mask = 0;
        LeaveCriticalSection(&config->mutex);
    }
    return 0;
}


static unsigned __stdcall read_input_proc(void *ud) {
    char buffer[BUFFER_SIZE];
    window_config_t *config = (window_config_t *) ud;

    // FIXME: better input parsing
    while (fgets(buffer, sizeof(buffer), stdin)) {
        // check if window is valid before we continue processing
        EnterCriticalSection(&config->mutex);
        if (!config->window || !IsWindow(config->window)) {
            LeaveCriticalSection(&config->mutex);
            break;
        }
        LeaveCriticalSection(&config->mutex);

        if (strncmp(buffer, CMD_CONFIG, sizeof(CMD_CONFIG) - 1) == 0) {
            int value;

            if (strlen(buffer) != sizeof(CMD_CONFIG) + 3) { // "config <extend><backdrop_type>"
                log_error("invalid command: %s", buffer);
                continue;
            }
            
            // save the config
            EnterCriticalSection(&config->mutex);

            // extend border
            value = buffer[sizeof(CMD_CONFIG)] - '0';
            if (value != config->extend_border) {
                config->extend_border = !!value;
                config->mask |= CONFIG_EXTEND_BORDER;
            }

            // backdrop type
            value = buffer[sizeof(CMD_CONFIG) + 1] - '0';
            if (value >= 0 && value < BACKDROP_MAX && value != config->backdrop_type) {
                config->backdrop_type = value;
                config->mask |= CONFIG_BACKDROP_TYPE;
            }

            WakeConditionVariable(&config->config_changed);
            LeaveCriticalSection(&config->mutex);
        } else if (strncmp(buffer, CMD_THEME, sizeof(CMD_THEME) - 1) == 0) {
            int value;
            LRESULT rc;
            EnterCriticalSection(&config->mutex);
            rc = is_dark_mode(config->regkey, &value);
            if (rc != ERROR_SUCCESS) {
                log_win32_error("is_dark_mode", rc);
                LeaveCriticalSection(&config->mutex);
                break;
            }
            log_response(CMD_THEME, "%d", value);
            LeaveCriticalSection(&config->mutex);
#ifdef ENABLE_ACCENT_REPORT
        } else if (strncmp(buffer, CMD_ACCENT, sizeof(CMD_ACCENT) - 1) == 0) {
            int type;
            HRESULT hr;
            uint32_t value = 0;

            if (strlen(buffer) != sizeof(CMD_ACCENT) + 2) {
                log_error("invalid command: %s", buffer);
                continue;
            }

            type = buffer[sizeof(CMD_ACCENT)] - '0';
            if (type > COLOR_TYPE_MIN && type < COLOR_TYPE_MAX) {
                EnterCriticalSection(&config->mutex);
                hr = winrt_ui_settings_get_color(config->ui_settings, type, &value);
                if (FAILED(hr)) {
                    log_win32_error("winrt_ui_settings_get_color", HRESULT_CODE(hr));
                    LeaveCriticalSection(&config->mutex);
                    break;
                }
                log_response(CMD_ACCENT, "%d %u", type, value);
                LeaveCriticalSection(&config->mutex);
            } else {
                log_error("invalid parameter: %c", buffer[sizeof(CMD_ACCENT)]);
            }
#endif
        } else if (strncmp(buffer, CMD_EXIT, sizeof(CMD_EXIT) - 1) == 0) {
            break;
        } else {
           log_error("invalid command: %s", buffer);
        }
    }
    return 0;
}


BOOL CALLBACK enum_window_proc(HWND hwnd, LPARAM lparam) {  
    DWORD pid;
    char buffer[MAX_CLASS_SIZE];
    window_config_t *target = (window_config_t *) lparam;
    if (!GetWindowThreadProcessId(hwnd, &pid)
        || !GetClassNameA(hwnd, buffer, MAX_CLASS_SIZE))
        return FALSE;
    if (pid == target->pid && strcmp(buffer, target->class) == 0) {
        target->window = hwnd;
        return FALSE;
    }
    return TRUE;
}


int main(int argc, char **argv) {
    DWORD rc;
    window_config_t config = { 0 };
    HANDLE thread_handles[3] = { INVALID_HANDLE_VALUE };

#ifdef ENABLE_ACCENT_REPORT
    HRESULT hr;
    hr = winrt_initialize();
    if (FAILED(hr)) {
        log_win32_error("winrt_initialize", HRESULT_CODE(hr));
        return 0;
    }
#endif

    if (argc != 3) {
        log_error("invalid number of arguments: %d", argc);
        return 0;
    }

    // find the current window
    config.pid = strtol(argv[1], NULL, 10);
    snprintf(config.class, MAX_CLASS_SIZE, "%s", argv[2]);
    InitializeCriticalSection(&config.mutex);
    InitializeConditionVariable(&config.config_changed);
#ifdef ENABLE_ACCENT_REPORT
    hr = winrt_ui_settings_new(&config.ui_settings);
    if (FAILED(hr)) {
        log_win32_error("winrt_ui_settings_new", HRESULT_CODE(hr));
        goto exit;
    }
#endif
    if (!EnumWindows(&enum_window_proc,(LPARAM) &config) && GetLastError() != ERROR_SUCCESS) {
        log_win32_error("EnumWindows", GetLastError());
        goto exit;
    }
    if (!config.window) {
        log_error("cannot find window class %s owned by %ld", config.class, config.pid);
        goto exit;
    }

    rc = RegOpenKeyExA(HKEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        0,
                        KEY_READ | KEY_NOTIFY,
                        &config.regkey);
    if (rc != ERROR_SUCCESS) {
        log_win32_error("RegOpenKeyExA", rc);
        goto exit;
    }

    rc = is_dark_mode(config.regkey, &config.dark_mode);
    if (rc != ERROR_SUCCESS) {
        log_win32_error("RegQueryValueExA", rc);
        goto exit;
    }
    config.mask |= CONFIG_DARK_MODE;
    WakeConditionVariable(&config.config_changed);

    thread_handles[0] = (HANDLE) _beginthreadex(NULL, 0, &theme_monitor_proc, &config, 0, NULL);
    thread_handles[1] = (HANDLE) _beginthreadex(NULL, 0, &config_change_proc, &config, 0, NULL);
    thread_handles[2] = (HANDLE) _beginthreadex(NULL, 0, &read_input_proc, &config, 0, NULL);

    for (int i = 0;i < sizeof(thread_handles) / sizeof(*thread_handles); i++) {
        if (thread_handles[i] == INVALID_HANDLE_VALUE) {
            log_error("cannot create threads: %s", strerror(errno));
            goto exit;
        }
    }

    log_ready();

    rc = WaitForMultipleObjects(sizeof(thread_handles) / sizeof(*thread_handles),
                                thread_handles,
                                FALSE,
                                INFINITE);
    if (rc >= WAIT_OBJECT_0 && rc <= WAIT_OBJECT_0 + 2) {
        // close the regkey if any of the threads failed
        EnterCriticalSection(&config.mutex);
        if (config.regkey)
            RegCloseKey(config.regkey);
        config.regkey = NULL;
        config.window = NULL;
        WakeConditionVariable(&config.config_changed);
        LeaveCriticalSection(&config.mutex);

        fclose(stdin);

        // wait for the rest of the threads to quit
        WaitForMultipleObjects(sizeof(thread_handles) / sizeof(*thread_handles),
                                thread_handles,
                                TRUE,
                                INFINITE);
    } else {
        log_win32_error("WaitForMultipleObjects", GetLastError());
        goto exit;
    }

exit:
    for (int i = 0; i < sizeof(thread_handles) / sizeof(*thread_handles); i++) {
        if (thread_handles[i] != INVALID_HANDLE_VALUE)
            CloseHandle(thread_handles[i]);
    }
    if (config.regkey)
        RegCloseKey(config.regkey);
    DeleteCriticalSection(&config.mutex);
#ifdef ENABLE_ACCENT_REPORT
    if (config.ui_settings)
        winrt_ui_settings_free(config.ui_settings);
    winrt_deinitialize();
#endif
    return 0;
}