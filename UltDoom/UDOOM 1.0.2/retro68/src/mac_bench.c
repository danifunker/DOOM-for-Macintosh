/*
 * Frame-rate display and built-in benchmark for the Retro68 build.
 *
 * Adds to the end of the Control menu:
 *
 *     Show Frame Rate    (Cmd-F)  toggles Lion's on-screen FPS counter (also 'Q');
 *                                 runtime only, starts off (or on with -fps)
 *     Benchmark  >  All Demos (Cmd-B) / demo1 / demo2 / demo3
 *                                 plays those demos as timedemos
 *
 * A timedemo runs exactly one game tic per rendered frame (singletics) as
 * fast as the machine can go, the way "doom -timedemo demoN" does on the PC.
 * Results are shown in an alert and appended to "DOOM Benchmark Log" next
 * to the application, so runs on different machines can be compared.
 *
 * For unattended runs, put a text file named "DOOM Args" next to the
 * application containing any of:
 *
 *     -bench              run the demo1..demo3 benchmark at startup
 *     -timedemo demoN     time a single demo at startup
 *     -fps                start with the frame-rate counter on
 *     -classic            Lion's original drawers (see the Benchmark menu)
 *     -mouse              mouse control (as Options > Mouse)
 *     -quit               quit when the benchmark finishes (no alert)
 *
 * and, to start a TCP/IP network game without the dialogs:
 *
 *     -host N             host an N-player game (2-4)
 *     -join a.b.c.d[:port]  join the game hosted at that address
 *     -serial modem|printer 2-player game over a null-modem cable
 *     -appletalk N        N-player game over LocalTalk / EtherTalk
 *
 * and for WADs:
 *
 *     -file A.WAD B.WAD   add these WADs (next to the application)
 *     -musiclog           append each music lookup to "DOOM Music Log"
 *     -deathmatch / -altdeath / -nomonsters / -respawn
 *     -skill N            1-5
 *     -warp E M           episode and map (DOOM II: -warp 1 M)
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "DoomResources.h"

#include <stdio.h>
#include <time.h>

#define kBenchDemos 3

extern Boolean  gFrameRateOn;
extern Boolean  gLargeGraphics;
extern int      detailLevel;
extern int      screenblocks;
extern boolean  timingdemo;
extern boolean  singletics;
extern int      starttime;
extern int      startgametic;
extern boolean  nodrawers;
extern boolean  noblit;
extern char    *defdemoname;
extern gameaction_t gameaction;
extern boolean  advancedemo;
extern char     gTCPHostAddr[64];

#define kBenchMenuID    210
enum { iBenchAll = 1, iBenchSep, iBenchDemo1, iBenchSep2 = iBenchDemo1 + 3, iBenchClassic };

/* Lion's original assembly drawers instead of the 68040 ones (Fast68K.s),
   for timing comparisons on real machines.  -classic in DOOM Args. */
Boolean gClassicDrawers = false;
extern boolean setsizeneeded;

static short    sFrameRateItem;
static MenuHandle sBenchMenu;
static Boolean  sArgFPS;                /* -fps in DOOM Args */
static int      sDemoIndex;             /* 1-based demo being timed, 0 = idle */
static int      sTics[kBenchDemos + 1];
static int      sRealTics[kBenchDemos + 1];
static char     sDemoName[8];
static int      sFirstDemo = 1, sLastDemo = kBenchDemos;
static Boolean  sAutoBench, sAutoQuit;
static int      sNetHost;               /* -host N */
static Boolean  sNetJoin;               /* -join addr */
static int      sNetSerial;             /* -serial: 1 modem port, 2 printer port */
static int      sNetAppleTalk;          /* -appletalk N */

void MacBench_SyncMenu(void);
void M_ClearMenus(void);

static void StartDemo(int n)
{
    sprintf(sDemoName, "demo%d", n);
    sDemoIndex = n;
    nodrawers = false;
    noblit = false;
    timingdemo = true;
    singletics = true;
    defdemoname = sDemoName;
    advancedemo = false;            /* don't let the attract loop override it */
    M_ClearMenus();                 /* close DOOM's menu if it was open */
    gameaction = ga_playdemo;
}

void MacBench_InstallMenu(void)
{
    MenuHandle m = GetMHandle(mControlMenu);
    short      item;

    gFrameRateOn = sArgFPS;

    AppendMenu(m, "\p(-");
    AppendMenu(m, "\pShow Frame Rate/F");
    sFrameRateItem = CountMItems(m);

    /* Benchmark > hierarchical submenu */
    sBenchMenu = NewMenu(kBenchMenuID, "\pBenchmark");
    AppendMenu(sBenchMenu, "\pAll Demos/B;(-;demo1;demo2;demo3;(-;Lion's Original Drawers");
    InsertMenu(sBenchMenu, -1);                 /* -1: hierarchical */
    AppendMenu(m, "\pBenchmark");
    item = CountMItems(m);
    SetItemCmd(m, item, 0x1B);                  /* hMenuCmd: has a submenu */
    SetItemMark(m, item, kBenchMenuID);

    MacBench_SyncMenu();
}

void MacBench_SyncMenu(void)
{
    if (sFrameRateItem)
        SetItemMark(GetMHandle(mControlMenu), sFrameRateItem,
                    gFrameRateOn ? checkMark : noMark);
    if (sBenchMenu)
        SetItemMark(sBenchMenu, iBenchClassic, gClassicDrawers ? checkMark : noMark);
    if (sBenchMenu)
    {
        if (netgame)
            DisableItem(sBenchMenu, 0);         /* no timedemos in a net game */
        else
            EnableItem(sBenchMenu, 0);
    }
}

/* Returns true when the item belonged to us. */
Boolean MacBench_HandleMenu(short item)
{
    if (item == sFrameRateItem && item != 0)
    {
        gFrameRateOn = !gFrameRateOn;
        MacBench_SyncMenu();
        return true;
    }
    return false;
}

/* Benchmark submenu.  Returns true when the menu was ours. */
Boolean MacBench_HandleSubmenu(short menuID, short item)
{
    if (menuID != kBenchMenuID)
        return false;
    if (item == iBenchClassic)
    {
        gClassicDrawers = !gClassicDrawers;
        setsizeneeded = true;               /* picks the drawers again */
        MacBench_SyncMenu();
        return true;
    }
    if (sDemoIndex != 0 || netgame)
        return true;
    if (item == iBenchAll)
    {
        sFirstDemo = 1;
        sLastDemo = kBenchDemos;
    }
    else if (item >= iBenchDemo1 && item < iBenchDemo1 + kBenchDemos)
        sFirstDemo = sLastDemo = item - iBenchDemo1 + 1;
    else
        return true;
    StartDemo(sFirstDemo);
    return true;
}

/* fps * 10, rounded, from tics rendered over 35 Hz real tics. */
static long FPS10(int tics, int realtics)
{
    if (realtics <= 0)
        return 0;
    return ((long)tics * 350 + realtics / 2) / realtics;
}

static const char *CPUName(void)
{
    long cpu = 0, fpu = 0, arch = 0;

    /* A Power Mac: gestaltNativeCPUtype ('cput') names the chip; the 68K
       build runs there under the 68K emulator. */
    if (Gestalt('sysa', &arch) == noErr && arch == 2)
    {
        const char *chip = "PowerPC";

        Gestalt('cput', &cpu);
        switch (cpu)
        {
            case 0x101:                         chip = "PowerPC 601"; break;
            case 0x103: case 0x106: case 0x107: chip = "PowerPC 603"; break;
            case 0x104: case 0x109: case 0x10A: chip = "PowerPC 604"; break;
            case 0x108: case 0x120:             chip = "G3"; break;
            case 0x10C: case 0x110: case 0x111: case 0x112: chip = "G4"; break;
        }
#if TARGET_CPU_PPC
        return chip;
#else
        return strcmp(chip, "G4") == 0 ? "68K emulated on G4"
             : strcmp(chip, "G3") == 0 ? "68K emulated on G3" : "68K emulated on PowerPC";
#endif
    }

    /* gestaltProcessorType: 1=68000 2=68010 3=68020 4=68030 5=68040 */
    Gestalt(gestaltProcessorType, &cpu);
    Gestalt(gestaltFPUType, &fpu);
    switch (cpu)
    {
        case 3:  return fpu ? "68020+FPU" : "68020";
        case 4:  return fpu ? "68030+FPU" : "68030";
        case 5:  return fpu ? "68040" : "68LC040";
        default: return "68K";
    }
}

void MacBench_ReadArgs(void);

static void WriteLog(long totalFPS10)
{
    FILE      *f;
    time_t     now = time(NULL);
    int        i;

    f = fopen("DOOM Benchmark Log", "a");
    if (!f)
        return;
    fprintf(f, "%s", ctime(&now));
    fprintf(f, "  drawers: %s\n", gClassicDrawers ? "Lion's originals" : "68040");
    fprintf(f, "  CPU %s, %s graphics, detailLevel %d, screenblocks %d\n",
            CPUName(), gLargeGraphics ? "large" : "small",
            detailLevel, screenblocks);
    for (i = sFirstDemo; i <= sLastDemo; i++)
        if (sRealTics[i])
            fprintf(f, "  demo%d: %d gametics in %d realtics = %ld.%ld fps\n",
                    i, sTics[i], sRealTics[i],
                    FPS10(sTics[i], sRealTics[i]) / 10,
                    FPS10(sTics[i], sRealTics[i]) % 10);
    fprintf(f, "  average: %ld.%ld fps\n", totalFPS10 / 10, totalFPS10 % 10);
    /* lets a PC-sampling profile be mapped back to the link map */
    fprintf(f, "  MacBench_ReadArgs at %p\n\n", (void *)MacBench_ReadArgs);
    fclose(f);
}

static void ShowResults(void)
{
    char       line[128] = "";
    Str255     p[4];
    int        i, tics = 0, real = 0;
    long       avg;

    for (i = sFirstDemo; i <= sLastDemo; i++)
    {
        tics += sTics[i];
        real += sRealTics[i];
        sprintf(line + strlen(line), "demo%d %ld.%ld   ", i,
                FPS10(sTics[i], sRealTics[i]) / 10,
                FPS10(sTics[i], sRealTics[i]) % 10);
    }
    avg = FPS10(tics, real);
    WriteLog(avg);
    if (sAutoQuit)
        I_Quit();

    /* The alert shows ^0^1^2^3 as one text item; \r breaks the lines. */
    sprintf((char *)p[0], "Benchmark: %ld.%ld fps average (%s)\r",
            avg / 10, avg % 10, CPUName());
    sprintf((char *)p[1], "%s\r", line);
    sprintf((char *)p[2], "%d gametics in %d realtics.\r", tics, real);
    strcpy((char *)p[3], "Saved to \"DOOM Benchmark Log\".");
    for (i = 0; i < 4; i++)
        c2pstr((char *)p[i]);
    ParamText(p[0], p[1], p[2], p[3]);
    InitCursor();
    (void) Alert(rAlertErrGeneral, NULL);
    HideCursor();
}

/*
 * Called from G_CheckDemoStatus when a timed demo ends.  Returns true if it
 * started the next demo of the run.
 */
boolean MacBench_DemoDone(int tics, int realtics)
{
    if (sDemoIndex == 0)
        return false;

    sTics[sDemoIndex] = tics;
    sRealTics[sDemoIndex] = realtics;

    if (sDemoIndex < sLastDemo)
    {
        sprintf(sDemoName, "demo%d", sDemoIndex + 1);
        if (W_CheckNumForName(sDemoName) >= 0)
        {
            StartDemo(sDemoIndex + 1);
            return true;
        }
        sLastDemo = sDemoIndex;
    }

    timingdemo = false;
    singletics = false;
    sDemoIndex = 0;
    ShowResults();
    memset(sTics, 0, sizeof(sTics));
    memset(sRealTics, 0, sizeof(sRealTics));
    return false;
}

/* Reads "DOOM Args" (see top of file).  Called before the menus are set up. */
void MacBench_ReadArgs(void)
{
    FILE *f = fopen("DOOM Args", "r");
    char  word[32];

    if (!f)
        return;
    extern void MacWads_AddArgFile(const char *name);
    extern void MacWads_SetMusicLog(Boolean on);
    Boolean     inFile = false;

    while (fscanf(f, "%31s", word) == 1)
    {
        if (inFile && word[0] != '-')
        {
            MacWads_AddArgFile(word);
            continue;
        }
        inFile = false;
        if (!strcmp(word, "-file"))
            inFile = true;
        else if (!strcmp(word, "-musiclog"))
            MacWads_SetMusicLog(true);
        else if (!strcmp(word, "-fps"))
            sArgFPS = true;
        else if (!strcmp(word, "-classic"))
            gClassicDrawers = true;
        else if (!strcmp(word, "-mouse"))
        {   /* play with the mouse (Options menu: Mouse) */
            extern int useMouse, usejoystick;
            useMouse = 1;
            usejoystick = 0;
        }
        else if (!strcmp(word, "-quit"))
            sAutoQuit = true;
        else if (!strcmp(word, "-bench"))
        {
            sAutoBench = true;
            sFirstDemo = 1;
            sLastDemo = kBenchDemos;
        }
        else if (!strcmp(word, "-host") && fscanf(f, "%d", &sNetHost) == 1)
            ;
        else if (!strcmp(word, "-join") && fscanf(f, "%63s", gTCPHostAddr) == 1)
            sNetJoin = true;
        else if (!strcmp(word, "-serial") && fscanf(f, "%31s", word) == 1)
            sNetSerial = strcmp(word, "printer") ? 1 : 2;
        else if (!strcmp(word, "-appletalk") && fscanf(f, "%d", &sNetAppleTalk) == 1)
            ;
        else if (!strcmp(word, "-deathmatch"))
            deathmatch = 1;
        else if (!strcmp(word, "-altdeath"))
            deathmatch = 2;
        else if (!strcmp(word, "-nomonsters"))
            nomonsters = 1;
        else if (!strcmp(word, "-respawn"))
            respawnparm = 1;
        else if (!strcmp(word, "-skill") && fscanf(f, "%d", &startskill) == 1)
            startskill--;                       /* 1-5 on the command line */
        else if (!strcmp(word, "-warp") &&
                 fscanf(f, "%d %d", &startepisode, &startmap) == 2)
            autostart = true;
        else if (!strcmp(word, "-timedemo") && fscanf(f, "%31s", word) == 1)
        {
            int n = 0;
            if (sscanf(word, "demo%d", &n) == 1 && n >= 1 && n <= kBenchDemos)
            {
                sAutoBench = true;
                sFirstDemo = sLastDemo = n;
            }
        }
    }
    fclose(f);
}

/*
 * If "DOOM Args" asked for a network game, set it up the way the multiplayer
 * dialog would and return true so the start-up dialogs are skipped.
 */
Boolean MacBench_ApplyNetArgs(void)
{
    extern Boolean gPlayAlone, gPlayNetGame, gKeyPlayer;
    extern int     gPlayersWanted;
    extern NetType gNetType;

    extern int     gSerialPort;

    if (!sNetHost && !sNetJoin && !sNetSerial && !sNetAppleTalk)
        return false;
    gPlayAlone = false;
    gPlayNetGame = true;
    if (sNetAppleTalk)
    {   /* players find each other through NBP */
        gNetType = kAppleTalkNet;
        gPlayersWanted = sNetAppleTalk;
        return true;
    }
    if (sNetSerial)
    {   /* who is player 1 is settled by the serial handshake */
        gNetType = kSerialNet;
        gSerialPort = sNetSerial;
        gPlayersWanted = 2;
        return true;
    }
    gNetType = kTCPNet;
    gKeyPlayer = sNetHost != 0;
    gPlayersWanted = sNetHost ? sNetHost : 2;
    return true;
}

/* Called once as the main loop starts. */
void MacBench_Autostart(void)
{
    if (sAutoBench)
        StartDemo(sFirstDemo);
}
