/*
 * Frame-rate display and built-in benchmark for the Retro68 build.
 *
 * Adds two items (after a separator) to the end of the Control menu:
 *
 *     Show Frame Rate    (Cmd-F)  toggles Lion's on-screen FPS counter (also 'Q')
 *     Run Benchmark      (Cmd-B)  plays demo1..demo3 as timedemos
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
 *     -quit               quit when the benchmark finishes (no alert)
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

static short    sFrameRateItem;
static short    sBenchItem;
static int      sDemoIndex;             /* 1-based demo being timed, 0 = idle */
static int      sTics[kBenchDemos + 1];
static int      sRealTics[kBenchDemos + 1];
static char     sDemoName[8];
static int      sFirstDemo = 1, sLastDemo = kBenchDemos;
static Boolean  sAutoBench, sAutoQuit;

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
    gameaction = ga_playdemo;
}

void MacBench_InstallMenu(void)
{
    MenuHandle m = GetMHandle(mControlMenu);

    AppendMenu(m, "\p(-");
    AppendMenu(m, "\pShow Frame Rate/F");
    sFrameRateItem = CountMItems(m);
    AppendMenu(m, "\pRun Benchmark (timedemo)/B");
    sBenchItem = CountMItems(m);
    SetItemMark(m, sFrameRateItem, gFrameRateOn ? checkMark : noMark);
}

void MacBench_SyncMenu(void)
{
    if (sFrameRateItem)
        SetItemMark(GetMHandle(mControlMenu), sFrameRateItem,
                    gFrameRateOn ? checkMark : noMark);
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
    if (item == sBenchItem && item != 0)
    {
        if (sDemoIndex == 0)
        {
            sFirstDemo = 1;
            sLastDemo = kBenchDemos;
            StartDemo(1);
        }
        return true;
    }
    return false;
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
    long cpu = 0, fpu = 0;

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
    while (fscanf(f, "%31s", word) == 1)
    {
        if (!strcmp(word, "-fps"))
            gFrameRateOn = true;
        else if (!strcmp(word, "-quit"))
            sAutoQuit = true;
        else if (!strcmp(word, "-bench"))
        {
            sAutoBench = true;
            sFirstDemo = 1;
            sLastDemo = kBenchDemos;
        }
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

/* Called once as the main loop starts. */
void MacBench_Autostart(void)
{
    if (sAutoBench)
        StartDemo(sFirstDemo);
}
