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
void    W_CloseWadFiles(void);

void    M_ClearMenus(void);
static MenuHandle sMenu;
static Boolean    sReloadPending;
static char       sArgFiles[kMaxWadFiles][64];
static int        sNumArgFiles;
static Boolean    sMusicLog;
void MacWads_PrepareMusic(long progFrom, long progTo);

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
/* Boom / MBF detection                                                     */

/*
 * This engine plays vanilla and limit-removing levels.  Levels made for
 * Boom or MBF use line and sector specials it doesn't know (doors, lifts
 * and exits that do nothing), so say so once when such a WAD is loaded.
 */
static int CountBoomSpecials(int lump, int recsize, int specoffset, int vanillamax)
{
    const unsigned char *p, *base;
    int                  len = W_LumpLength(lump), n = 0;

    if (len < recsize)
        return 0;
    base = p = W_CacheLumpNum(lump, PU_STATIC);
    for (; p + recsize <= base + len; p += recsize)
    {
        int special = p[specoffset] | (p[specoffset + 1] << 8);

        if (special > vanillamax)
            n++;
    }
    Z_Free((void *)base);
    return n;
}

void MacWads_CheckFeatures(void)
{
    int  i, lines = 0, sectors = 0, maps = 0;
    int  iwadhandle = numlumps ? lumpinfo[0].handle : -1;
    char msg[256];

    for (i = 0; i + 8 < numlumps; i++)
    {
        if (lumpinfo[i].handle == iwadhandle)
            continue;                           /* only PWAD levels */
        if (strncasecmp(lumpinfo[i + 2].name, "LINEDEFS", 8) ||
            strncasecmp(lumpinfo[i + 8].name, "SECTORS", 8))
            continue;
        maps++;
        lines += CountBoomSpecials(i + 2, 14, 6, 141);    /* DOOM II's last */
        sectors += CountBoomSpecials(i + 8, 26, 22, 17);
    }
    if (lines + sectors == 0)
        return;
    sprintf(msg, "These WADs were made for Boom or MBF: %d line and %d sector "
            "specials in %d level%s are not supported by this version of DOOM. "
            "Some doors, lifts, exits or effects will not work.",
            lines, sectors, maps, maps == 1 ? "" : "s");
    Alert1(msg);
}

/* ------------------------------------------------------------------------ */
/* Reload                                                                   */

/* Close every WAD file W_AddFile opened.  Lumps are stored file by file,
   so each file's refnum starts a new run. */
static void CloseWads(void)
{
    W_CloseWadFiles();
    DisposePtr((Ptr)lumpinfo);
    DisposePtr((Ptr)lumpcache);
    lumpinfo = NULL;
    lumpcache = NULL;
    numlumps = 0;
}

static void Reload(void)
{
    int i;

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
    {   /* limit-removal arrays lived in the zone too */
        extern void P_ForgetZoneArrays(void);
        P_ForgetZoneArrays();
    }
    for (i = 1; i < NUMSFX; i++)
    {   /* cached sound lumps lived in the zone; lump numbers may change */
        S_sfx[i].data = NULL;
        S_sfx[i].lumpnum = -1;
        S_sfx[i].usefulness = -1;           /* -1: not cached (as S_Init sets it) */
    }
    UpdateBaseWad((char *)gWadFiles[0].name);   /* sets shareware/registered/commercial */
    D_SetGameMission();

    W_InitMultipleFiles();
    DEH_Init();
    UMI_Init();
    StatusDialog(80, 20);
    M_Init();
    R_Init();
    {   /* I_SetPalette maps the colormaps to the Mac palette (black and white
           swapped) once; R_Init has just loaded them again */
        extern short remapped;
        remapped = 0;
        I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
    }
    StatusDialog(80, 60);
    P_Init();
    HU_Init();
    ST_Init();
    MacWads_PrepareMusic(60, 79);       /* all the new WADs' music, now */
    StatusDialog(80, 80);

    CloseStatusDialog();                /* take the progress bar down */
    BlackScreen();
    RedrawScreen();
    RebuildList();
    MacWads_CheckFeatures();
    HideCursor();
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

/* Appends one line to "DOOM Music Log" next to the application.  Retro68's
   fopen(..., "a") lost every line after the first few, so this uses the
   File Manager directly and flushes the volume after each line. */
static void MusicLogLine(const char *text)
{
    FSSpec spec;
    short  ref;
    long   len;
    char   buf[256];
    OSErr  err;

    err = FSMakeFSSpec(gAppVRefNum, gAppDirId, "\pDOOM Music Log", &spec);
    if (err == fnfErr)
        err = FSpCreate(&spec, 'ttxt', 'TEXT', smSystemScript);
    if (err != noErr || FSpOpenDF(&spec, fsRdWrPerm, &ref) != noErr)
        return;
    len = strlen(text);
    if (len > (long)sizeof(buf) - 1)
        len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len++] = '\r';
    SetFPos(ref, fsFromLEOF, 0);
    FSWrite(ref, &len, buf);
    FSClose(ref);
    FlushVol(NULL, gAppVRefNum);
}

void MacWads_SetMusicLog(Boolean on)
{
    char line[64];

    sMusicLog = on;
    if (on)
    {   /* lets a crash address be mapped back to the link map */
        sprintf(line, "MacWads_SetMusicLog at %p", (void *)MacWads_SetMusicLog);
        MusicLogLine(line);
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
        if (e == 0)
        {   /* an empty movie is what an interrupted conversion leaves */
            short ref;
            long  eof = 0;

            if (FSpOpenDF(out, fsRdPerm, &ref) == noErr)
            {
                GetEOF(ref, &eof);
                FSClose(ref);
            }
            if (eof == 0)
            {
                FSpDelete(out);
                continue;
            }
        }
        /* a .MID copied from a PC has no type; QuickTime imports 'Midi' files */
        if (e == 1 && FSpGetFInfo(out, &fi) == noErr && fi.fdType != 'Midi')
        {
            fi.fdType = 'Midi';
            fi.fdCreator = 'TVOD';
            FSpSetFInfo(out, &fi);
        }
        if (sMusicLog)
        {
            char line[160];

            p2cstr((unsigned char *)path);
            sprintf(line, "%s -> %s", track, path);
            MusicLogLine(line);
        }
        return true;
    }
    return false;
}

/* ---- Music from the WADs' own lumps ---- */

unsigned char *MUS2MID(const unsigned char *mus, long muslen, long *midlen);
unsigned char *MIDI_Fit(const unsigned char *mid, long len, long *midlen);
int  W_LumpFileIndex(int lump);

static void LogMusic(const char *track, const char *what)
{
    char line[160];

    if (sMusicLog)
    {
        sprintf(line, "%.9s -> %.140s", track, what);
        MusicLogLine(line);
    }
}

/* S_ChangeMusic reports what QuickTime did with a track (-musiclog). */
void MacWads_LogMusicErr(const char *track, const char *what, long err)
{
    char msg[96];

    if (err)
        sprintf(msg, "%s (error %ld)", what, err);
    else
        strcpy(msg, what);
    LogMusic(track, msg);
}

/* S_ChangeMusic: QuickTime 2.0's NewMovieFromFile can't open a standard
   MIDI file, so a 'Midi' file is converted once into a QuickTime movie
   next to it (<track>.MID!, which the lookup prefers from then on).
   The MIDI import comes with QuickTime Musical Instruments, which also
   holds the instruments the music is played with. */
static Boolean WriteMidi(const FSSpec *spec, const void *data, long len);

/* A MIDI file with more events than QuickTime 2.x's import survives (made
   by an older build, or copied in) is rewritten to fit first. */
static void FitMidiFile(FSSpec *spec, const char *track)
{
    short          ref;
    long           len = 0, fitlen;
    unsigned char *data, *fit;

    if (FSpOpenDF(spec, fsRdPerm, &ref) != noErr)
        return;
    GetEOF(ref, &len);
    data = len > 0 ? (unsigned char *) malloc(len) : NULL;
    if (data && FSRead(ref, &len, data) != noErr)
    {
        free(data);
        data = NULL;
    }
    FSClose(ref);
    if (!data)
        return;
    fit = MIDI_Fit(data, len, &fitlen);
    free(data);
    if (fit)
    {
        LogMusic(track, "MIDI file thinned out for QuickTime");
        WriteMidi(spec, fit, fitlen);
        free(fit);
    }
}

Boolean MacWads_MidiToMovie(FSSpec *spec, const char *track)
{
    FInfo  fi;
    FSSpec movie;
    OSErr  err;

    if (FSpGetFInfo(spec, &fi) != noErr || fi.fdType != 'Midi')
        return true;                        /* a movie already */
    FitMidiFile(spec, track);
    movie = *spec;
    if (movie.name[0] > 30)
        return false;
    movie.name[++movie.name[0]] = '!';
    err = ConvertFileToMovieFile(spec, &movie, 'TVOD', smSystemScript, NULL,
                                 createMovieFileDeleteCurFile, NULL, NULL, 0);
    if (err != noErr)
    {
        FSpDelete(&movie);
        MacWads_LogMusicErr(track, "QuickTime could not convert the MIDI file "
                            "(QuickTime Musical Instruments installed?)", err);
        return false;
    }
    FlushVol(NULL, movie.vRefNum);
    LogMusic(track, "converted the MIDI file to a QuickTime movie");
    *spec = movie;
    return true;
}

/* Writes a file of type 'Midi' (QuickTime imports those). */
static Boolean WriteMidi(const FSSpec *spec, const void *data, long len)
{
    short ref;
    long  n = len;
    OSErr err;

    FSpDelete(spec);
    if (FSpCreate(spec, 'TVOD', 'Midi', smSystemScript) != noErr)
        return false;
    if (FSpOpenDF(spec, fsRdWrPerm, &ref) != noErr)
        return false;
    err = FSWrite(ref, &n, data);
    SetEOF(ref, n);
    FSClose(ref);
    FlushVol(NULL, spec->vRefNum);      /* keep it if the Mac crashes later */
    if (err != noErr)
    {
        FSpDelete(spec);
        return false;
    }
    return true;
}

/*
 * Converts music lump "lump" to MIDI and saves it as :MIDI:<folder>:<TRACK>.MID
 * next to the application, where it is found directly next time (and can be
 * replaced by a better file of the same name).  If that disk can't be
 * written, the Temporary Items folder is used.
 */
static Boolean sLookupOnly;             /* MacWads_PrepareMusic's first pass */

static Boolean ConvertMusic(int lump, const char *folder, const char *track, FSSpec *out)
{
    const unsigned char *data;
    unsigned char       *mid;
    long                 len, midlen;
    Boolean              ok = false;
    char                 name[64];
    long                 dirID;
    short                vref;
    int                  i;

    if (sLookupOnly)
        return false;
    len = W_LumpLength(lump);
    data = W_CacheLumpNum(lump, PU_STATIC);
    if (len >= 4 && !memcmp(data, "MThd", 4))
    {   /* some WADs already hold standard MIDI files: kept as they are
           unless QuickTime can't take that many events (mus2mid.c) */
        mid = MIDI_Fit(data, len, &midlen);
        if (!mid && (mid = (unsigned char *) malloc(len)) != NULL)
        {
            memcpy(mid, data, len);
            midlen = len;
        }
    }
    else
        mid = MUS2MID(data, len, &midlen);
    Z_Free((void *)data);
    if (!mid)
    {
        LogMusic(track, "lump is not MUS or MIDI");
        return false;
    }

    sprintf(name, "%s.MID", track);
    for (i = 0; name[i]; i++)
        name[i] = toupper(name[i]);
    c2pstr(name);

    /* :MIDI:<folder>: next to the application, created if need be */
    {
        CInfoPBRec pb;
        Str255     p;
        long       midiDir;

        memset(&pb, 0, sizeof pb);
        strcpy((char *)p, "MIDI");
        c2pstr((char *)p);
        if (DirCreate(gAppVRefNum, gAppDirId, p, &midiDir) != noErr)
        {
            pb.dirInfo.ioNamePtr = p;
            pb.dirInfo.ioVRefNum = gAppVRefNum;
            pb.dirInfo.ioDrDirID = gAppDirId;
            midiDir = (PBGetCatInfoSync(&pb) == noErr) ? pb.dirInfo.ioDrDirID : 0;
        }
        if (midiDir)
        {
            strcpy((char *)p, folder);
            c2pstr((char *)p);
            dirID = 0;
            if (DirCreate(gAppVRefNum, midiDir, p, &dirID) != noErr)
            {
                memset(&pb, 0, sizeof pb);
                pb.dirInfo.ioNamePtr = p;
                pb.dirInfo.ioVRefNum = gAppVRefNum;
                pb.dirInfo.ioDrDirID = midiDir;
                dirID = (PBGetCatInfoSync(&pb) == noErr) ? pb.dirInfo.ioDrDirID : 0;
            }
            if (dirID && FSMakeFSSpec(gAppVRefNum, dirID, (unsigned char *)name, out) != nsvErr)
                ok = WriteMidi(out, mid, midlen);
            if (ok)
                LogMusic(track, "saved in the MIDI folder");
        }
    }
    if (!ok && FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder,
                          &vref, &dirID) == noErr &&
        FSMakeFSSpec(vref, dirID, (unsigned char *)name, out) != nsvErr)
        ok = WriteMidi(out, mid, midlen);
    free(mid);
    LogMusic(track, ok ? "converted from the WAD" : "could not write the MIDI file");
    return ok;
}

/*
 * Finds the music file for a track name such as "e1m1" or "runnin":
 *
 *   1. :MIDI:<WAD>: folders of the WADs added after (or with) the WAD whose
 *      D_<track> lump is used, newest first;
 *   2. that WAD's lump, when it's an added WAD: converted to MIDI;
 *   3. the base WAD's folder, :MIDI:DOOM1: (registered DOOM's episode 1)
 *      and the original :Music: folder;
 *   4. the base WAD's lump, converted.
 */
Boolean MacWads_FindMusic(const char *track, FSSpec *out)
{
    char folder[64], lumpname[16];
    int  i, lump, from;

    sprintf(lumpname, "D_%s", track);
    lump = W_CheckNumForName(lumpname);
    from = (lump >= 0) ? W_LumpFileIndex(lump) : -1;

    for (i = gNumWads - 1; i >= 1 && i >= from; i--)
    {
        WadFolderName(gWadFiles[i].name, folder);
        if (TryMusic(folder, track, out))
            return true;
    }
    if (from >= 1)
    {
        WadFolderName(gWadFiles[from].name, folder);
        if (ConvertMusic(lump, folder, track, out))
            return true;
    }
    for (i = (from >= 1 ? from - 1 : 0); i >= 0; i--)
    {
        WadFolderName(gWadFiles[i].name, folder);
        if (TryMusic(folder, track, out))
            return true;
    }
    if (registered && TryMusic("DOOM1", track, out))
        return true;
    if (TryMusic(NULL, track, out))
        return true;
    if (lump >= 0)
    {
        WadFolderName(gWadFiles[0].name, folder);
        if (ConvertMusic(lump, folder, track, out))
            return true;
    }
    if (!sLookupOnly)
        LogMusic(track, "not found");
    return false;
}

/*
 * Converts every track of the loaded WADs up front (MacWads_Reload and
 * D_DoomMain, while the loading status window is up), so nothing is
 * converted during play: MUS lump -> :MIDI:<WAD>:<TRACK>.MID -> .MID!
 * QuickTime movie.  Tracks that already have a movie are skipped, so after
 * the first time this only looks the files up.  Needs QuickTime (music on);
 * S_ChangeMusic still converts on demand otherwise.
 */
void StatusDialog(long total, long current);
void DrawStatusDialog(Boolean forUpdate);
void StatusParamText(char *one, char *two, char *three, char *four);

static Boolean IsMusicLump(int i, char *track)
{
    char name[9];
    int  k;

    if (lumpinfo[i].name[0] != 'D' || lumpinfo[i].name[1] != '_')
        return false;
    memcpy(name, lumpinfo[i].name, 8);
    name[8] = 0;
    if (W_CheckNumForName(name) != i)       /* a later WAD replaces it */
        return false;
    for (k = 0; name[k + 2]; k++)
        track[k] = tolower(name[k + 2]);
    track[k] = 0;
    return k > 0;
}

static Boolean HasMovie(const char *track)
{
    FSSpec spec;
    FInfo  fi;
    Boolean found;

    sLookupOnly = true;
    found = MacWads_FindMusic(track, &spec);
    sLookupOnly = false;
    return found && FSpGetFInfo(&spec, &fi) == noErr && fi.fdType != 'Midi';
}

void MacWads_PrepareMusic(long progFrom, long progTo)
{
    extern Boolean gQuickTimeLoaded;
    static char   *todo;
    char           track[9], msg[80];
    FSSpec         spec;
    int            i, total = 0, done = 0;

    if (!gQuickTimeLoaded || !numlumps)
        return;
    todo = (char *) realloc(todo, numlumps);
    if (!todo)
        return;
    for (i = 0; i < numlumps; i++)
    {
        todo[i] = IsMusicLump(i, track) && !HasMovie(track);
        total += todo[i];
    }
    for (i = 0; i < numlumps; i++)
    {
        if (!todo[i] || !IsMusicLump(i, track))
            continue;
        done++;
        sprintf(msg, "Generating MIDI files from WAD... (%d of %d)", done, total);
        c2pstr(msg);
        StatusParamText(msg, "\p", "\p", "\p");
        DrawStatusDialog(TRUE);
        StatusDialog(80, progFrom + (progTo - progFrom) * done / total);
        if (MacWads_FindMusic(track, &spec))
            MacWads_MidiToMovie(&spec, track);
    }
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
