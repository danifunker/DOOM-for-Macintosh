/*
 * Small pieces of Toolbox glue that MPW's Interface.o / StdCLib supplied and
 * Retro68's Multiversal libraries do not.
 */
#include <string.h>

char *p2cstr(unsigned char *s)
{
    unsigned char len = s[0];
    memmove(s, s + 1, len);
    s[len] = 0;
    return (char *)s;
}

unsigned char *c2pstr(char *s)
{
    size_t len = strlen(s);
    if (len > 255)
        len = 255;
    memmove(s + 1, s, len);
    s[0] = (char)len;
    return (unsigned char *)s;
}

OSErr HGetVol(StringPtr volName, short *vRefNum, long *dirID)
{
    WDPBRec pb;

    memset(&pb, 0, sizeof(pb));
    pb.ioNamePtr = volName;
    PBHGetVolSync(&pb);
    *vRefNum = pb.ioVRefNum;
    *dirID = pb.ioWDDirID;
    return pb.ioResult;
}
