/*
 * Retro68 compatibility layer for the Lion Entertainment DOOM sources.
 *
 * The original code was built with CodeWarrior / MPW against Apple's
 * Universal Interfaces.  Retro68 ships the Multiversal Interfaces, which
 * put nearly everything in one header; this file pulls that in and fills
 * the handful of gaps.  It is force-included into every translation unit.
 */
#ifndef RETRO68_COMPAT_H
#define RETRO68_COMPAT_H

#include <Multiverse.h>

#ifndef GENERATINGPOWERPC
#define GENERATINGPOWERPC TARGET_CPU_PPC
#endif
#ifndef GENERATING68K
#define GENERATING68K TARGET_CPU_68K
#endif
#ifndef USES68KINLINES
#define USES68KINLINES 0
#endif

/* ---- MPW / Universal Interfaces inline-trap spellings ---- */
#ifndef ONEWORDINLINE
#define ONEWORDINLINE(a)                 M68K_INLINE(a)
#define TWOWORDINLINE(a,b)               M68K_INLINE(a,b)
#define THREEWORDINLINE(a,b,c)           M68K_INLINE(a,b,c)
#define FOURWORDINLINE(a,b,c,d)          M68K_INLINE(a,b,c,d)
#define FIVEWORDINLINE(a,b,c,d,e)        M68K_INLINE(a,b,c,d,e)
#define SIXWORDINLINE(a,b,c,d,e,f)       M68K_INLINE(a,b,c,d,e,f)
#endif

/* ---- Mixed Mode ProcInfo macros (Apple's MixedMode.h; Multiversal has the
   enum constants but not these).  On classic 68K the values are computed but
   never used; the PowerPC build hands them to NewRoutineDescriptor. ---- */
#ifndef RESULT_SIZE
#define SIZE_CODE(size) \
    (((size) == 4) ? kFourByteCode : (((size) == 2) ? kTwoByteCode : (((size) == 1) ? kOneByteCode : 0)))
#define RESULT_SIZE(sizeCode) \
    ((ProcInfoType)(sizeCode) << 4)
#define STACK_ROUTINE_PARAMETER(whichParam, sizeCode) \
    ((ProcInfoType)(sizeCode) << (6 + (((whichParam) - 1) * 2)))
#define REGISTER_RESULT_LOCATION(whichReg) \
    ((ProcInfoType)(whichReg) << 6)
#define REGISTER_ROUTINE_PARAMETER(whichParam, whichReg, sizeCode) \
    ((((ProcInfoType)(sizeCode)) | ((ProcInfoType)(whichReg) << 2)) << (11 + (((whichParam) - 1) * 5)))
#define SPECIAL_CASE_PROCINFO(specialCaseCode) \
    (kSpecialCase | ((ProcInfoType)(specialCaseCode) << 4))
#endif
#ifndef USESROUTINEDESCRIPTORS
#define USESROUTINEDESCRIPTORS TARGET_CPU_PPC
#endif
#if TARGET_CPU_68K
/* Multiversal declares these as _MixedModeDispatch traps, which classic 68K
   systems don't implement ("unimplemented trap").  Apple's 68K headers made
   them plain casts / no-ops; do the same. */
#define NewRoutineDescriptor(proc, info, isa)   ((UniversalProcPtr)(proc))
#define DisposeRoutineDescriptor(upp)           ((void)(upp))
#endif
typedef ProcPtr Register68kProcPtr;
enum { kSpecialCaseProtocolHandler = 7, kSpecialCaseSocketListener = 8 };
typedef const unsigned char *ConstStr32Param;

/* ---- Virtual Memory (_MemoryDispatch) ---- */
#pragma parameter __D0 HoldMemory(__A0, __A1)
pascal OSErr HoldMemory(void *address, unsigned long count) M68K_INLINE(0x7000, 0xA05C);

/* ---- AppleTalk error codes (Universal Headers Errors.h) ---- */
enum { noBridgeErr = -93, excessCollsns = -95, portInUse = -97, portNotCf = -98,
       extractErr = -3104, dsChkErr = 5, ddpSktErr = -91,
       nbpDuplicate = -1027, nbpNotFound = -1028 };

/* ---- QuickDraw / GDevice constants (Universal Interfaces Quickdraw.h) ---- */
enum { clutType = 0, fixedType = 1, directType = 2 };
enum { gdDevType = 0, burstDevice = 7, ext32Device = 8, ramInit = 10,
       mainScreen = 11, allInit = 12, screenDevice = 13, noDriver = 14,
       screenActive = 15 };
typedef ColorSpec CSpecArray[1];

/* ---- Folder Manager ---- */
enum { kOnSystemDisk = (short)0x8000 };
enum { kCreateFolder = 1, kDontCreateFolder = 0 };

/* ---- Event modifiers: Multiversal spells it ControlKey ---- */
enum { controlKey = 0x1000 };

/* ---- QuickTime ---- */
enum { newMovieActive = 1 << 0, newMovieDontResolveDataRefs = 1 << 1,
       newMovieDontAskUnresolvedDataRefs = 1 << 2 };

/* ---- Sound Manager commands/flags missing from Multiversal ---- */
enum { reInitCmd = 5, volumeCmd = 46, getVolumeCmd = 47 };
enum { initMono = 0x0080, initStereo = 0x00C0 };

/* ---- Misc Toolbox constants missing from Multiversal ---- */
enum { false32b = 0, true32b = 1 };                       /* SwapMMUMode */
enum { OSTrap = 0, ToolTrap = 1 };                        /* TrapType */
enum { suspendResumeMessage = 1, resumeFlag = 1, convertClipboardFlag = 2 };
enum { geneva = 3, monaco = 4 };
enum { gestaltCPU68000 = 0, gestaltCPU68010 = 1, gestaltCPU68020 = 2,
       gestaltCPU68030 = 3 };
enum { gestaltHelpMgrPresent = 0 };
#define gestaltQuickTime        'qtim'
#define keyMissedKeywordAttr    'miss'

/* ---- Cursor Device Manager (_CursorDeviceDispatch, 0xAADB) ---- */
typedef struct CursorDevice CursorDevice, *CursorDevicePtr;
pascal OSErr CursorDeviceMoveTo(CursorDevicePtr ourDevice, long absX, long absY)
    M68K_INLINE(0x7001, 0xAADB);
pascal OSErr CursorDeviceNextDevice(CursorDevicePtr *ourDevice)
    M68K_INLINE(0x700B, 0xAADB);

/* ---- Pre-System 7.5 routine names (Universal Interfaces OldRoutineNames) ---- */
#define DisposPtr           DisposePtr
#define DisposHandle        DisposeHandle
#define DisposCTable        DisposeCTable
#define DisposDialog        DisposeDialog
#define DisposGDevice       DisposeGDevice
#define GetDItem            GetDialogItem
#define SetDItem            SetDialogItem
#define ShowDItem           ShowDialogItem
#define HideDItem           HideDialogItem
#define GetIText            GetDialogItemText
#define SetIText            SetDialogItemText
#define SelIText            SelectDialogItemText
#define GetCtlValue         GetControlValue
#define SetCtlValue         SetControlValue
#define GetMHandle          GetMenuHandle
#define GetItem             GetMenuItemText
#define SetItem             SetMenuItemText
#define AddResMenu          AppendResMenu
#define TextBox             TETextBox
#define GetGrayRgn()        LMGetGrayRgn()
typedef TMTask *TMTaskPtr;
#if TARGET_CPU_PPC
enum { uppTimerProcInfo = kRegisterBased | REGISTER_ROUTINE_PARAMETER(1, kRegisterA1, kFourByteCode) };
#define NewTimerProc(p)     NewRoutineDescriptor((ProcPtr)(p), uppTimerProcInfo, GetCurrentArchitecture())
#else
#define NewTimerProc(p)     ((ProcPtr)(p))
#endif

/* Low-memory accessors Multiversal declares but provides no glue for. */
#define GetMMUMode()        ((Byte)LMGetMMU32Bit())
#define SetEventMask(m)     (*(short *)0x0144 = (short)(m))   /* SysEvtMask */

/* ---- Time Manager ---- */
#pragma parameter InsXTime(__A0)
pascal void InsXTime(QElemPtr tmTaskPtr) M68K_INLINE(0xA458);

/* ---- File Manager glue (lives in mac_glue.c) ---- */
OSErr HGetVol(StringPtr volName, short *vRefNum, long *dirID);
OSErr DirCreate(short vRefNum, long parentDirID, ConstStr255Param directoryName,
                long *createdDirID);

/* ---- MPW and CodeWarrior compiled "\n" as a carriage return (0x0D), the
   Mac line break; GCC makes it a line feed (0x0A), which alerts draw as a box.
   Route ParamText through a copy that turns line feeds into returns. ---- */
pascal void Retro68ParamText(ConstStr255Param p0, ConstStr255Param p1,
                             ConstStr255Param p2, ConstStr255Param p3);
#define ParamText Retro68ParamText

/* ---- C <-> Pascal string conversion in place (mac_glue.c) ---- */
char          *p2cstr(unsigned char *s);
unsigned char *c2pstr(char *s);
void MacSetEntries(short start, short count, CTabHandle ctab);  /* mac_glue.c */

/* ---- QuickTime calls missing from Multiversal; selectors from Apple's
   Universal Headers 2.0a3 Movies.h (shipped in this repo under CW5). ---- */
typedef struct wide { long hi; unsigned long lo; } wide;
typedef struct TimeBaseRecord *TimeBase;
typedef long TimeScale;
typedef struct TimeRecord { wide value; TimeScale scale; TimeBase base; } TimeRecord;
pascal OSErr     GetMoviesError(void) M68K_INLINE(0x7003, 0xAAAA);
pascal OSErr     LoadMovieIntoRam(Movie theMovie, TimeValue time, TimeValue duration,
                                  long flags) M68K_INLINE(0x7007, 0xAAAA);
pascal TimeValue GetMovieDuration(Movie theMovie) M68K_INLINE(0x702B, 0xAAAA);
pascal void      SetMovieVolume(Movie theMovie, short volume) M68K_INLINE(0x702F, 0xAAAA);
pascal TimeValue GetMovieTime(Movie theMovie, TimeRecord *currentTime)
                                  M68K_INLINE(0x7039, 0xAAAA);
/* QuickTime 2.0's NewMovieFromFile does not open a standard MIDI file:
   mac_wads.c converts it to a movie file first. */
enum { createMovieFileDeleteCurFile = 1L << 31 };
pascal OSErr     ConvertFileToMovieFile(const FSSpec *inputFile, const FSSpec *outputFile,
                                  OSType creator, ScriptCode scriptTag, short *resID,
                                  long flags, ComponentInstance userComp, void *proc,
                                  long refCon) M68K_INLINE(0x303C, 0x1CB, 0xAAAA);

/* ---- Comm Toolbox: networking is not built in this port, but the shared
   headers still name the connection handle type. ---- */
typedef struct ConnRecord **ConnHandle;
typedef long CMFlags;
enum { cmNoErr = 0, cmRejected, cmFailed, cmTimeOut, cmNotOpen, cmNotClosed,
       cmNoRequestPending, cmNotSupported, cmNoTools, cmUserCancel };

/* ---- Device Manager / Serial Driver (Universal Interfaces spellings) ---- */
typedef ProcPtr IOCompletionUPP;
#define PBRead(pb, async)   ((async) ? PBReadAsync(pb) : PBReadSync(pb))
#define PBWrite(pb, async)  ((async) ? PBWriteAsync(pb) : PBWriteSync(pb))
#define KillIO(refNum)      Retro68KillIO(refNum)
OSErr Retro68KillIO(short refNum);          /* mac_glue.c */
#define fDTR null           /* Multiversal's SerShk calls the DTR flag "null" */
extern ConnHandle gConn;
OSErr CMIdle(ConnHandle hConn);
OSErr CMClose(ConnHandle hConn, Boolean async, void *completor, long timeout, Boolean now);

/* The DOOM sources declare their own versions of these with slightly
   different prototypes; rename them so they don't clash with newlib. */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#define strcasecmp  doom_strcasecmp
#define strncasecmp doom_strncasecmp
#define strupr      doom_strupr
int doom_strcasecmp(char *first, char *second);            /* R_DATA.C */
int doom_strncasecmp(char *first, char *second, long len);

#endif
