#include <windows.h>
#include <commctrl.h>
#include "SysToolX.h"
#include "FlashKit.h"

// http://code.google.com/p/miniz/
// miniz flags
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
// miniz routines
#define malloc(x) GetMem((x))
#define free(x) FreeMem((x))
#define realloc(x,y) GrowMem((x),(y))
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
// remove assert()
#define NDEBUG
#include "miniz/miniz.c"
#undef NDEBUG

// FIXME: miniz stubs
void *memset(void *s, int c, size_t n) {
char *p;
  p = s;
  while (n--) {
    *(p++) = (char) c;
  }
  return(s);
}

// --------------------------------------------------------------------------

/*
// older version
{ 9,  9, 0}
{10, 10, 0}
// first block
{11, 10, 2}
{12, 10, 3}
// second block
{13, 11, 0}
{14, 11, 1}
{15, 11, 2}
{16, 11, 3}
{17, 11, 4}
{18, 11, 5}
{19, 11, 6}
{20, 11, 7}
{21, 11, 8}
{22, 11, 9}
// third block
{23, 12, 0}
{24, 13, 0}
{25, 14, 0}
{26, 15, 0}
{27, 16, 0}
{28, 17, 0}
{29, 18, 0}
{30, 19, 0}
{31, 20, 0}
*/

#define MAKEVERS(x,y) MAKELONG((y),(x))

// http://www.adobe.com/devnet/articles/flashplayer-air-feature-list.html
// http://sleepydesign.blogspot.com/2012/04/flash-swf-version-meaning.html
DWORD FK_GetRequiredPlayerVersion(DWORD dwFlash) {
DWORD result;
  // according to Adobe specification:
  // "ZWS file compression is permitted in SWF 13 or later only."
  if (FK_GET_SIGN(dwFlash) == FK_SIGN_ZWS) {
    dwFlash = max(FK_GET_FVER(dwFlash), 13);
  } else {
    dwFlash = FK_GET_FVER(dwFlash);
  }
  // fix warnings
  result = 0;
  // older version
  if (dwFlash <= 10) {
    result = MAKEVERS(dwFlash, 0);
  }
  // newer version
  // first block
  if ((dwFlash >= 11) && (dwFlash <= 12)) {
    result = MAKEVERS(10, dwFlash - 9);
  }
  // second block
  if ((dwFlash >= 13) && (dwFlash <= 22)) {
    result = MAKEVERS(11, dwFlash - 13);
  }
  // third block
  if ((dwFlash >= 23) && (dwFlash <= 31)) {
    result = MAKEVERS(dwFlash - 11, 0);
  }
  // avoid conflicts with possible future versions
  if (dwFlash >= 32) {
    result = MAKEVERS(20, 0);
  }
  return(result);
}

void FK_GetFileInfo(TCHAR *FileName, FLASHINFO *fi) {
DWORD flag, dw, data;
HANDLE fl;
  flag = 0;
  ZeroMemory(fi, sizeof(*fi));
  fl = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fl != INVALID_HANDLE_VALUE) {
    fi->FileSize = GetFileSize(fl, NULL);
    ReadFile(fl, &data, 4, &dw, NULL);
    if (LOWORD(data) == FK_SIGN_EXE) {
      // FIXME: check for correct player executable
      if (1) {
        SetFilePointer(fl, fi->FileSize - 8, NULL, FILE_BEGIN);
        fi->FileOffs = fi->FileSize;
        data = 0;
        ReadFile(fl, &data, 4, &dw, NULL);
        // check attach
        if (data == FK_SIGN_END) {
          // read attach size
          ReadFile(fl, &fi->FileSize, 4, &dw, NULL);
          fi->FileOffs -= (8 + fi->FileSize);
          SetFilePointer(fl, fi->FileOffs, NULL, FILE_BEGIN);
          flag |= 4;
        }
        flag |= 2;
      }
    }
    if (flag&4) {
      flag ^= 4;
    } else {
      SetFilePointer(fl, 0, NULL, FILE_BEGIN);
    }
    // read attach signature
    ReadFile(fl, fi, 8, &dw, NULL);
    data = FK_GET_SIGN(fi->HeadSign);
    // v1.1: add ZWS
    if ((data == FK_SIGN_FWS) || (data == FK_SIGN_CWS) || (data == FK_SIGN_ZWS)) {
      flag |= 1;
    }
    CloseHandle(fl);
  }
  data = (flag&2) ? fi->FileOffs : 0;
  if (!(flag&1)) {
    ZeroMemory(fi, sizeof(*fi));
  }
  fi->FileOffs = data;
}

// v1.1
#pragma pack(push, 1)
typedef struct {
  HWND wnd;
  DWORD BlockPos;
  DWORD FileSize;
  BYTE *outbuf;
} MZCBDATA;
#pragma pack(pop)

// v1.1
static int tinfl_put_buf_func(const void *pBuf, int len, void *pUser) {
MZCBDATA *cd;
  cd = (MZCBDATA *) pUser;
  // prevent buffer overflow
  len = min(len, cd->FileSize - cd->BlockPos);
  // update handled block size
  cd->BlockPos += len;
  // prevent divide by 0
  if (cd->FileSize >= 100) {
    // update progress bar %
    SendMessage(cd->wnd, PBM_SETPOS, cd->BlockPos/(cd->FileSize/100), 0);
    // update window
    UpdateWindow(GetParent(cd->wnd));
  }
  // move data block to output buffer
  MoveMemory(cd->outbuf, pBuf, len);
  // move output buffer pointer
  cd->outbuf += len;
  // prevent buffer overflow
  return(cd->BlockPos < cd->FileSize);
}

BYTE *FK_UnpackFile(HWND wnd, BYTE *pbuf, DWORD *psize) {
DWORD usize;
BYTE *ubuf;
MZCBDATA cd;
  ubuf = pbuf;
  if ((*psize > 8) && ((MEM_MOVE(pbuf, DWORD) & 0x00FFFFFF) == FK_SIGN_CWS)) {
    usize = MEM_MOVE(&pbuf[4], DWORD);
    ubuf = (BYTE *) GetMem(usize);
    // memory allocated
    if (ubuf) {
      usize -= 8;
//      uncompress(&ubuf[8], &usize, &pbuf[8], *psize - 8);
      // v1.1
      cd.wnd = wnd;
      cd.BlockPos = 0;
      cd.FileSize = usize;
      cd.outbuf = &ubuf[8];
      usize = *psize - 8;
      tinfl_decompress_mem_to_callback(&pbuf[8], (size_t *) &usize, tinfl_put_buf_func, &cd, TINFL_FLAG_PARSE_ZLIB_HEADER);
      // ===
      MEM_MOVE(ubuf, DWORD) = (MEM_MOVE(pbuf, DWORD) & 0xFF000000) | FK_SIGN_FWS;
      *psize = MEM_MOVE(&pbuf[4], DWORD);
      MEM_MOVE(&ubuf[4], DWORD) = *psize;
      FreeMem(pbuf);
    } else {
      ubuf = pbuf;
    }
  }
  return(ubuf);
}

BYTE *FK_PackFile(HWND wnd, BYTE *ubuf, DWORD *usize) {
DWORD psize, us;
BYTE *pbuf;
MZCBDATA cd;
  pbuf = ubuf;
  if ((*usize > 8) && ((MEM_MOVE(ubuf, DWORD) & 0x00FFFFFF) == FK_SIGN_FWS)) {
    us = MEM_MOVE(&ubuf[4], DWORD);
    if (us > 8) {
      us -= 8;
//      psize = compressBound(us);
      // FIXME: copy/paste from miniz.c
      // saves about 2.5 Kb of executable with MINIZ_NO_ZLIB_* flags
      psize = max(128 + (us * 110) / 100, 128 + us + ((us / (31 * 1024)) + 1) * 5);
      pbuf = (BYTE *) GetMem(psize + 8);
      // memory allocated
      if (pbuf) {
//        compress2(&pbuf[8], &psize, &ubuf[8], us, Z_BEST_COMPRESSION); // Z_OK
        // v1.1
        cd.wnd = wnd;
        cd.BlockPos = 0;
        cd.FileSize = psize;
        cd.outbuf = &pbuf[8];
        // 0x3300 - Z_BEST_COMPRESSION + zlib flags
        tdefl_compress_mem_to_output(&ubuf[8], us, tinfl_put_buf_func, &cd, 0x3300);
        // fix output size
        psize = cd.BlockPos;
        // ===
        MEM_MOVE(pbuf, DWORD) = (MEM_MOVE(ubuf, DWORD) & 0xFF000000) | FK_SIGN_CWS;
        MEM_MOVE(&pbuf[4], DWORD) = MEM_MOVE(&ubuf[4], DWORD);
        FreeMem(ubuf);
        *usize = psize + 8;
      } else {
        pbuf = ubuf;
      }
    }
  }
  return(pbuf);
}

BYTE *FK_ReadBlock(TCHAR *filename, DWORD Offs, DWORD Size) {
BYTE *result;
HANDLE fl;
DWORD dw;
  result = NULL;
  // sanity check
  if (filename && Size) {
    fl = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (fl != INVALID_HANDLE_VALUE) {
      SetFilePointer(fl, Offs, NULL, FILE_BEGIN);
      result = (BYTE *) GetMem(Size);
      ReadFile(fl, result, Size, &dw, NULL);
      CloseHandle(fl);
    }
  }
  return(result);
}

void FK_ExtractPlayer(TCHAR *fsource, TCHAR *filename) {
FLASHINFO fs;
HANDLE fl;
DWORD dw;
BYTE *p;
  // sanity check
  if (fsource && filename) {
    // get source file information
    FK_GetFileInfo(fsource, &fs);
    if (fs.FileOffs) {
      p = FK_ReadBlock(fsource, 0, fs.FileOffs);
      if (p) {
        // create output file
        fl = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (fl != INVALID_HANDLE_VALUE) {
          WriteFile(fl, p, fs.FileOffs, &dw, NULL);
          CloseHandle(fl);
        } // CreateFile()
        FreeMem(p);
      } // p
    } // offs
  } // schk
}

void FK_HandleFile(HWND wnd, TCHAR *fsource, TCHAR *filename, TCHAR *fplayer, DWORD flags) {
FLASHINFO fs, fp;
DWORD dw, sz;
HANDLE fl;
BYTE *p;
  if (flags) {
    // get source file information
    FK_GetFileInfo(fsource, &fs);
    if (fs.FileSize) {
      // create output file
      fl = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (fl != INVALID_HANDLE_VALUE) {
        p = NULL;
        sz = 0;
        fp.FileOffs = 0;
        // handle player
        switch (HIWORD(flags)) {
          // keep player
          case 0:
            // player exists in input file - move player to output file
            if (fs.FileOffs) {
              p = FK_ReadBlock(fsource, 0, fs.FileOffs);
              sz = fs.FileOffs;
            }
          // add/replace player file from fplayer
          case 2:
            // check that we don't go from case 0
            if (HIWORD(flags) == 2) {
              FK_GetFileInfo(fplayer, &fp);
              p = FK_ReadBlock(fplayer, 0, fp.FileOffs);
              sz = fp.FileOffs;
            }
            if (p) {
              WriteFile(fl, p, sz, &dw, NULL);
              FreeMem(p);
              // flag
              fp.FileOffs = sz;
            }
            break;
        }
        // handle flash
        sz = fs.FileSize;
        p = FK_ReadBlock(fsource, fs.FileOffs, sz);
        switch (LOWORD(flags)) {
          // decompress to FWS
          case 1:
          // compress to CWS
          case 2:
            // if compress selected for already compressed file
            // we should recompress it (uncompress and compress back)
            p = FK_UnpackFile(wnd, p, &sz);
            // check that we don't go from case 1
            if (LOWORD(flags) == 2) {
              p = FK_PackFile(wnd, p, &sz);
            }
            break;
        }
        if (p) {
          WriteFile(fl, p, sz, &dw, NULL);
          FreeMem(p);
        }
        // if there is a player - add footer
        if (fp.FileOffs) {
          fp.FileOffs = FK_SIGN_END;
          WriteFile(fl, &fp.FileOffs, 4, &dw, NULL);
          WriteFile(fl, &sz, 4, &dw, NULL);
        }
        CloseHandle(fl);
      } // CreateFile()
    } // fs.FileSize
  } // flags
}
