/*
 * Multiplayer menu: start a TCP/IP, serial or AppleTalk game from inside a
 * running session (the original game only offered multiplayer in the
 * start-up dialog).
 *
 * Each item opens the usual multiplayer dialog preset to that connection.
 * The network is then brought up at the top of the next main-loop pass
 * (MacMenus_Poll), not inside the menu handler, which runs in the middle of
 * a tic: the same GetPlayMode / D_CheckNetGame / G_InitNew sequence the
 * start-up path uses, with the tic counters reset so every machine starts
 * the lockstep from zero.
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "DoomResources.h"

void M_ClearMenus(void);

#define kMultiplayerMenuID  211

enum { iMPHost = 1, iMPJoin, iMPSep1, iMPSerialModem, iMPSerialPrinter, iMPSep2,
       iMPAppleTalk };

extern short    gMPPreset;              /* I_MAIN.C: dialog preset */
extern Boolean  gPlayAlone, gPlayNetGame;
extern boolean  timingdemo, singletics, demoplayback, netdemo;
extern int      maketic;

short MultiplayerOptionsDialog(void);
void  GetPlayMode(void);
void  BlackScreen(void);
void  RedrawScreen(void);
void  D_CheckNetGame(void);
void  CloseStatusDialog(void);
void  MacBench_SyncMenu(void);

static MenuHandle sMenu;
static Boolean    sStartPending;

/* ---- Options > Mouse Look ----
   Key layout 4 (KeyConfig.c): the mouse turns and fires, W A S D or the
   arrow keys move and strafe, and moving the mouse forward does nothing
   (G_BuildTiccmd).  Saved in the prefs with the other layouts. */
#define kMouseLookConfig    4
extern int      gDoomMoveConfig, useMouse, usejoystick;
extern Boolean  gMouseCaptured;
static short    sMouseLookItem;
static int      sPrevMoveConfig = 1;

static void SyncControlMarks(void)
{
    MenuHandle m = GetMHandle(mOptionsMenu);
    Boolean    look = gDoomMoveConfig == kMouseLookConfig;

    if (!m || !sMouseLookItem)
        return;
    SetItemMark(m, 1, !look && !useMouse && !usejoystick ? checkMark : noMark);
    SetItemMark(m, 2, !look && useMouse && !usejoystick ? checkMark : noMark);
    SetItemMark(m, 3, !look && usejoystick ? checkMark : noMark);
    SetItemMark(m, sMouseLookItem, look ? checkMark : noMark);
}

void MacMenus_InstallControls(void)
{
    MenuHandle m = GetMHandle(mOptionsMenu);

    if (!m)
        return;
    AppendMenu(m, "\p(-;x");
    sMouseLookItem = CountMItems(m);
    SetMenuItemText(m, sMouseLookItem, "\pMouse Look (WASD / Arrows)");
    if (gDoomMoveConfig == kMouseLookConfig)
    {
        useMouse = 1;
        usejoystick = 0;
    }
    SyncControlMarks();
}

/* After Lion's own Options handling (I_IBM.C HandleMenu). */
void MacMenus_HandleOptions(short item)
{
    Boolean look = gDoomMoveConfig == kMouseLookConfig;

    if (sMouseLookItem && item == sMouseLookItem)
    {
        if (look)
            gDoomMoveConfig = sPrevMoveConfig;  /* back to plain mouse mode */
        else
        {
            sPrevMoveConfig = gDoomMoveConfig;
            gDoomMoveConfig = kMouseLookConfig;
            useMouse = 1;
            usejoystick = 0;
        }
        gMouseCaptured = false;             /* I_StartTic captures it again */
        InitCursor();
    }
    else if (look && item >= 1 && item <= 3)
    {
        gDoomMoveConfig = sPrevMoveConfig;  /* Keyboard / Mouse / Joystick */
        if (item == 2)
        {
            useMouse = 1;
            usejoystick = 0;
        }
    }
    SyncControlMarks();
}

void MacMenus_Install(void)
{
    sMenu = NewMenu(kMultiplayerMenuID, "\pMultiplayer");
    if (!sMenu)
        return;
    /* AppendMenu reads "/" and "(" as metacharacters, so add placeholders
       and set the real titles with SetMenuItemText, which takes them as is. */
    AppendMenu(sMenu, "\px;x;(-;x;x;(-;x");
    SetMenuItemText(sMenu, iMPHost, "\pHost TCP/IP Game\311");
    SetMenuItemText(sMenu, iMPJoin, "\pJoin TCP/IP Game\311");
    SetMenuItemText(sMenu, iMPSerialModem, "\pSerial Game (Modem Port)\311");
    SetMenuItemText(sMenu, iMPSerialPrinter, "\pSerial Game (Printer Port)\311");
    SetMenuItemText(sMenu, iMPAppleTalk, "\pAppleTalk Game\311");
    InsertMenu(sMenu, 0);
    DrawMenuBar();
}

/* Grey the menu out once a network game is running. */
void MacMenus_Sync(void)
{
    if (!sMenu)
        return;
    if (netgame)
        DisableItem(sMenu, 0);
    else
        EnableItem(sMenu, 0);
    DrawMenuBar();
}

/* Returns true when the menu was ours. */
Boolean MacMenus_Handle(short menuID, short item)
{
    static const short preset[] = { 0, 1, 2, 0, 3, 4, 0, 5 };   /* by item */

    if (menuID != kMultiplayerMenuID)
        return false;
    if (netgame || item < iMPHost || item > iMPAppleTalk || !preset[item])
        return true;

    gMPPreset = preset[item];
    InitCursor();
    MultiplayerOptionsDialog();
    HideCursor();
    gMPPreset = 0;

    if (gPlayNetGame)
        sStartPending = true;           /* OK was clicked */
    else
        gPlayAlone = true;
    RedrawScreen();
    return true;
}

/* Called at the top of every main-loop pass. */
void MacMenus_Poll(void)
{
    extern void MacWads_Poll(void);
    extern void MacWads_SyncMenu(void);

    MacWads_Poll();                     /* WADs menu: reload requested */
    if (!sStartPending)
        return;
    sStartPending = false;

    /* leave whatever is running: demo, timedemo, menus */
    timingdemo = false;
    singletics = false;
    demoplayback = false;
    netdemo = false;
    paused = false;
    M_ClearMenus();

    BlackScreen();                      /* status text goes on a clean screen */
    GetPlayMode();                      /* lobby / handshake for gNetType */

    gametic = 0;                        /* everyone starts the lockstep at 0 */
    maketic = 0;
    D_CheckNetGame();                   /* arbitration, players, netgame */
    CloseStatusDialog();                /* clear the status line */
    BlackScreen();
    RedrawScreen();
    G_InitNew(startskill, startepisode, startmap);

    MacMenus_Sync();
    MacBench_SyncMenu();
    MacWads_SyncMenu();                 /* no WAD changes during a net game */
}
