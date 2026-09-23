/*
 * Network transport stubs.
 *
 * The original game offered AppleTalk, serial, Comm Toolbox and IPX
 * multiplayer.  Those modules depend on AppleTalk / CTB / MacIPX interfaces
 * that Retro68's Multiversal headers do not provide, so this port builds
 * single-player only: gNetType stays kNoNet and none of these are reached.
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

void    GetPlayMode(void)                                 { gNetType = kNoNet; }

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
