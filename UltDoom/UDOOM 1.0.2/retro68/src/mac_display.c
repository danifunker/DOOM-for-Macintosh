/*
 * Screen resolution for large monitors.
 *
 * Lion's game draws 640x400 in the middle of the main screen.  On a big
 * display (a G4's 1280x854, say) that is a small picture, so at launch,
 * next to Lion's "set your monitor to 256 colors?" question, the player is
 * offered the smallest resolution the video driver has that is at least
 * 640x480.  At quit Lion's "reset your monitor to its previous setting?"
 * puts the resolution back too (RestoreScreenDepth, I_MAIN.C).
 *
 * The driver's resolutions come from its cscGetNextResolution status call;
 * the switch is the Display Manager 2.0's DMSetDisplayMode (Mac OS 7.6 and
 * later), so older systems are left as they are.
 */
#include "LionDoom.h"

#include <stdio.h>
#include <string.h>

/* Video driver records (Apple's Video.h); drivers expect 68K packing. */
#pragma pack(push, 2)
typedef struct {
    unsigned short  csMode;         /* depth mode */
    unsigned long   csData;         /* display mode ID */
    unsigned short  csPage;
    Ptr             csBaseAddr;
    unsigned long   csReserved;
} DispSwitchInfo;

typedef struct {
    unsigned long   csPreviousDisplayModeID;
    unsigned long   csDisplayModeID;
    unsigned long   csHorizontalPixels;
    unsigned long   csVerticalLines;
    Fixed           csRefreshRate;
    unsigned short  csMaxDepthMode;
    unsigned long   csReserved;
    unsigned long   csReserved1;
} DispResInfo;
#pragma pack(pop)

enum { kCscGetCurMode = 10, kCscGetNextResolution = 17 };
#define kModeFindFirst  0xFFFFFFFEUL
#define kModeNoMore     0xFFFFFFFDUL

/* Display Manager (Displays.h); on PowerPC these come from DisplayLib. */
pascal OSErr DMBeginConfigureDisplays(Handle *displayState)
    M68K_INLINE(0x303C, 0x0206, 0xABEB);
pascal OSErr DMEndConfigureDisplays(Handle displayState)
    M68K_INLINE(0x303C, 0x0207, 0xABEB);
pascal OSErr DMSetDisplayMode(GDHandle theDevice, unsigned long mode,
                              unsigned long *depthMode, unsigned long reserved,
                              Handle displayState)
    M68K_INLINE(0x303C, 0x0A11, 0xABEB);

extern short    gCursID;

static Boolean          sChanged;
static unsigned long    sOldMode;
static unsigned short   sOldDepthMode;

static OSErr DriverStatus(GDHandle gd, short code, void *rec)
{
    CntrlParam pb;

    memset(&pb, 0, sizeof(pb));
    pb.ioCRefNum = (**gd).gdRefNum;
    pb.csCode = code;
    *(void **)pb.csParam = rec;
    return PBStatusSync((ParmBlkPtr)&pb);
}

/* The smallest resolution of at least 640x480 that is smaller than the
   current one (the fastest refresh among equals); 0 if there is none. */
static unsigned long FindSmallMode(GDHandle gd, long curW, long curH,
                                   long *outW, long *outH)
{
    DispResInfo     r;
    unsigned long   id = kModeFindFirst, best = 0;
    long            w, h, bestW = 0, bestH = 0;
    Fixed           bestHz = 0;
    int             n;

    for (n = 0; n < 64; n++)
    {
        memset(&r, 0, sizeof(r));
        r.csPreviousDisplayModeID = id;
        if (DriverStatus(gd, kCscGetNextResolution, &r) != noErr ||
            r.csDisplayModeID == kModeNoMore || r.csDisplayModeID == id)
            break;
        id = r.csDisplayModeID;
        w = r.csHorizontalPixels;
        h = r.csVerticalLines;
        if (w < 640 || h < 480 || w * h >= curW * curH)
            continue;
        if (!best || w * h < bestW * bestH ||
            (w * h == bestW * bestH && r.csRefreshRate > bestHz))
        {
            best = id;
            bestW = w;
            bestH = h;
            bestHz = r.csRefreshRate;
        }
    }
    *outW = bestW;
    *outH = bestH;
    return best;
}

static OSErr SetMode(GDHandle gd, unsigned long mode, unsigned long depthMode)
{
    Handle  state = NULL;
    OSErr   err;

    if (DMBeginConfigureDisplays(&state) != noErr)
        return -1;
    err = DMSetDisplayMode(gd, mode, &depthMode, 0, state);
    DMEndConfigureDisplays(state);
    return err;
}

/* MakeDoomWindow, after the colour-depth question. */
void MacDisplay_OfferLowRes(GDHandle gd)
{
    DispSwitchInfo  cur;
    Rect            r;
    long            dm = 0, curW, curH, w, h;
    unsigned long   mode;
    char            msg[256];

    if (!gd)
        return;
    r = (**gd).gdRect;
    curW = r.right - r.left;
    curH = r.bottom - r.top;
    if (curW <= 640 && curH <= 480)
        return;
    if (Gestalt('dplv', &dm) != noErr || dm < 0x00020000L)    /* Display Manager 2.0 */
        return;
    memset(&cur, 0, sizeof(cur));
    if (DriverStatus(gd, kCscGetCurMode, &cur) != noErr)
        return;
    mode = FindSmallMode(gd, curW, curH, &w, &h);
    if (!mode)
        return;

    sprintf(msg, "Your monitor is set to %ld x %ld. Would you like to switch it to "
            "%ld x %ld while you play, so DOOM fills more of the screen?",
            curW, curH, w, h);
    c2pstr(msg);
    ParamText((unsigned char *)msg, "\p", "\p", "\p");
    InitCursor();
    if (Alert(600, NULL) == 1 && SetMode(gd, mode, cur.csMode) == noErr)
    {
        sChanged = true;
        sOldMode = cur.csData;
        sOldDepthMode = cur.csMode;
        if ((**(**gd).gdPMap).pixelSize != 8)      /* the depth didn't carry over */
            SetDepth(gd, 8, 1 << gdDevType, 1);
    }
    gCursID = -2;
    HideCursor();
}

Boolean MacDisplay_Changed(void)
{
    return sChanged;
}

/* RestoreScreenDepth: back to the resolution the player had. */
void MacDisplay_Restore(GDHandle gd)
{
    if (!sChanged || !gd)
        return;
    SetMode(gd, sOldMode, sOldDepthMode);
    sChanged = false;
}
