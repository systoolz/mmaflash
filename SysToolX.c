#include <windows.h>
#include "SysToolX.h"

void *GrowMem(void *lpPtr, DWORD dwSize) {
  if (lpPtr == NULL) {
    return(HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, dwSize));
  } else {
    return(HeapReAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, lpPtr, dwSize));
  }
}

void *GetMem(DWORD dwSize) {
  return(GrowMem(NULL, dwSize));
}

BOOL FreeMem(void *lpPtr) {
  return(HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpPtr));
}

TCHAR *LangLoadString(UINT sid) {
TCHAR *res;
WORD *p;
HRSRC hr;
int i;
  res = NULL;
  hr = FindResource(NULL, MAKEINTRESOURCE(sid / 16 + 1), RT_STRING);
  p = hr ? (WORD *) LockResource(LoadResource(NULL, hr)) : NULL;
  if (p != NULL) {
    for (i = 0; i < (sid & 15); i++) {
      p += 1 + *p;
    }
    res = STR_ALLOC(*p);
    LoadString(NULL, sid, res, *p + 1);
  }
  return(res);
}

/* == COMMAD LINE ROUTINES ================================================= */

/* http://msdn.microsoft.com/en-us/library/17w5ykft.aspx */
#define CMD_PARSE_QUOTE 1
#define CMD_PARSE_COPY  2
void ParseCmdLine(TCHAR *p, TCHAR **argv, TCHAR *lpstr, DWORD *numargs, DWORD *numbytes) {
DWORD flags;
DWORD slashes;
  *numbytes = 0;
  *numargs = 1;
  if (argv) {
    *argv++ = lpstr;
  }
  flags = (*p == TEXT('\"')) ? 1 : 0;
  p += flags;
  while (
    (*p != TEXT('\0')) && (
      (  flags  && (*p != TEXT('\"'))) ||
      ((!flags) && (*p != TEXT('\t')) && (*p != TEXT(' ')))
    )
  ) {
    *numbytes += sizeof(TCHAR);
    if (lpstr) {
      *lpstr++ = *p;
    }
    p++;
  }
  *numbytes += sizeof(TCHAR);
  if (lpstr) {
    *lpstr++ = TEXT('\0');
  }
  flags &= (*p == TEXT('\"')) ? 1 : 0;
  p += flags;
  flags = 0;
  for ( ; ; ) {
    while ((*p == TEXT(' ')) || (*p == TEXT('\t'))) {
      ++p;
    }
    if (*p == TEXT('\0')) {
      break;
    }
    if (argv) {
      *argv++ = lpstr;
    }
    ++*numargs;
    for ( ; ; ) {
      flags |= CMD_PARSE_COPY;
      slashes = 0;
      while (*p == TEXT('\\')) {
        ++p;
        ++slashes;
      }
      if (*p == TEXT('\"')) {
        if ((slashes % 2) == 0) {
          if (flags & CMD_PARSE_QUOTE) {
            if (p[1] == TEXT('\"')) {
              p++;
            } else {
              flags ^= CMD_PARSE_COPY;
            }
          } else {
            flags ^= CMD_PARSE_COPY;
          }
          flags ^= CMD_PARSE_QUOTE;
        }
        slashes /= 2;
      }
      while (slashes--) {
        if (lpstr) {
          *lpstr++ = TEXT('\\');
        }
        *numbytes += sizeof(TCHAR);
      }
      if ((*p == TEXT('\0')) || ((!(flags & CMD_PARSE_QUOTE)) && ((*p == TEXT(' ')) || (*p == TEXT('\t'))))) {
        break;
      }
      if (flags & CMD_PARSE_COPY) {
        if (lpstr) {
          *lpstr++ = *p;
        }
        *numbytes += sizeof(TCHAR);
      }
      ++p;
    }
    if (lpstr) {
      *lpstr++ = TEXT('\0');
    }
    *numbytes += sizeof(TCHAR);
  }
}

TCHAR **GetCmdLineArgs(DWORD *argc) {
TCHAR **argv;
TCHAR *cmdline;
DWORD cmdsize;
TCHAR prgname[MAX_PATH];
  argv = NULL;
  if (argc) {
    cmdline = GetCommandLine();
    if ((!cmdline) || (!*cmdline)) {
      GetModuleFileName(NULL, prgname, MAX_PATH);
      cmdline = prgname;
    }
    ParseCmdLine(cmdline, NULL, NULL, argc, &cmdsize);
    argv = (TCHAR **) GetMem((*argc + 1) * sizeof(cmdline) + cmdsize);
    if (argv) {
      argv[*argc] = NULL;
      ParseCmdLine(
        cmdline, argv,
        (TCHAR *) (((BYTE *) argv) + (*argc + 1) * sizeof(cmdline)),
        argc, &cmdsize
      );
    }
  }
  return(argv);
}

/* == FILE AND DISK ROUTINES =============================================== */

TCHAR *GetFullFilePath(TCHAR *filename) {
TCHAR *result, *np;
int sz;
  sz = GetFullPathName(filename, 0, NULL, &np);
  result = STR_ALLOC(sz);
  GetFullPathName(filename, sz + 1, result, &np);
  result[sz] = 0;
  return(result);
}

TCHAR *GetWndText(HWND wnd) {
TCHAR *result;
int sz;
  sz = GetWindowTextLength(wnd);
  result = STR_ALLOC(sz);
  GetWindowText(wnd, result, sz + 1);
  result[sz] = 0;
  return(result);
}

TCHAR *OpenSaveDialog(HWND wnd, TCHAR *filemask, TCHAR *defext, int savedlg) {
OPENFILENAME ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize     = sizeof(ofn);
  ofn.hwndOwner       = wnd;
  ofn.nMaxFile        = MAX_PATH;
  ofn.lpstrFile       = STR_ALLOC(ofn.nMaxFile);
  ofn.Flags           = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR
/// OFN_ALLOWMULTISELECT
// http://stackoverflow.com/questions/656655/getopenfilename-with-ofn-allowmultiselect-flag-set
// http://support.microsoft.com/kb/131462
  ofn.lpstrFilter     = filemask;
  ofn.lpstrDefExt     = defext;
  if (savedlg) {
    if (GetSaveFileName(&ofn)) {
      ofn.lpstrFileTitle = GetFullFilePath(ofn.lpstrFile);
      FreeMem(ofn.lpstrFile);
      return(ofn.lpstrFileTitle);
    }
  } else {
    if (GetOpenFileName(&ofn)) {
      ofn.lpstrFileTitle = GetFullFilePath(ofn.lpstrFile);
      FreeMem(ofn.lpstrFile);
      return(ofn.lpstrFileTitle);
    }
  }
  FreeMem(ofn.lpstrFile);
  return(NULL);
}
