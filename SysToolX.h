#ifndef __SYSTOOLX_H
#define __SYSTOOLX_H

#define MEM_MOVE(x, y) ((y *)x)[0]
#define STR_SIZE(x) (lstrlen(x) + 1) * sizeof(TCHAR)
#define STR_ALLOC(x) (TCHAR *) GetMem((x + 1) * sizeof(TCHAR))

void FreeMem(void *block);
void *GetMem(DWORD dwSize);
void *GrowMem(void *block, DWORD dwSize);

TCHAR *LangLoadString(UINT sid);

TCHAR **GetCmdLineArgs(DWORD *argc);

TCHAR *GetFullFilePath(TCHAR *filename);
TCHAR *GetWndText(HWND wnd);

TCHAR *OpenSaveDialog(HWND wnd, TCHAR *filemask, TCHAR *defext, int savedlg);

// v1.1
void URLOpenLink(HWND wnd, TCHAR *s);

int MsgBox(HWND wnd, TCHAR *lpText, UINT uType);
void DialogEnableWindow(HWND hdlg, int idControl, BOOL bEnable);
DWORD GetFileVersionMS(TCHAR *filename);

// v1.5
BYTE *HTTPGetContent(TCHAR *url, DWORD *len);

#endif
