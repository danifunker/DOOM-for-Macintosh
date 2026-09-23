/*
 * TCP/IP multiplayer: DOOM's lockstep packets carried in UDP datagrams over
 * MacTCP (which Open Transport also provides, through its MacTCP
 * compatibility), so it runs on any 68K Mac with either stack.
 *
 * Topology is a star around the host (player 0).  Joiners talk only to the
 * host, which relays packets meant for other joiners.  Only the host needs a
 * reachable UDP port (kTCPPort; forward it on the router for internet play);
 * joiners behind NAT work because their own outgoing packets open the path.
 *
 * Lobby:  joiner --JOIN(WAD signature)--> host   (repeated until answered)
 *         host  --WELCOME(player, total)-> joiner
 *               or REJECT(host's WAD list) when the WADs differ
 *         host  --START(total)--> joiners  (repeated until each ACKs)
 * then the usual D_ArbitrateNetStart / tic exchange runs as GAME packets.
 *
 * Node numbering (what D_NET.C calls "nodes"): node 0 is always this
 * machine; nodes 1.. are the other players in ascending player order.  On
 * the host that makes node i == player i, which D_ArbitrateNetStart needs.
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "DoomResources.h"
#include "MacTCP.h"

#include <stdio.h>
#include <string.h>

#define kTCPPort        5029
#define kMagic          0x4434          /* 'D4' */
#define kMaxPlayers     4
#define kRecvBufSize    32768
#define kResendTicks    30              /* half a second */

enum { kMsgJoin = 1, kMsgWelcome, kMsgStart, kMsgAck, kMsgFull, kMsgGame, kMsgReject };

unsigned long MacWads_Signature(void);
void          MacWads_Describe(char *out, int max);

typedef struct {
    unsigned short magic;
    unsigned char  type;
    unsigned char  src;                 /* sending player */
    unsigned char  dst;                 /* destination player */
    unsigned char  arg;                 /* message specific */
    unsigned short len;                 /* payload bytes following */
} NetHdr;

typedef struct {
    ip_addr  ip;
    udp_port port;
    Boolean  used;
    Boolean  acked;
} Peer;

/* Settings filled in by the dialog or "DOOM Args". */
char            gTCPHostAddr[64];       /* joiner: "a.b.c.d" or "a.b.c.d:port" */

extern Boolean  gKeyPlayer;
extern int      gPlayersWanted;
extern short    gNumPlayersOnNet;
extern Boolean  gPCCommunication;

void StatusParamText(char *one, char *two, char *three, char *four);
void StatusDialog(long total, long current);
void DrawStatusDialog(Boolean forUpdate);
void SpinCursor(void);

static short     sIPP;
static StreamPtr sStream;
static char      sRecvBuf[kRecvBufSize];
static UDPiopb   sReadPB;
static Boolean   sReadPending;
static Peer      sPeer[kMaxPlayers];    /* host: joiners by player; joiner: [0] = host */
static int       sMe, sNumPlayers;
static Boolean   sIsHost, sOpen;

/* ------------------------------------------------------------------------ */

static void IPToString(ip_addr a, char *s)
{
    sprintf(s, "%lu.%lu.%lu.%lu", a >> 24, (a >> 16) & 255, (a >> 8) & 255, a & 255);
}

/* Parses "a.b.c.d" or "a.b.c.d:port". */
Boolean TCPNet_ParseAddr(const char *s, ip_addr *ip, udp_port *port)
{
    unsigned a, b, c, d, p = kTCPPort;
    int      n = sscanf(s, "%u.%u.%u.%u:%u", &a, &b, &c, &d, &p);

    if (n < 4 || a > 255 || b > 255 || c > 255 || d > 255 || p == 0 || p > 65535)
        return false;
    *ip = ((ip_addr)a << 24) | ((ip_addr)b << 16) | (c << 8) | d;
    *port = p;
    return true;
}

static OSErr OpenIPP(void)
{
    if (sIPP)
        return noErr;
    return OpenDriver("\p.IPP", &sIPP);
}

/* Our own address as text, or "" when MacTCP is missing/unconfigured. */
void TCPNet_LocalAddress(char *out)
{
    GetAddrParamBlock ga;

    out[0] = 0;
    if (OpenIPP() != noErr)
        return;
    memset(&ga, 0, sizeof ga);
    ga.ioCRefNum = sIPP;
    ga.csCode = ipctlGetAddr;
    if (PBControlSync((ParmBlkPtr)&ga) == noErr && ga.ourAddress)
        IPToString(ga.ourAddress, out);
}

static void StartRead(void)
{
    memset(&sReadPB, 0, sizeof sReadPB);
    sReadPB.ioCRefNum = sIPP;
    sReadPB.csCode = UDPRead;
    sReadPB.udpStream = sStream;
    sReadPB.csParam.receive.timeOut = 0;        /* no timeout; we poll */
    sReadPending = PBControlAsync((ParmBlkPtr)&sReadPB) == noErr;
}

static OSErr OpenStream(udp_port port)
{
    UDPiopb pb;
    OSErr   err;

    if ((err = OpenIPP()) != noErr)
        return err;
    memset(&pb, 0, sizeof pb);
    pb.ioCRefNum = sIPP;
    pb.csCode = UDPCreate;
    pb.csParam.create.rcvBuff = sRecvBuf;
    pb.csParam.create.rcvBuffLen = sizeof sRecvBuf;
    pb.csParam.create.localPort = port;
    if ((err = PBControlSync((ParmBlkPtr)&pb)) != noErr)
        return err;
    sStream = pb.udpStream;
    sOpen = true;
    StartRead();
    return noErr;
}

static void Send(ip_addr ip, udp_port port, int type, int dst, int arg,
                 const void *data, int len)
{
    NetHdr   h;
    wdsEntry wds[3];
    UDPiopb  pb;

    h.magic = kMagic;
    h.type = type;
    h.src = sMe;
    h.dst = dst;
    h.arg = arg;
    h.len = len;
    wds[0].length = sizeof h;
    wds[0].ptr = (Ptr)&h;
    wds[1].length = len;
    wds[1].ptr = (Ptr)data;
    wds[2].length = 0;
    if (len == 0)
        wds[1].length = 0;

    memset(&pb, 0, sizeof pb);
    pb.ioCRefNum = sIPP;
    pb.csCode = UDPWrite;
    pb.udpStream = sStream;
    pb.csParam.send.remoteHost = ip;
    pb.csParam.send.remotePort = port;
    pb.csParam.send.wdsPtr = (Ptr)wds;
    pb.csParam.send.checkSum = true;
    PBControlSync((ParmBlkPtr)&pb);
}

/* Forwards an already-framed datagram unchanged (host relay). */
static void Forward(ip_addr ip, udp_port port, const void *pkt, int len)
{
    wdsEntry wds[2];
    UDPiopb  pb;

    wds[0].length = len;
    wds[0].ptr = (Ptr)pkt;
    wds[1].length = 0;
    memset(&pb, 0, sizeof pb);
    pb.ioCRefNum = sIPP;
    pb.csCode = UDPWrite;
    pb.udpStream = sStream;
    pb.csParam.send.remoteHost = ip;
    pb.csParam.send.remotePort = port;
    pb.csParam.send.wdsPtr = (Ptr)wds;
    pb.csParam.send.checkSum = true;
    PBControlSync((ParmBlkPtr)&pb);
}

/* Non-blocking receive of one datagram; returns its length or -1. */
static int Poll(char *out, int max, ip_addr *ip, udp_port *port)
{
    UDPiopb ret;
    int     n;

    if (!sOpen)
        return -1;
    if (!sReadPending)
        StartRead();
    if (!sReadPending || sReadPB.ioResult > 0)
        return -1;                              /* still waiting */

    sReadPending = false;
    n = -1;
    if (sReadPB.ioResult == noErr)
    {
        n = sReadPB.csParam.receive.rcvBuffLen;
        if (n > max)
            n = max;
        memcpy(out, sReadPB.csParam.receive.rcvBuff, n);
        *ip = sReadPB.csParam.receive.remoteHost;
        *port = sReadPB.csParam.receive.remotePort;

        ret = sReadPB;                          /* hand the buffer back */
        ret.csCode = UDPBfrReturn;
        PBControlSync((ParmBlkPtr)&ret);
    }
    StartRead();
    return n;
}

static Boolean UserCancelled(void)
{
    EventRecord ev;

    while (GetOSEvent(keyDownMask, &ev))
    {
        char c = ev.message & charCodeMask;
        if (c == 0x1B || (c == '.' && (ev.modifiers & cmdKey)))
            return true;
    }
    return false;
}

static void NetStatus(const char *fmt, const char *a, int b, int c)
{
    Str255 s;

    sprintf((char *)s, fmt, a, b, c);
    c2pstr((char *)s);
    StatusParamText((char *)s, "\p", "\p", "\p");
    DrawStatusDialog(TRUE);
}

/* ------------------------------------------------------------------------ */

static int NodeToPlayer(int node)
{
    int p;

    if (node == 0)
        return sMe;
    for (p = 0; p < sNumPlayers; p++)
        if (p != sMe && --node == 0)
            return p;
    return -1;
}

static int PlayerToNode(int player)
{
    int p, node = 0;

    if (player == sMe)
        return 0;
    for (p = 0; p < sNumPlayers; p++)
        if (p != sMe)
        {
            node++;
            if (p == player)
                return node;
        }
    return -1;
}

static void HostLobby(void)
{
    static char pkt[64];                /* JOIN: header + 4-byte WAD signature */
    NetHdr     *h = (NetHdr *)pkt;
    char        myIP[20];
    ip_addr     ip;
    udp_port    port;
    int         n, p, joined = 1, shown = 0, waiting;
    long        lastSend = 0;

    sIsHost = true;
    sMe = 0;
    sNumPlayers = gPlayersWanted;
    if (sNumPlayers < 2 || sNumPlayers > kMaxPlayers)
        sNumPlayers = 2;

    TCPNet_LocalAddress(myIP);
    NetStatus("%sChecking WAD files...", "", 0, 0);
    (void) MacWads_Signature();             /* compute now so joins are answered quickly */
    if (OpenStream(kTCPPort) != noErr)
        I_Error("TCP/IP: can't open UDP port %d (is another copy running?)", kTCPPort);

    for (;;)
    {
        if (joined != shown && joined < sNumPlayers)
        {
            NetStatus("Hosting on %s port 5029: %d of %d players. Esc cancels.",
                   myIP, joined, sNumPlayers);
            shown = joined;
        }
        SpinCursor();
        if (UserCancelled())
            I_Error("Network game cancelled.");

        while ((n = Poll(pkt, sizeof pkt, &ip, &port)) >= (int)sizeof(NetHdr))
        {
            if (h->magic != kMagic)
                continue;
            if (h->type == kMsgJoin)
            {
                unsigned long sig = 0;

                /* every player must load the same WADs in the same order */
                if (n >= (int)(sizeof(NetHdr) + 4))
                    memcpy(&sig, pkt + sizeof(NetHdr), 4);
                if (sig != MacWads_Signature())
                {
                    char list[160];

                    MacWads_Describe(list, sizeof list);
                    Send(ip, port, kMsgReject, 0, 0, list, strlen(list) + 1);
                    continue;
                }
                for (p = 1; p < sNumPlayers; p++)
                    if (sPeer[p].used && sPeer[p].ip == ip && sPeer[p].port == port)
                        break;
                if (p == sNumPlayers)           /* new player */
                {
                    for (p = 1; p < sNumPlayers && sPeer[p].used; p++)
                        ;
                    if (p == sNumPlayers)
                    {
                        Send(ip, port, kMsgFull, 0, 0, NULL, 0);
                        continue;
                    }
                    sPeer[p].used = true;
                    sPeer[p].ip = ip;
                    sPeer[p].port = port;
                    joined++;
                }
                Send(ip, port, kMsgWelcome, p, p, NULL, 0);
            }
            else if (h->type == kMsgAck && h->src < sNumPlayers)
                sPeer[h->src].acked = true;
        }

        if (joined < sNumPlayers)
            continue;

        /* Everyone is in: announce the start until each player confirms. */
        waiting = 0;
        for (p = 1; p < sNumPlayers; p++)
            if (!sPeer[p].acked)
                waiting++;
        if (!waiting)
            break;
        if (TickCount() - lastSend > kResendTicks)
        {
            NetStatus("%sAll %d players joined; starting game...", "", sNumPlayers, 0);
            for (p = 1; p < sNumPlayers; p++)
                if (!sPeer[p].acked)
                    Send(sPeer[p].ip, sPeer[p].port, kMsgStart, p, sNumPlayers, NULL, 0);
            lastSend = TickCount();
        }
    }
}

static void JoinLobby(void)
{
    static char pkt[224];
    NetHdr     *h = (NetHdr *)pkt;
    ip_addr     ip;
    udp_port    port;
    long        lastSend = 0, started = TickCount();
    Boolean     welcomed = false;
    unsigned long sig;
    int         n;

    sIsHost = false;
    NetStatus("%sChecking WAD files...", "", 0, 0);
    sig = MacWads_Signature();              /* reads every WAD once (cached) */
    if (!TCPNet_ParseAddr(gTCPHostAddr, &sPeer[0].ip, &sPeer[0].port))
        I_Error("TCP/IP: \"%s\" is not an IP address (use a.b.c.d or a.b.c.d:port)",
                gTCPHostAddr);
    sPeer[0].used = true;
    if (OpenStream(0) != noErr)                 /* any free local port */
        I_Error("TCP/IP: can't open a UDP stream. Is MacTCP / TCP/IP configured?");

    for (;;)
    {
        SpinCursor();
        if (UserCancelled())
            I_Error("Network game cancelled.");

        if (!welcomed && TickCount() - lastSend > kResendTicks * 2)
        {
            NetStatus("Contacting %s ... Esc cancels.", gTCPHostAddr, 0, 0);
            Send(sPeer[0].ip, sPeer[0].port, kMsgJoin, 0, 0, &sig, 4);
            lastSend = TickCount();
            if (TickCount() - started > 60 * 60)
                I_Error("TCP/IP: no answer from %s after a minute.", gTCPHostAddr);
        }

        while ((n = Poll(pkt, sizeof pkt - 1, &ip, &port)) >= (int)sizeof(NetHdr))
        {
            if (h->magic != kMagic || ip != sPeer[0].ip)
                continue;
            switch (h->type)
            {
                case kMsgReject:
                {
                    char mine[160];

                    pkt[n] = 0;
                    MacWads_Describe(mine, sizeof mine);
                    TCPNet_Terminate();
                    I_Error("TCP/IP: the host is playing with different WAD files. "
                            "Host: %s. You: %s.", pkt + sizeof(NetHdr), mine);
                    break;
                }
                case kMsgWelcome:
                    if (!welcomed)
                    {
                        welcomed = true;
                        sMe = h->arg;
                        NetStatus("Joined %s as player %d; waiting for the others...",
                               gTCPHostAddr, sMe + 1, 0);
                    }
                    break;
                case kMsgFull:
                    I_Error("TCP/IP: the game at %s is already full.", gTCPHostAddr);
                    break;
                case kMsgStart:
                    sMe = h->dst;
                    sNumPlayers = h->arg;
                    sPeer[0].port = port;       /* in case NAT rewrote it */
                    Send(sPeer[0].ip, sPeer[0].port, kMsgAck, 0, 0, NULL, 0);
                    return;
            }
        }
    }
}

/* Called from GetPlayMode once the dialog (or DOOM Args) chose TCP/IP. */
void TCPNet_Connect(void)
{
    char myIP[20];

    gPCCommunication = false;
    TCPNet_LocalAddress(myIP);
    if (!myIP[0])
        I_Error("TCP/IP networking isn't available. Install and configure "
                "MacTCP or Open Transport TCP/IP.");

    if (gKeyPlayer)
        HostLobby();
    else
        JoinLobby();

    doomcom->consoleplayer = sMe;
    gNumPlayersOnNet = sNumPlayers - 1;
}

/* ------------------------------------------------------------------------ */

void TCPNet_Send(int node, Ptr data, long length)
{
    int dst = NodeToPlayer(node);

    if (dst < 0 || dst == sMe)
        return;
    if (sIsHost)
        Send(sPeer[dst].ip, sPeer[dst].port, kMsgGame, dst, 0, data, length);
    else                                        /* everything goes via the host */
        Send(sPeer[0].ip, sPeer[0].port, kMsgGame, dst, 0, data, length);
}

Boolean TCPNet_Receive(Ptr data, long *length, short *remoteNode)
{
    static char pkt[sizeof(NetHdr) + sizeof(doomdata_t) + 16];
    NetHdr     *h = (NetHdr *)pkt;
    ip_addr     ip;
    udp_port    port;
    int         n, len;

    while ((n = Poll(pkt, sizeof pkt, &ip, &port)) >= (int)sizeof(NetHdr))
    {
        if (h->magic != kMagic || h->src >= sNumPlayers)
            continue;

        if (h->type == kMsgStart && !sIsHost)
        {   /* our ACK got lost; the host is still announcing */
            Send(sPeer[0].ip, sPeer[0].port, kMsgAck, 0, 0, NULL, 0);
            continue;
        }
        if (h->type == kMsgAck && sIsHost)
        {
            sPeer[h->src].acked = true;
            continue;
        }
        if (h->type != kMsgGame)
            continue;

        if (sIsHost)
        {
            /* only accept players from the address they joined from */
            if (h->src == 0 || sPeer[h->src].ip != ip)
                continue;
            if (h->dst != 0)
            {
                if (h->dst < sNumPlayers)
                    Forward(sPeer[h->dst].ip, sPeer[h->dst].port, pkt, n);
                continue;
            }
        }
        else if (ip != sPeer[0].ip || h->dst != sMe)
            continue;

        len = n - sizeof(NetHdr);
        if (len > *length)
            len = *length;
        memcpy(data, pkt + sizeof(NetHdr), len);
        *length = len;
        *remoteNode = PlayerToNode(h->src);
        return true;
    }
    return false;
}

void TCPNet_Terminate(void)
{
    UDPiopb pb;

    if (!sOpen)
        return;
    memset(&pb, 0, sizeof pb);
    pb.ioCRefNum = sIPP;
    pb.csCode = UDPRelease;                     /* also cancels the pending read */
    pb.udpStream = sStream;
    PBControlSync((ParmBlkPtr)&pb);
    sOpen = false;
    sReadPending = false;
}
