/*
 * MacTCP UDP interface for Retro68 (Multiversal Interfaces lack MacTCP.h).
 * Structures and codes from Apple's Universal Headers 2.0a3 MacTCP.h.
 * Everything here is a PBControl call on the ".IPP" driver, so the same code
 * works under MacTCP 2.x and under Open Transport's MacTCP compatibility.
 */
#ifndef MACTCP_UDP_H
#define MACTCP_UDP_H

typedef unsigned long  ip_addr;
typedef unsigned short udp_port;
typedef Ptr            StreamPtr;
struct ICMPReport;

typedef struct wdsEntry {
    unsigned short length;          /* 0 terminates the list */
    Ptr            ptr;
} wdsEntry;

enum { ipctlGetAddr = 15 };

typedef struct GetAddrParamBlock {
    QElemPtr       qLink;
    short          qType;
    short          ioTrap;
    Ptr            ioCmdAddr;
    ProcPtr        ioCompletion;
    OSErr          ioResult;
    StringPtr      ioNamePtr;
    short          ioVRefNum;
    short          ioCRefNum;
    short          csCode;
    ip_addr        ourAddress;
    long           ourNetMask;
} GetAddrParamBlock;

enum {
    UDPCreate = 20, UDPRead = 21, UDPBfrReturn = 22, UDPWrite = 23,
    UDPRelease = 24, UDPMaxMTUSize = 25, UDPStatus = 26
};
enum { UDPDataArrival = 1, UDPICMPReceived = 2 };

typedef pascal void (*UDPNotifyProcPtr)(StreamPtr udpStream, unsigned short eventCode,
                                        Ptr userDataPtr, struct ICMPReport *icmpMsg);

typedef struct UDPCreatePB {
    Ptr              rcvBuff;
    unsigned long    rcvBuffLen;
    UDPNotifyProcPtr notifyProc;
    unsigned short   localPort;
    Ptr              userDataPtr;
    udp_port         endingPort;
} UDPCreatePB;

typedef struct UDPSendPB {
    unsigned short reserved;
    ip_addr        remoteHost;
    udp_port       remotePort;
    Ptr            wdsPtr;
    Boolean        checkSum;
    UInt8          filler;
    unsigned short sendLength;
    Ptr            userDataPtr;
    udp_port       localPort;
} UDPSendPB;

typedef struct UDPReceivePB {
    unsigned short timeOut;         /* seconds; 0 = wait forever */
    ip_addr        remoteHost;
    udp_port       remotePort;
    Ptr            rcvBuff;
    unsigned short rcvBuffLen;
    unsigned short secondTimeStamp;
    Ptr            userDataPtr;
    ip_addr        destHost;
    udp_port       destPort;
} UDPReceivePB;

typedef struct UDPiopb {
    char           fill12[12];
    ProcPtr        ioCompletion;
    short          ioResult;
    char          *ioNamePtr;
    short          ioVRefNum;
    short          ioCRefNum;
    short          csCode;
    StreamPtr      udpStream;
    union {
        UDPCreatePB  create;
        UDPSendPB    send;
        UDPReceivePB receive;
    } csParam;
} UDPiopb;

#endif
