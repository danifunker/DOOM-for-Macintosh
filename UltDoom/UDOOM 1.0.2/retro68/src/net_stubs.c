/*
 * Network transport stubs.
 *
 * The original game offered AppleTalk, serial, Comm Toolbox and IPX
 * multiplayer.  Those modules depend on AppleTalk / CTB / MacIPX interfaces
 * that Retro68's Multiversal headers do not provide, so they are stubbed;
 * the multiplayer dialog only offers TCP/IP (tcpnet.c).
 */
#include "LionDoom.h"
#include "doomdef.h"

short      gNumPlayersOnNet = 0;
ConnHandle gConn = NULL;
short      gSerialPort = 0;
short      gIPXSocket = 0;

OSErr CMIdle(ConnHandle hConn) { return noErr; }
OSErr CMClose(ConnHandle hConn, Boolean async, void *completor, long timeout,
              Boolean now) { return noErr; }

void StatusParamText(char *one, char *two, char *three, char *four);
void StatusDialog(long total, long current);
void DrawStatusDialog(Boolean forUpdate);
void SpinCursor(void);
void TCPNet_Connect(void);

/* NetDialogs.c's GetPlayMode, reduced to the transports this port builds. */
void GetPlayMode(void)
{
    I_InitNetwork();                /* sets up doomcom and netgame */
    StatusParamText("\pStarting DOOM...", "\p", "\p", "\p");
    StatusDialog(80, 1);
    DrawStatusDialog(TRUE);
    SpinCursor();

    switch (gNetType)
    {
        case kTCPNet:
            TCPNet_Connect();
            break;
        default:
            I_Error("That network type is not available in this version.");
    }
}

/* AppleTalk */
OSErr   TerminateNet(void)                                { return noErr; }
OSErr   SendDDPPacket(UInt16 to, Ptr data, Size length)   { return noErr; }
Boolean ReceiveDDPPacket(Ptr data, Size *length, short *remoteNode) { return false; }

/* Serial */
void    SerTerminateNet(void)                             { }
void    SerSendPacketFilter(short node, Ptr data, long length) { }
Boolean SerReceivePacketFilter(Ptr data, long *length)    { return false; }

/* Comm Toolbox */
void    CTBTerminateNet(void)                             { }
void    CTBKillIO(void)                                   { }
void    CTBSendPacketFilter(short node, Ptr data, long length) { }
Boolean CTBReceivePacketFilter(Ptr data, long *length)    { return false; }

/* IPX */
void    IPXTerminateNet(void)                             { }
void    SendPacket(int destination)                       { }
Boolean GetPacket(long *size)                             { return false; }
