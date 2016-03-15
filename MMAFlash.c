#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource/MMAFlash.h"
#include "SysToolX.h"
#include "FlashKit.h"

TCHAR *OpenSaveDialogEx(HWND wnd, DWORD msk, int savedlg) {
TCHAR buf[128], *result, *s;
  result = NULL;
  buf[0] = 0;
  // flash movie
  if (msk & 1) {
    s = LangLoadString(IDS_MSK_SWF);
    if (s) {
      lstrcpy(&buf[lstrlen(buf)], s);
      FreeMem(s);
    }
  }
  // projector
  if (msk & 2) {
    s = LangLoadString(IDS_MSK_EXE);
    if (s) {
      lstrcpy(&buf[lstrlen(buf)], s);
      FreeMem(s);
    }
  }
  // all files mask must be always present
  s = LangLoadString(IDS_MSK_ALL);
  if (s) {
    lstrcpy(&buf[lstrlen(buf)], s);
    FreeMem(s);
  }
  for (s = buf; *s; s++) {
    if (*s == TEXT('|')) {
       *s = 0;
    }
  }
  // get default extension
  for (s = buf; *s; s++);
  for (s++; *s; s++);
  for (s--; *s; s--) {
    if (*s == TEXT('.')) {
      break;
    }
    if ((*s == TEXT('*')) || (*s == TEXT('?'))) {
      s = NULL;
      break;
    }
  }
  if (s) { s++; }
  result = OpenSaveDialog(wnd, buf, s, savedlg);
  return(result);
}

// v1.1
TCHAR *VerErrFmt(DWORD fmt, DWORD dwFlash, DWORD dwPlayer, DWORD dwRequired) {
TCHAR buf[1025], *s;
  s = LangLoadString(fmt);
  if (s) {
    wsprintf(buf, s, dwFlash, HIWORD(dwPlayer), LOWORD(dwPlayer), HIWORD(dwRequired), LOWORD(dwRequired));
    FreeMem(s);
    s = STR_ALLOC(lstrlen(buf));
    if (s) {
      lstrcpy(s, buf);
    }
  }
  return(s);
}

// v1.5
// http://www.adobe.com/devnet/flashplayer/articles/flash_player_admin_guide.html
static DWORD vmax = 0;
DWORD GetLatestFlashVersion(void) {
DWORD i, l, vcur;
TCHAR *s;
BYTE *b;
  // find latest version available
  if (!vmax) {
    vmax = FK_SA_MAX_VER;
    s = LangLoadString(IDS_FMT_VERSIONCHK);
    if (s) {
      // warning: this is ANSI text, not TEXT()
      /*
        <version>
        <Player major="21" majorBeta="21"/>
        </version>
      */
      b = HTTPGetContent(s, &l);
      if (b) {
        for (i = 0; i < l; i++) {
          if (b[i] == '"') {
            i++;
            break;
          }
        }
        // just in case
        while ((i < l) && ((b[i] == ' ') || (b[i] == '\t'))) { i++; }
        // number found
        if (i < l) {
          vcur = 0;
          while ((i < l) && (b[i] >= '0') && (b[i] <= '9')) {
            vcur *= 10;
            vcur += (b[i] - '0');
            i++;
          }
          // set the maximum version
          vmax = max(vcur, vmax);
        }
        FreeMem(b);
      }
      FreeMem(s);
    }
  }
  return(vmax);
}

// v1.1
void DownloadPlayerFile(HWND wnd, DWORD dwRequired) {
TCHAR buf[1025], *s;
HMENU hm;
DWORD i;
RECT rc;
  // there are no Player version lower than 10
  // available on the direct link at Adobe site
  if (dwRequired) {
    dwRequired = max(HIWORD(dwRequired), FK_SA_MIN_VER);
  }
  // create popup menu
  hm = CreatePopupMenu();
  if (hm) {
    // add header
    s = LangLoadString(IDS_FMT_PLAYERMENU);
    if (s) {
      AppendMenu(hm, MF_STRING | MF_DISABLED, 0, s);
      FreeMem(s);
      HiliteMenuItem(wnd, hm, 0, MF_BYPOSITION | MF_HILITE);
    }
    AppendMenu(hm, MF_SEPARATOR, 0, NULL);
    // add items
    s = LangLoadString(IDS_FMT_PLAYERNAME);
    if (s) {
      for (i = GetLatestFlashVersion(); i >= FK_SA_MIN_VER; i--) {
        wsprintf(buf, s, i);
        // also highlight minimum required player version
        AppendMenu(hm, MF_STRING | ((i == dwRequired) ? MF_CHECKED : 0), 100 + i, buf);
      }
      FreeMem(s);
    }
    // add footer
    AppendMenu(hm, MF_SEPARATOR, 0, NULL);
    s = LangLoadString(IDS_FMT_PLAYERSTOP);
    if (s) {
      AppendMenu(hm, MF_STRING, 0, s);
      FreeMem(s);
    }
    // get window coords
    GetWindowRect(wnd, &rc);
    rc.left += (rc.right - rc.left) / 2;
    rc.top += (rc.bottom - rc.top) / 2;
    // show menu
    i = (DWORD) TrackPopupMenu(hm, TPM_RETURNCMD | TPM_NONOTIFY | TPM_CENTERALIGN | TPM_VCENTERALIGN, rc.left, rc.top, 0, wnd, NULL);
    // cleanup
    DestroyMenu(hm);
    // user selected something
    if (i >= 100) {
      i -= 100;
      // load URL string format
      s = LangLoadString(IDS_FMT_PLAYERLINK);
      if (s) {
        wsprintf(buf, s, i, i);
        FreeMem(s);
        // open URL link
        URLOpenLink(wnd, buf);
      }
    }
  }
}

void UpdateFlashVersion(HWND wnd) {
TCHAR buf[1025], *st;
FLASHINFO fi;
  // init structs
  buf[0] = 0;
  ZeroMemory(&fi, sizeof(fi));
  // part 1 - get all items version
  // get flash and player version
  st = GetWndText(GetDlgItem(wnd, IDC_FSOURCE));
  if (st) {
    FK_GetFileInfo(st, &fi);
    // get player version and required
    if (fi.FileOffs) {
      fi.FileOffs = GetFileVersionMS(st);
    }
    FreeMem(st);
  }
  // external player version
  st = GetWndText(GetDlgItem(wnd, IDC_FPLAYER));
  if (st) {
    fi.HeadSize = GetFileVersionMS(st);
    FreeMem(st);
  } else {
    fi.HeadSize = 0;
  }
  // part 2 - output version infromation
  // put flash version
  if (fi.FileSize) {
    st = LangLoadString(IDS_VER_FLASHMOVIE);
    if (st) {
      wsprintf(buf, st, FK_GET_FVER(fi.HeadSign));
      FreeMem(st);
    }
    // put minimum required version
    st = LangLoadString(IDS_VER_PLAYERMREQ);
    if (st) {
      fi.HeadSign = FK_GetRequiredPlayerVersion(fi.HeadSign);
      wsprintf(&buf[lstrlen(buf)], st, HIWORD(fi.HeadSign), LOWORD(fi.HeadSign));
      FreeMem(st);
    }
  }
  // put current player version
  if (fi.FileOffs) {
    st = LangLoadString(IDS_VER_PLAYERFILE);
    if (st) {
      wsprintf(&buf[lstrlen(buf)], st, HIWORD(fi.FileOffs), LOWORD(fi.FileOffs));
      FreeMem(st);
    }
  }
  // put external player version
  if (fi.HeadSize) {
    st = LangLoadString(IDS_VER_PLAYERFROM);
    if (st) {
      wsprintf(&buf[lstrlen(buf)], st, HIWORD(fi.HeadSize), LOWORD(fi.HeadSize));
      FreeMem(st);
    }
  }
  SetDlgItemText(wnd, IDC_VERSION, buf);
}

void EnableDlgRadio(HWND wnd, DWORD bEnable) {
int i;
  for (i = 0; i < 3; i++) {
    DialogEnableWindow(wnd, IDC_FRADIO1 + i, (bEnable == 1) ? TRUE : FALSE);
    DialogEnableWindow(wnd, IDC_PRADIO1 + i, bEnable ? TRUE : FALSE);
  }
  // v1.1
  if (bEnable == 2) {
    CheckRadioButton(wnd, IDC_FRADIO1, IDC_FRADIO3, IDC_FRADIO1);
  }
}

void SwapBottomBar(HWND wnd, BOOL state) {
HWND wh;
  ShowWindow(GetDlgItem(wnd, IDC_HOMEPAGE), state ? SW_HIDE : SW_SHOW);
  wh = GetDlgItem(wnd, IDC_PROGRESS);
  SendMessage(wh, PBM_SETPOS, 0, 0);
  ShowWindow(wh, state ? SW_SHOW : SW_HIDE);
  UpdateWindow(wnd);
}

BOOL CALLBACK DlgPrc(HWND wnd, UINT msg, WPARAM wparm, LPARAM lparm) {
TCHAR *filename, *fplayer, *fsource;
DWORD i, j, f1, f2;
BOOL result;
FLASHINFO fi;
// v1.1
DRAWITEMSTRUCT *dis;
TCHAR *s;
  result = FALSE;
  switch (msg) {
    case WM_INITDIALOG:
      // set range for progress bar
      SendMessage(GetDlgItem(wnd, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
      // and hide it
      ShowWindow(GetDlgItem(wnd, IDC_PROGRESS), SW_HIDE);
      // add icons
      SendMessage(wnd, WM_SETICON, ICON_BIG  , (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICN)));
      SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICN)));
      // initial controls state
      CheckRadioButton(wnd, IDC_FRADIO1, IDC_FRADIO3, IDC_FRADIO1);
      CheckRadioButton(wnd, IDC_PRADIO1, IDC_PRADIO3, IDC_PRADIO1);
      // disable before open file
      EnableDlgRadio(wnd, 0);
      // disable "Extract player" button
      DialogEnableWindow(wnd, IDC_EXTRACT, FALSE);
      // disable "Save" button
      DialogEnableWindow(wnd, IDC_FLSAVE, FALSE);
      // set focus to "Open"
      SetFocus(GetDlgItem(wnd, IDC_FLOPEN));
      // handle file from command line
      if (lparm) {
        SetDlgItemText(wnd, IDC_FSOURCE, (TCHAR *) lparm);
        // add message to window message queue - emulate "Open..." click
        // note that lparam (handle of control) must be null - used as flag
        PostMessage(wnd, WM_COMMAND, MAKELONG(IDC_FLOPEN, BN_CLICKED), 0);
      }
      // must be true
      result = TRUE;
      break;
    case WM_COMMAND:
      if (HIWORD(wparm) == BN_CLICKED) {
        result = TRUE;
        switch (LOWORD(wparm)) {
          // exit from program
          case IDCANCEL:
            EndDialog(wnd, 0);
            break;
          // save output file
          case IDC_FLSAVE:
          case IDC_EXTRACT:
            fsource = GetWndText(GetDlgItem(wnd, IDC_FSOURCE));
            fplayer = GetWndText(GetDlgItem(wnd, IDC_FPLAYER));
            f1 = 0;
            f2 = 0;
            if (LOWORD(wparm) == IDC_FLSAVE) {
              for (i = 0; i < 3; i++) {
                if (IsDlgButtonChecked(wnd, IDC_FRADIO1 + i) == BST_CHECKED) {
                  f1 = i;
                }
                if (IsDlgButtonChecked(wnd, IDC_PRADIO1 + i) == BST_CHECKED) {
                  f2 = i;
                }
              }
              // <! test player & flash version - only if player not removed from file
              if (f2 != 1) {
                FK_GetFileInfo(fsource, &fi);
                // if use external player or there are a player inside source file
                if (f2 || fi.FileOffs) {
                  // v1.1
                  // get file version
                  i = GetFileVersionMS(f2 ? fplayer : fsource);
                  // get required version
                  j = FK_GetRequiredPlayerVersion(fi.HeadSign);
                  if (i >= j) {
                    i = 1;
                  } else {
                    // since fi structure won't used if error
                    fi.FileOffs = i;
                    fi.FileSize = j;
                    i = 0;
                  }
                } else {
                  i = 1; // .swf only
                }
              } else {
                i = 1; // .swf only - nothing to check
              }
            } else {
              i = 1; // player extraction - nothing to check
            }
            // !>
            // i not null - no errors
            if (i) {
              // file mask generation
              if (LOWORD(wparm) == IDC_FLSAVE) {
                i = f2;
                if (!i) {
                  i = fi.FileOffs ? 2 : 1;
                }
              } else {
                i = 2; // .exe only
              }
              filename = OpenSaveDialogEx(wnd, i, 1);
              if (filename) {
                // save file to disk
                if (LOWORD(wparm) == IDC_FLSAVE) {
                  SwapBottomBar(wnd, TRUE);
                  FK_HandleFile(GetDlgItem(wnd, IDC_PROGRESS), fsource, filename, fplayer, MAKELONG(f1, f2));
                  SwapBottomBar(wnd, FALSE);
                } else {
                  FK_ExtractPlayer(fsource, filename);
                }
                FreeMem(filename);
                MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_DONE), MB_ICONINFORMATION);
              }
            } else {
              // v1.1
              s = VerErrFmt(IDS_MSG_BADVERSION, FK_GET_FVER(fi.HeadSign), fi.FileOffs, fi.FileSize);
              if (s) {
                // ask for action
                i = MsgBox(wnd, s, MB_ICONWARNING | MB_YESNO);
                FreeMem(s);
                // download appropriate player version
                if (i == IDYES) {
                  DownloadPlayerFile(wnd, fi.FileSize);
                }
              }
            }
            if (fplayer) { FreeMem(fplayer); }
            if (fsource) { FreeMem(fsource); }
            break;
          // open file for work
          case IDC_FLOPEN:
            // lparam are zero if got here from WM_INITDIALOG
            filename = lparm ? OpenSaveDialogEx(wnd, 3, 0) : GetWndText(GetDlgItem(wnd, IDC_FSOURCE));
            // init edit control
            if (!lparm) { SetDlgItemText(wnd, IDC_FSOURCE, (TCHAR *) &lparm); }
            if (filename) {
              FK_GetFileInfo(filename, &fi);
              if (fi.FileSize) {
                SetDlgItemText(wnd, IDC_FSOURCE, filename);
                // scroll text in edit box to the end, so user can see a filename
                // http://support.microsoft.com/kb/12190
                SendMessage(GetDlgItem(wnd, IDC_FSOURCE), EM_SETSEL, 0, -1);
                // enable/disable "Extract player" button
                DialogEnableWindow(wnd, IDC_EXTRACT, fi.FileOffs ? TRUE : FALSE);
                UpdateFlashVersion(wnd);
                // v1.1 LZMA not supported
                i = (FK_GET_SIGN(fi.HeadSign) == FK_SIGN_ZWS) ? 1 : 0;
                // enable radio buttons
                EnableDlgRadio(wnd, 1 + i);
                // show information about LZMA
                if (i) {
                  MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_LZMA), MB_ICONINFORMATION);
                }
              } else {
                MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_BADFLASH), MB_ICONERROR);
              }
              FreeMem(filename);
            }
            break;
          case IDC_FRADIO1:
          case IDC_FRADIO2:
          case IDC_FRADIO3:
          case IDC_PRADIO1:
          case IDC_PRADIO2:
          case IDC_PRADIO3:
            // PATCH: keyboard input sends two (!) BN_CLICKED messages when selecting radiobutton
            if ((LOWORD(wparm) == IDC_PRADIO3) && (!IsDlgButtonChecked(wnd, IDC_PRADIO3))) {
              break;
            }
            // enable/disable "Save" button
            DialogEnableWindow(wnd, IDC_FLSAVE, (IsDlgButtonChecked(wnd, IDC_FRADIO1) && IsDlgButtonChecked(wnd, IDC_PRADIO1)) ? FALSE : TRUE);
            if (LOWORD(wparm) == IDC_PRADIO3) {
              filename = OpenSaveDialogEx(wnd, 2, 0);
              if (filename) {
                FK_GetFileInfo(filename, &fi);
                if (fi.FileOffs) {
                  SetDlgItemText(wnd, IDC_FPLAYER, filename);
                  SendMessage(GetDlgItem(wnd, IDC_FPLAYER), EM_SETSEL, 0, -1);
                  UpdateFlashVersion(wnd);
                } else {
                  CheckRadioButton(wnd, IDC_PRADIO1, IDC_PRADIO3, IDC_PRADIO1);
                  SetFocus(GetDlgItem(wnd, IDC_PRADIO1));
                  MsgBox(wnd, MAKEINTRESOURCE(IDS_MSG_BADPLAYER), MB_ICONERROR);
                }
                FreeMem(filename);
              } else {
                CheckRadioButton(wnd, IDC_PRADIO1, IDC_PRADIO3, IDC_PRADIO1);
                SetFocus(GetDlgItem(wnd, IDC_PRADIO1));
                SendMessage(wnd, WM_COMMAND, (WPARAM) MAKELONG(IDC_PRADIO1, BN_CLICKED), (LPARAM) GetDlgItem(wnd, IDC_PRADIO1));
              }
            }
            break;
          // v1.1
          case IDC_HOMEPAGE:
            // get control text
            s = GetWndText(GetDlgItem(wnd, IDC_HOMEPAGE));
            if (s) {
              // find link splitter
              for (i = 0; s[i]; i++) {
                // found it
                if (s[i] == TEXT('|')) {
                  // remove spaces
                  for (i++; s[i] == TEXT(' '); i++);
                  // shift string
                  lstrcpy(s, &s[i]);
                  // flag link as found
                  i = 0;
                  break;
                }
              }
              // open link
              if ((!i) && *s) {
                URLOpenLink(wnd, s);
              }
              // free memory
              FreeMem(s);
            }
            break;
          // v1,2
          case IDC_GETPLAY:
            // just in case highlight required player version
            fsource = GetWndText(GetDlgItem(wnd, IDC_FSOURCE));
            ZeroMemory(&fi, sizeof(fi));
            if (fsource) {
              FK_GetFileInfo(fsource, &fi);
              FreeMem(fsource);
            }
            DownloadPlayerFile(wnd, fi.FileSize ? FK_GetRequiredPlayerVersion(fi.HeadSign) : 0);
            break;
        }
      }
      break;
    // v1.1
    // http://www.codeproject.com/Articles/13505/Win-C-Easy-Hyper-Link
    // https://msdn.microsoft.com/en-us/library/windows/desktop/hh404152.aspx
    case WM_DRAWITEM:
      dis = (DRAWITEMSTRUCT *) lparm;
      if (dis && (dis->CtlID == IDC_HOMEPAGE)) {
        s = GetWndText(dis->hwndItem);
        if (s) {
          SetTextColor(dis->hDC, RGB(0, 0, 0xFF));
          SetBkMode(dis->hDC, TRANSPARENT);
          DrawText(dis->hDC, s, -1, &dis->rcItem, DT_LEFT | DT_TOP | DT_SINGLELINE);
          FreeMem(s);
          result = TRUE;
        }
      }
      break;
    // http://stackoverflow.com/questions/19257237/reset-cursor-in-wm-setcursor-handler-properly
    // http://www.codeproject.com/Articles/1724/Some-handy-dialog-box-tricks-tips-and-workarounds
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms645469.aspx
    case WM_SETCURSOR:
      if (((HWND) wparm) == GetDlgItem(wnd, IDC_HOMEPAGE)) {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        SetWindowLongPtr(wnd, DWLP_MSGRESULT, TRUE);
        result = TRUE;
      }
      break;
  }
  return(result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
TCHAR **argv, *openfile;
DWORD argc;
  // DialogBox*() won't work with manifest.xml in resources if this never called
  // or comctl32.dll not linked to the executable file
  // or not called DLLinit through LoadLibrary(TEXT("comctl32.dll"))
  InitCommonControls();

  openfile = NULL;
  argv = GetCmdLineArgs(&argc);
  if (argv) { // v1.1
    if (argc == 2) {
      openfile = GetFullFilePath(argv[1]);
    }
    FreeMem(argv);
  }

  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DLG), 0, &DlgPrc, (LPARAM) openfile);

  if (openfile) {
    FreeMem(openfile);
  }

  ExitProcess(0);
  return(0);
}
