/*
 * AppleTalk interface glue that MPW's Interface.o supplied: C versions of
 * the routines in Apple's Libs/InterfaceSrcs/nAppleTalk.a and piNBP.a
 * (System 7.1 sources).  The P... calls are Control calls to the .MPP
 * driver; the rest just format buffers.
 */
#include "AppleTalk.h"
#include <string.h>

#define kMPPRefNum      (-10)           /* -(mppUnitNum + 1) */

/* DDP/LAP header layout (AppleTalk equates) */
#define lapHdSz         3
#define ddpDstNet       4
#define ddpDstNode      8
#define ddpDstSkt       10
#define ddpType         12
#define tupleSkt        3
#define tupleName       5

static OSErr MPPCtlCall(MPPPBPtr pb, short csCode, Boolean async)
{
    CntrlParam *cp = (CntrlParam *)pb;

    cp->ioCRefNum = kMPPRefNum;
    cp->csCode = csCode;
    return async ? PBControlAsync((ParmBlkPtr)pb) : PBControlSync((ParmBlkPtr)pb);
}

pascal OSErr POpenSkt(MPPPBPtr pb, Boolean async)      { return MPPCtlCall(pb, openSkt, async); }
pascal OSErr PCloseSkt(MPPPBPtr pb, Boolean async)     { return MPPCtlCall(pb, closeSkt, async); }
pascal OSErr PWriteDDP(MPPPBPtr pb, Boolean async)     { return MPPCtlCall(pb, writeDDP, async); }
pascal OSErr PRegisterName(MPPPBPtr pb, Boolean async) { return MPPCtlCall(pb, registerName, async); }
pascal OSErr PLookupName(MPPPBPtr pb, Boolean async)   { return MPPCtlCall(pb, lookupName, async); }
pascal OSErr PRemoveName(MPPPBPtr pb, Boolean async)   { return MPPCtlCall(pb, removeName, async); }

/*
 * One-block DDP write data structure.  The header buffer must be at least
 * 17 bytes; its first byte is skipped because DDP wants the header at an
 * odd address.
 */
pascal void BuildDDPwds(Ptr wdsPtr, Ptr headerPtr, Ptr dataPtr, AddrBlock netAddr,
                        short ddpTypeVal, short dataLen)
{
    unsigned char *h = (unsigned char *)headerPtr + 1 + lapHdSz;
    unsigned char *w = (unsigned char *)wdsPtr;

    *(Ptr *)(w + 2) = headerPtr + 1;            /* entry 0: header */
    *(short *)(w + 6) = dataLen;                /* entry 1: data */
    *(Ptr *)(w + 8) = dataPtr;
    *(short *)(w + 12) = 0;                     /* end of list */

    *(short *)(h + ddpDstNet) = netAddr.aNet;
    h[ddpDstNode] = netAddr.aNode;
    h[ddpDstSkt] = netAddr.aSocket;
    h[ddpType] = ddpTypeVal;
}

/* Concatenates the Pascal strings object, type and zone into buffer. */
pascal void NBPSetEntity(Ptr buffer, ConstStr32Param nbpObject,
                         ConstStr32Param nbpType, ConstStr32Param nbpZone)
{
    unsigned char *d = (unsigned char *)buffer;

    memcpy(d, nbpObject, nbpObject[0] + 1);
    d += nbpObject[0] + 1;
    memcpy(d, nbpType, nbpType[0] + 1);
    d += nbpType[0] + 1;
    memcpy(d, nbpZone, nbpZone[0] + 1);
}

/* Builds a Names Table Entry: clears the link, sets the socket and name. */
pascal void NBPSetNTE(Ptr ntePtr, ConstStr32Param nbpObject, ConstStr32Param nbpType,
                      ConstStr32Param nbpZone, short socket)
{
    unsigned char *tuple = (unsigned char *)ntePtr + 4;

    *(long *)ntePtr = 0;
    tuple[tupleSkt] = socket;
    NBPSetEntity((Ptr)(tuple + tupleName), nbpObject, nbpType, nbpZone);
}

/* Pulls tuple whichOne (1-based) out of a PLookupName result buffer. */
pascal OSErr NBPExtract(Ptr theBuffer, short numInBuf, short whichOne,
                        EntityName *abEntity, AddrBlock *address)
{
    unsigned char *t = (unsigned char *)theBuffer;
    unsigned char *parts[3];
    int            i, len;

    if (whichOne == 0 || whichOne > numInBuf)
        return extractErr;
    while (--whichOne)
    {
        unsigned char *n = t + tupleName;
        len = n[0] + 1;
        len += n[len] + 1;
        len += n[len] + 1;
        t = n + len;
    }

    memcpy(address, t, 4);
    t += tupleName;
    parts[0] = abEntity->objStr;
    parts[1] = abEntity->typeStr;
    parts[2] = abEntity->zoneStr;
    for (i = 0; i < 3; i++)
    {
        memcpy(parts[i], t, t[0] + 1);
        t += t[0] + 1;
    }
    return noErr;
}

/* TRUE when AppleTalk owns port B (piMAIN.a: PortBUse low nibble == UseATalk). */
pascal Boolean IsMPPOpen(void)
{
    signed char use = *(signed char *)0x291;    /* PortBUse */

    return use >= 0 && (use & 0x0F) == 1;
}

/* .XPP extended calls for zone information (nAppleTalk.a GZCtlCall). */
#define kXPPRefNum      (-41)                   /* -(xppUnitNum + 1) */
#define xCall           246
#define xppSubCodeOff   0x1C

static OSErr ZoneCall(XPPParmBlkPtr pb, short subCode, Boolean async)
{
    CntrlParam *cp = (CntrlParam *)pb;

    *(short *)((char *)pb + xppSubCodeOff) = subCode;
    cp->ioCRefNum = kXPPRefNum;
    cp->csCode = xCall;
    return async ? PBControlAsync((ParmBlkPtr)pb) : PBControlSync((ParmBlkPtr)pb);
}

pascal OSErr GetZoneList(XPPParmBlkPtr pb, Boolean async)   { return ZoneCall(pb, 6, async); }
pascal OSErr GetMyZone(XPPParmBlkPtr pb, Boolean async)     { return ZoneCall(pb, 7, async); }
pascal OSErr GetLocalZones(XPPParmBlkPtr pb, Boolean async) { return ZoneCall(pb, 5, async); }
