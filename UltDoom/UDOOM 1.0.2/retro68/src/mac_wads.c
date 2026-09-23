/*
 * WAD handling for the Retro68 build.
 *
 *  - WADs menu: "Add WAD File...", "Remove Added WADs" and the list of WADs
 *    in use.  Changing the list reloads every WAD without quitting: the zone
 *    is emptied and the same initialisers D_DoomMain runs at launch are run
 *    again, then the game returns to the title screen.
 *  - "-file A.WAD B.WAD ..." in DOOM Args adds WADs next to the application.
 *  - Music is looked up per WAD:
 *        :MIDI:<last added WAD>:<track>.MID!   ... down to
 *        :MIDI:<base WAD>:<track>.MID!           (e.g. :MIDI:DOOM2:RUNNIN.MID!)
 *        :MIDI:DOOM1:<track>.MID!                (for DOOM.WAD: episode 1 is shared)
 *        :Music:<track>.MID!                     (the original layout)
 *    Each name is tried as ".MID!" (QuickTime MIDI movie, as Lion shipped)
 *    and as ".MID" (standard MIDI file, which QuickTime imports).
 *  - A signature of the WAD list (CRC-32 of each file's contents, in load
 *    order), compared by the TCP/IP lobby.
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "DoomResources.h"
#include "sounds.h"
#include "soundst.h"

#include <stdio.h>
#include <string.h>

#define kWadsMenuID     212
#define kMaxWadFiles    MAXWADFILES     /* DOOMDEF.H */

enum { iWadAdd = 1, iWadRemove, iWadSep, iWadFirst };

extern FSSpec       gWadFiles[kMaxWadFiles];
extern int          gNumWads;
extern short        gAppVRefNum;
extern long         gAppDirId;
extern void       **lumpcache;
extern int          gNumChannels;
extern boolean      timingdemo, singletics, demoplayback, netdemo, usergame;

Boolean GetWadFile(FSSpec *fsSpec);
Boolean UpdateBaseWad(char *theWad);
Boolean CompPStr(void *strA, void *strB);
void    BlackScreen(void);
void    RedrawScreen(void);
void    StatusParamText(char *one, char *two, char *three, char *four);
void    StatusDialog(long total, long current);
void    DrawStatusDialog(Boolean forUpdate);
void    CloseStatusDialog(void);
void    S_StopChannel(int cnum);
void    M_Init(void);
void    R_Init(void);
void    P_Init(void);
void    HU_Init(void);
void    ST_Init(void);
void    D_StartTitle(void);
void    W_InitMultipleFiles(void);
void    Z_Reset(void);

static MenuHandle sMenu;
static Boolean    sReloadPending;
static char       sArgFiles[kMaxWadFiles][64];
static int        sNumArgFiles;
static Boolean    sMusicLog;

/* ------------------------------------------------------------------------ */

/* "MYMAP.WAD" -> "MYMAP" (C string) */
static void WadFolderName(const unsigned char *pname, char *out)
{
    int n = pname[0];

    memcpy(out, pname + 1, n);
    out[n] = 0;
    if (n > 4 && out[n - 4] == '.' &&
        (out[n - 3] | 0x20) == 'w' && (out[n - 2] | 0x20) == 'a' && (out[n - 1] | 0x20) == 'd')
        out[n - 4] = 0;
}

static Boolean AlreadyLoaded(const FSSpec *spec)
{
    int i;

    for (i = 0; i < gNumWads; i++)
        if (gWadFiles[i].vRefNum == spec->vRefNum && gWadFiles[i].parID == spec->parID &&
            CompPStr((void *)gWadFiles[i].name, (void *)spec->name))
            return true;
    return false;
}

/* Base WADs replace entry 0; others are appended.  Returns false if full. */
static Boolean AddWad(const FSSpec *spec)
{
    if (UpdateBaseWad((char *)spec->name))
    {
        gWadFiles[0] = *spec;
        return true;
    }
    if (AlreadyLoaded(spec))
        return true;
    if (gNumWads >= kMaxWadFiles)
        return false;
    gWadFiles[gNumWads++] = *spec;
    return true;
}

static void Alert1(const char *msg)
{
    Str255 s;

    strcpy((char *)s, msg);
    c2pstr((char *)s);
    ParamText(s, "\p", "\p", "\p");
    InitCursor();
    (void) Alert(rAlertErrGeneral, NULL);
}

/* ------------------------------------------------------------------------ */
/* Menu                                                                     */

static void RebuildList(void)
{
    int  i;
    char name[64];

    if (!sMenu)
        return;
    while (CountMItems(sMenu) >= iWadFirst)
        DeleteMenuItem(sMenu, iWadFirst);
    for (i = 0; i < gNumWads; i++)
    {
        Str255 s;

        memcpy(name, gWadFiles[i].name + 1, gWadFiles[i].name[0]);
        name[gWadFiles[i].name[0]] = 0;
        sprintf((char *)s, "%s%s", name, i == 0 ? "  (base)" : "");
        c2pstr((char *)s);
        AppendMenu(sMenu, "\px");
        SetMenuItemText(sMenu, iWadFirst + i, s);  /* no metacharacter parsing */
        DisableItem(sMenu, iWadFirst + i);
    }
    if (gNumWads > 1 && !netgame)
        EnableItem(sMenu, iWadRemove);
    else
        DisableItem(sMenu, iWadRemove);
    if (netgame)
        DisableItem(sMenu, iWadAdd);
    else
        EnableItem(sMenu, iWadAdd);
}

void MacWads_InstallMenu(void)
{
    sMenu = NewMenu(kWadsMenuID, "\pWADs");
    if (!sMenu)
        return;
    AppendMenu(sMenu, "\px;x;(-");
    SetMenuItemText(sMenu, iWadAdd, "\pAdd WAD File\311");
    SetMenuItemText(sMenu, iWadRemove, "\pRemove Added WADs");
    RebuildList();
    InsertMenu(sMenu, 0);
    DrawMenuBar();
}

void MacWads_SyncMenu(void)
{
    RebuildList();
}

void MacWads_RequestReload(void)
{
    sReloadPending = true;
}

Boolean MacWads_HandleMenu(short menuID, short item)
{
    FSSpec spec;

    if (menuID != kWadsMenuID)
        return false;
    if (netgame)
        return true;
    if (item == iWadAdd)
    {
        InitCursor();
        if (GetWadFile(&spec))
        {
            if (!AddWad(&spec))
                Alert1("The maximum number of WAD files has already been added.");
            else
                sReloadPending = true;
        }
        HideCursor();
        RedrawScreen();
    }
    else if (item == iWadRemove && gNumWads > 1)
    {
        gNumWads = 1;
        sReloadPending = true;
    }
    return true;
}

/* ------------------------------------------------------------------------ */
/* Reload                                                                   */

/* Close every WAD file W_AddFile opened.  Lumps are stored file by file,
   so each file's refnum starts a new run. */
static void CloseWads(void)
{
    int i;

    for (i = 0; i < numlumps; i++)
        if (i == 0 || lumpinfo[i].handle != lumpinfo[i - 1].handle)
            FSClose(lumpinfo[i].handle);
    DisposePtr((Ptr)lumpinfo);
    DisposePtr((Ptr)lumpcache);
    lumpinfo = NULL;
    lumpcache = NULL;
    numlumps = 0;
}

static void Reload(void)
{
    int i;

    if (shareware && gNumWads > 1)
    {
        Alert1("The Shareware version of DOOM does not support multiple WAD files. "
               "Upgrade to the Registered version today!");
        gNumWads = 1;
    }

    /* leave whatever is running */
    timingdemo = false;
    singletics = false;
    demoplayback = false;
    netdemo = false;
    usergame = false;
    paused = false;
    M_ClearMenus();
    S_StopMusic();
    for (i = 0; i < gNumChannels; i++)
        S_StopChannel(i);

    BlackScreen();
    StatusParamText("\pLoading WAD files...", "\p", "\p", "\p");
    StatusDialog(80, 1);
    DrawStatusDialog(TRUE);

    CloseWads();
    Z_Reset();
    for (i = 1; i < NUMSFX; i++)
    {   /* cached sound lumps lived in the zone; lump numbers may change */
        S_sfx[i].data = NULL;
        S_sfx[i].lumpnum = -1;
        S_sfx[i].usefulness = -1;           /* -1: not cached (as S_Init sets it) */
    }
    UpdateBaseWad((char *)gWadFiles[0].name);   /* sets shareware/registered/commercial */
    D_SetGameMission();

    W_InitMultipleFiles();
    StatusDialog(80, 20);
    M_Init();
    R_Init();
    StatusDialog(80, 60);
    P_Init();
    HU_Init();
    ST_Init();
    StatusDialog(80, 80);

    CloseStatusDialog();                /* take the progress bar down */
    BlackScreen();
    RedrawScreen();
    RebuildList();
    D_StartTitle();
}

/* Called at the top of every main-loop pass. */
void MacWads_Poll(void)
{
    if (!sReloadPending)
        return;
    sReloadPending = false;
    if (!netgame)
        Reload();
}

/* ------------------------------------------------------------------------ */
/* DOOM Args                                                                */

/* "-file" takes names until the next "-option"; called by MacBench_ReadArgs. */
void MacWads_AddArgFile(const char *name)
{
    if (sNumArgFiles < kMaxWadFiles)
        strncpy(sArgFiles[sNumArgFiles++], name, 63);
}

void MacWads_SetMusicLog(Boolean on)
{
    sMusicLog = on;
    if (on)
    {   /* lets a crash address be mapped back to the link map */
        FILE *f = fopen("DOOM Music Log", "a");
        if (f)
        {
            fprintf(f, "MacWads_SetMusicLog at %p\n", (void *)MacWads_SetMusicLog);
            fclose(f);
        }
    }
}

/* After the base WAD has been found: add the -file WADs. */
void MacWads_ApplyArgs(void)
{
    int    i;
    FSSpec spec;
    Str255 pname;

    for (i = 0; i < sNumArgFiles; i++)
    {
        strcpy((char *)pname, sArgFiles[i]);
        c2pstr((char *)pname);
        if (FSMakeFSSpec(gAppVRefNum, gAppDirId, pname, &spec) == noErr)
            AddWad(&spec);
    }
}

/* ------------------------------------------------------------------------ */
/* Music                                                                    */

static Boolean TryMusic(const char *folder, const char *track, FSSpec *out)
{
    static const char *ext[2] = { ".MID!", ".MID" };
    char   path[128];
    FInfo  fi;
    int    e;

    for (e = 0; e < 2; e++)
    {
        if (folder)
            sprintf(path, ":MIDI:%s:%s%s", folder, track, ext[e]);
        else
            sprintf(path, ":Music:%s%s", track, ext[e]);
        c2pstr(path);
        if (FSMakeFSSpec(gAppVRefNum, gAppDirId, (unsigned char *)path, out) != noErr)
            continue;
        /* a .MID copied from a PC has no type; QuickTime imports 'Midi' files */
        if (e == 1 && FSpGetFInfo(out, &fi) == noErr && fi.fdType != 'Midi')
        {
            fi.fdType = 'Midi';
            fi.fdCreator = 'TVOD';
            FSpSetFInfo(out, &fi);
        }
        if (sMusicLog)
        {
            FILE *f = fopen("DOOM Music Log", "a");
            if (f)
            {
                p2cstr((unsigned char *)path);
                fprintf(f, "%s -> %s\n", track, path);
                fclose(f);
            }
        }
        return true;
    }
    return false;
}

/* Finds the music file for a track name such as "e1m1" or "runnin". */
Boolean MacWads_FindMusic(const char *track, FSSpec *out)
{
    char folder[64];
    int  i;

    for (i = gNumWads - 1; i >= 0; i--)
    {
        WadFolderName(gWadFiles[i].name, folder);
        if (TryMusic(folder, track, out))
            return true;
    }
    if (registered && TryMusic("DOOM1", track, out))
        return true;
    if (TryMusic(NULL, track, out))
        return true;
    if (sMusicLog)
    {
        FILE *f = fopen("DOOM Music Log", "a");
        if (f)
        {
            fprintf(f, "%s -> not found\n", track);
            fclose(f);
        }
    }
    return false;
}

/* ------------------------------------------------------------------------ */
/* Network check                                                            */

/* CRC-32 (IEEE) of each WAD's whole data fork, cached per file for the
   session and recomputed if the file's size or modification date changes. */
static unsigned long sCRCTable[256];

typedef struct {
    FSSpec        spec;
    long          size;
    unsigned long modDate;
    unsigned long crc;
} WadCRC;

static WadCRC sCRCCache[16];
static int    sCRCCached;

static unsigned long FileCRC(const FSSpec *spec, long size, unsigned long modDate)
{
    static char   buf[16384];
    unsigned long crc = 0xFFFFFFFFUL;
    short         ref;
    long          n;
    int           i;

    for (i = 0; i < sCRCCached; i++)
        if (sCRCCache[i].spec.vRefNum == spec->vRefNum &&
            sCRCCache[i].spec.parID == spec->parID &&
            CompPStr((void *)sCRCCache[i].spec.name, (void *)spec->name) &&
            sCRCCache[i].size == size && sCRCCache[i].modDate == modDate)
            return sCRCCache[i].crc;

    if (!sCRCTable[1])
    {
        unsigned long c;
        int           k;

        for (i = 0; i < 256; i++)
        {
            for (c = i, k = 0; k < 8; k++)
                c = (c & 1) ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
            sCRCTable[i] = c;
        }
    }

    if (FSpOpenDF(spec, fsRdPerm, &ref) != noErr)
        return 0;
    do
    {
        unsigned char *p = (unsigned char *)buf;

        n = sizeof buf;
        if (FSRead(ref, &n, buf) != noErr && n == 0)
            break;
        for (i = 0; i < n; i++)
            crc = sCRCTable[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    } while (n == sizeof buf);
    FSClose(ref);
    crc ^= 0xFFFFFFFFUL;

    if (sCRCCached < 16)
    {
        sCRCCache[sCRCCached].spec = *spec;
        sCRCCache[sCRCCached].size = size;
        sCRCCache[sCRCCached].modDate = modDate;
        sCRCCache[sCRCCached].crc = crc;
        sCRCCached++;
    }
    return crc;
}

static unsigned long WadCRCAt(int i)
{
    CInfoPBRec pb;

    memset(&pb, 0, sizeof pb);
    pb.hFileInfo.ioNamePtr = gWadFiles[i].name;
    pb.hFileInfo.ioVRefNum = gWadFiles[i].vRefNum;
    pb.hFileInfo.ioDirID = gWadFiles[i].parID;
    if (PBGetCatInfoSync(&pb) != noErr)
        return 0;
    return FileCRC(&gWadFiles[i], pb.hFileInfo.ioFlLgLen, pb.hFileInfo.ioFlMdDat);
}

/* Combined signature of the WAD list: every file's contents, in load order.
   Works before the WADs are read (the lobby runs first at launch). */
unsigned long MacWads_Signature(void)
{
    unsigned long h = 5381;
    int           i;

    for (i = 0; i < gNumWads; i++)
        h = h * 33 + WadCRCAt(i);
    return h ^ (unsigned long)gNumWads;
}

/* "DOOM2.WAD (1A2B3C4D) + MYMAP.WAD (0F1E2D3C)" */
void MacWads_Describe(char *out, int max)
{
    char one[48];
    int  i, n;

    out[0] = 0;
    for (i = 0; i < gNumWads; i++)
    {
        n = gWadFiles[i].name[0];
        if (n > 31)
            n = 31;
        memcpy(one, gWadFiles[i].name + 1, n);
        sprintf(one + n, " (%08lX)", WadCRCAt(i));
        if (strlen(out) + strlen(one) + 4 >= (size_t)max)
            break;
        if (i)
            strcat(out, " + ");
        strcat(out, one);
    }
}
