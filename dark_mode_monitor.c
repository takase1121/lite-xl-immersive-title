#include <stdio.h>
#include <windows.h>
#include "util.h"

#define MICA 1029
#define DARK_MODE_PRE_20H1 19
#define DARK_MODE 20

typedef HRESULT( * PFN_DWM_SET_WINDOW_ATTRIBUTE)(HWND, DWORD, LPCVOID, DWORD);
static PFN_DWM_SET_WINDOW_ATTRIBUTE DwmSetWindowAttribute = NULL;

static int mica = 0;
static const char * target = NULL;
static HWND target_win = NULL;

static void load_dwm() {
  HMODULE dwm = LoadLibrary("dwmapi.dll");
  if (dwm == NULL)
    die_rc(GetLastError());
  DwmSetWindowAttribute = (PFN_DWM_SET_WINDOW_ATTRIBUTE) GetProcAddress(dwm, "DwmSetWindowAttribute");
  if (!DwmSetWindowAttribute)
    die_rc(GetLastError());
}

static DWORD set_theme(HKEY key) {
  DWORD type, value = 0, size = 4;
  LSTATUS rc = RegQueryValueEx(
    key,
    TEXT("AppsUseLightTheme"),
    NULL, &
    type,
    (LPBYTE) & value, &
    size
  );
  
  if (rc == ERROR_SUCCESS && type == REG_DWORD) {
    value = !value;
    HRESULT res = DwmSetWindowAttribute(target_win, DARK_MODE_PRE_20H1, & value, 4);
    if (FAILED(res))
      res = DwmSetWindowAttribute(target_win, DARK_MODE, & value, 4);
    if (mica)
      DwmSetWindowAttribute(target_win, MICA, &value, 4);
    
    return HRESULT_CODE(res);
  }
  
  return HRESULT_CODE(E_FAIL);
}

BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lp) {
  int size = GetWindowTextLength(hwnd) + 1;
  char * name = malloc(size * sizeof(char));
  if (name == NULL)
    return FALSE;

  if (GetWindowText(hwnd, name, size) != 0) {
    if (strcmp(name, target) == 0) {
      target_win = hwnd;
      return FALSE;
    }
  }
  free(name);
  return TRUE;
}

int main(int argc, char ** argv) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  if (argc < 3)
    die("invalid argument");

  load_dwm();

  if (lstrcmpA(argv[1], "true") == 0)
    mica = 1;

  // try to get hwnd
  target = argv[2];
  int tries = 0;
  while (target_win == NULL && tries < 3) {
    EnumWindows(EnumWndProc, 0);
    tries++;
  }

  if (target_win == NULL)
    die("cannot find the window");
    
  puts("found!");

  HKEY key;
  DWORD rc = ERROR_SUCCESS;
  DWORD type, value = 0, size = 4;
  rc = RegOpenKeyEx(
    HKEY_CURRENT_USER,
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
    0,
    KEY_READ | KEY_NOTIFY, &
    key
  );
  if (rc < 0)
    die_rc(rc);

  rc = set_theme(key);
  if (rc < 0)
    die_rc(rc);

  while (rc >= 0) {
    rc = RegNotifyChangeKeyValue(
      key,
      TRUE,
      REG_NOTIFY_CHANGE_LAST_SET,
      NULL,
      FALSE
    );
    
    if (rc >= 0)
      rc = set_theme(key);
  }

  RegCloseKey(key);

  if (rc < 0)
    die_rc(rc);

  return 0;
}
