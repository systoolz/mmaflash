#include <windows.h>
#include <wininet.h>
#include "SysToolX.h"

void FreeMem(void *block) {
  if (block) {
    LocalFree(block);
  }
}

void *GetMem(DWORD dwSize) {
  return((void *) LocalAlloc(LPTR, dwSize));
}

void *GrowMem(void *block, DWORD dwSize) {
  if (!dwSize) {
    if (block) {
      FreeMem(block);
    }
    block = NULL;
  } else {
    if (block) {
      block = (void *) LocalReAlloc(block, dwSize, LMEM_MOVEABLE | LMEM_ZEROINIT);
    } else {
      block = GetMem(dwSize);
    }
  }
  return(block);
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
  if (result) {
    GetFullPathName(filename, sz + 1, result, &np);
    result[sz] = 0;
  }
  return(result);
}

TCHAR *GetWndText(HWND wnd) {
TCHAR *result;
int sz;
  sz = GetWindowTextLength(wnd);
  result = STR_ALLOC(sz);
  if (result) {
    GetWindowText(wnd, result, sz + 1);
    result[sz] = 0;
  }
  return(result);
}

TCHAR *OpenSaveDialog(HWND wnd, TCHAR *filemask, TCHAR *defext, int savedlg) {
OPENFILENAME ofn;
TCHAR filename[MAX_PATH], *result;
  result = NULL;
  filename[0] = 0;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner   = wnd;
  ofn.nMaxFile    = MAX_PATH;
  ofn.lpstrFile   = filename;
  ofn.Flags       = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR
  /// OFN_ALLOWMULTISELECT
  // http://stackoverflow.com/questions/656655/getopenfilename-with-ofn-allowmultiselect-flag-set
  // http://support.microsoft.com/kb/131462
  ofn.lpstrFilter = filemask;
  ofn.lpstrDefExt = defext;
  if ((savedlg ? GetSaveFileName : GetOpenFileName)(&ofn)) {
    result = GetFullFilePath(ofn.lpstrFile);
  }
  return(result);
}

// v1.1
void URLOpenLink(HWND wnd, TCHAR *s) {
  CoInitialize(NULL);
  ShellExecute(wnd, NULL, s, NULL, NULL, SW_SHOWNORMAL);
  CoUninitialize();
}

int MsgBox(HWND wnd, TCHAR *lpText, UINT uType) {
int result;
TCHAR *s, *t;
  s = NULL;
  if (!HIWORD(lpText)) {
    s = LangLoadString(LOWORD(lpText));
  }
  t = GetWndText(wnd);
  result = MessageBox(wnd, s ? s : lpText, t, uType);
  if (t) { FreeMem(t); }
  if (s) { FreeMem(s); }
  return(result);
}

// http://blogs.msdn.com/b/oldnewthing/archive/2004/08/04/208005.aspx
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable) {
HWND hctl;
  hctl = GetDlgItem(hdlg, idControl);
  if ((bEnable == FALSE) && (hctl == GetFocus())) {
    SendMessage(hdlg, WM_NEXTDLGCTL, 0, FALSE);
  }
  EnableWindow(hctl, bEnable);
}

// PATCH: Windows 98 combobox fix (common controls version < 6)
// http://blogs.msdn.com/b/oldnewthing/archive/2006/03/10/548537.aspx
// ! http://vbnet.mvps.org/index.html?code/comboapi/comboheight.htm
void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems) {
RECT rc;
  GetWindowRect(hWndCmbBox, &rc);
  rc.right -= rc.left;
  ScreenToClient(GetParent(hWndCmbBox), (POINT *) &rc);
  rc.bottom = (MaxVisItems + 2) * SendMessage(hWndCmbBox, CB_GETITEMHEIGHT, 0, 0);
  MoveWindow(hWndCmbBox, rc.left, rc.top, rc.right, rc.bottom, TRUE);
  // PATCH: enable integral height, ComboBox must be created with CBS_NOINTEGRALHEIGHT
  SetWindowLong(hWndCmbBox, GWL_STYLE, (GetWindowLong(hWndCmbBox, GWL_STYLE) | CBS_NOINTEGRALHEIGHT) ^ CBS_NOINTEGRALHEIGHT);
}

DWORD GetFileVersionMS(TCHAR *filename) {
DWORD result;
HINSTANCE hl;
HGLOBAL hg;
HRSRC hr;
BYTE *data;
  result = 0;
  hl = LoadLibraryEx(filename, 0, LOAD_LIBRARY_AS_DATAFILE);
  if (hl) {
    // http://stackoverflow.com/questions/13941837/how-to-get-version-info-from-resources
    hr = FindResource(hl, MAKEINTRESOURCE(1), RT_VERSION);
    if (hr) {
      hg = LoadResource(hl, hr);
      if (hg) {
        data = (BYTE *) LockResource(hg);
        // http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/resfmt.txt
        if (data) {
          result = MEM_MOVE(data, WORD);
          // "Each block of information is dword aligned." (c) link above
          while ((result > sizeof(VS_FIXEDFILEINFO)) && (MEM_MOVE(data, DWORD) != VS_FFI_SIGNATURE)) {
            result -= sizeof(DWORD);
            data   += sizeof(DWORD);
          }
          result = ((VS_FIXEDFILEINFO *) data)->dwFileVersionMS;
        } // data
      } // hg
    } // hr
    FreeLibrary(hl);
  } // hl
  return(result);
}

// v1.5
// it's not very optimized code but good for small internet pages
#define MAX_BLOCK_SIZE 1024
static const TCHAR strConClose[] = TEXT("Connection: Close");
static const TCHAR strHTTPVers[] = HTTP_VERSION;
BYTE *HTTPGetContent(TCHAR *url, DWORD *len) {
HINTERNET hOpen, hConn, hReq;
URL_COMPONENTS uc;
BYTE *result, *buf;
DWORD sz;
  // init result
  result = NULL;
  *len = 0;
  // prepare URL string
  ZeroMemory(&uc, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  sz = lstrlen(url);
  uc.lpszHostName     = STR_ALLOC(sz);
  uc.dwHostNameLength = sz;
  uc.lpszUrlPath      = STR_ALLOC(sz);
  uc.dwUrlPathLength  = sz;
  sz = InternetCrackUrl(url, lstrlen(url), 0, &uc);
  // sanity check
  if (sz && ((uc.nScheme == INTERNET_SCHEME_HTTPS) || (uc.nScheme == INTERNET_SCHEME_HTTP)) &&
      uc.lpszHostName && uc.lpszUrlPath && uc.lpszHostName[0] && uc.lpszUrlPath[0]
  ) {
    // open
    hOpen = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hOpen) {
      // connect
      sz = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
      hConn = InternetConnect(hOpen, uc.lpszHostName, sz, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
      if (hConn) {
        // request
        sz = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID) : 0;
        hReq = HttpOpenRequest(hConn, NULL, uc.lpszUrlPath, strHTTPVers, NULL, NULL, // v1.7
          /* INTERNET_FLAG_NO_AUTO_REDIRECT |*/ INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES |
          INTERNET_FLAG_NO_CACHE_WRITE | sz, 0);
        if (hReq) {
          sz = HttpSendRequest(hReq, strConClose, (DWORD) -1, NULL, 0);
          if (sz) {
            buf = GetMem(MAX_BLOCK_SIZE);
            do {
              sz = 0;
              InternetReadFile(hReq, buf, MAX_BLOCK_SIZE, &sz);
              if (sz) {
                result = GrowMem(result, *len + sz + 1);
                CopyMemory(&result[*len], buf, sz);
                *len += sz;
              }
            } while (sz);
            FreeMem(buf);
            // add 0 at the end for text buffer data
            if (result && *len) {
              result[*len] = 0;
            }
          } else {
            // Windows XP, TLS not enabled in Internet Explorer
            *len = ((GetLastError() == ERROR_INTERNET_CONNECTION_RESET) && (uc.nScheme == INTERNET_SCHEME_HTTPS)) ? 1 : 0;
          }
          InternetCloseHandle(hReq);
        }
        InternetCloseHandle(hConn);
      }
      InternetCloseHandle(hOpen);
    }
  }
  if (uc.lpszHostName) { FreeMem(uc.lpszHostName); }
  if (uc.lpszUrlPath)  { FreeMem(uc.lpszUrlPath);  }
  return(result);
}
