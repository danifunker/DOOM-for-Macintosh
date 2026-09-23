/*
 * Retro68 stand-in for hdrs/AppleTalkNet.h.
 *
 * AppleTalk multiplayer is not built in this port (Multiversal Interfaces
 * have no AppleTalk Manager), so this keeps only the constants and entry
 * points the shared code refers to.  The functions live in net_stubs.c.
 */
#ifndef __APPLETALKNET__
#define __APPLETALKNET__

#define kMaxNetPlayers 4

extern short gNumPlayersOnNet;

Boolean IsVMOn(void);
OSErr   CheckAppleTalkState(void);
OSErr   InitSocketListener(void);
OSErr   InitializeNet(void);
OSErr   TerminateSocketListener(void);
OSErr   TerminateNet(void);
OSErr   SendDDPPacket(UInt16 sendTo, Ptr data, Size length);
Boolean ReceiveDDPPacket(Ptr data, Size *length, short *remoteNode);
void    AppleTalkConnect(void);

#endif
