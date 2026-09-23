/*
 * Network transport stubs.
 *
 * The original game offered AppleTalk, serial, Comm Toolbox and IPX
 * multiplayer.  This port builds AppleTalk (AppleTalkNet.c), serial
 * (SerialNet.c) and TCP/IP (tcpnet.c); Comm Toolbox and MacIPX aren't wired
 * up, so their entry points are stubbed and the multiplayer dialog disables
 * them.
 */
#include "LionDoom.h"
#include "doomdef.h"

ConnHandle gConn = NULL;
short      gIPXSocket = 0;

OSErr CMIdle(ConnHandle hConn) { return noErr; }
OSErr CMClose(ConnHandle hConn, Boolean async, void *completor, long timeout,
              Boolean now) { return noErr; }

/* Comm Toolbox */
void    CTBInitializeNet(void)                            { }
Boolean CTBOpenConnection(void)                           { return false; }
OSErr   CTBWaitForConnection(void)                        { return cmFailed; }
void    CTBTerminateNet(void)                             { }
void    CTBKillIO(void)                                   { }
void    CTBFlushData(void)                                { }
OSErr   CTBSendString(UInt16 to, Ptr data, Size length)   { return noErr; }
Boolean CTBReadString(Ptr data, Size *length)             { return false; }
void    CTBSendPacketFilter(short node, Ptr data, long length) { }
Boolean CTBReceivePacketFilter(Ptr data, long *length)    { return false; }

/* IPX */
void    IPXSetup(void)                                    { }
void    IPXTerminateNet(void)                             { }
void    SendPacket(int destination)                       { }
Boolean GetPacket(long *size)                             { return false; }
