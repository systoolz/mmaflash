#ifndef __FLASHKIT_H
#define __FLASHKIT_H

// "FWS" (none compression)
#define FK_SIGN_FWS 0x00535746
// "CWS" (ZLIB compression)
#define FK_SIGN_CWS 0x00535743
// v1.1
// (!) ZWS file compression is permitted in SWF 13 or later only.
// "ZWS" (LZMA compression)
#define FK_SIGN_ZWS 0x0053575A
// "MZ"
#define FK_SIGN_EXE 0x5A4D
// overlay
#define FK_SIGN_END 0xFA123456

// get version
#define FK_GET_FVER(x) HIBYTE(HIWORD((x)))
// get sign
#define FK_GET_SIGN(x) ((x) & 0x00FFFFFF)

#pragma pack(push, 1)
typedef struct tagFLASHINFO {
  DWORD HeadSign; // FWS = ShockWaveFile; CWS = ShockWaveCompressed
  DWORD HeadSize; // FWS == filesize; CWS == unpacked filesize
  DWORD FileOffs; // Flash file offset (0 == from start, no overlay; this is also player size)
  DWORD FileSize; // Flash file length (same as headsize for FWS)
} FLASHINFO, *LPFLASHINFO;
#pragma pack(pop)

DWORD FK_GetRequiredPlayerVersion(DWORD dwFlash);
void FK_GetFileInfo(TCHAR *FileName, FLASHINFO *fi);
void FK_ExtractPlayer(TCHAR *fsource, TCHAR *filename);
void FK_HandleFile(HWND wnd, TCHAR *fsource, TCHAR *filename, TCHAR *fplayer, DWORD flags);

#endif
