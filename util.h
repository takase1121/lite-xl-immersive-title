#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <windows.h>

#define LEN(str) sizeof(str) / sizeof(*str)

static void __die(char *msg, DWORD rc) {
  int system = msg == NULL;
  
  if (system) {
    FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM
      | FORMAT_MESSAGE_ALLOCATE_BUFFER
      | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      rc,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR) &msg,
      0,
      NULL
    );
  }
  
  if (msg != NULL) {
    DWORD write = 0;
    fprintf(stderr, "error: %s\n", msg);
    if (system) LocalFree(msg);
  }
  
  CloseHandle(stderr);
  ExitProcess(1);
}

#define die(msg) __die(msg, S_OK)
#define die_hr(hr) __die(NULL, HRESULT_CODE(hr))
#define die_rc(rc) __die(NULL, rc)
#endif
