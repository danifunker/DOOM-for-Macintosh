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

/* KillIO: Multiversal declares it but ships no glue. */
OSErr Retro68KillIO(short refNum)
{
    ParamBlockRec pb;

    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    return PBKillIOSync(&pb);
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

/* ParamText with line feeds ("\n" under GCC) turned into returns, as MPW had them. */
#undef ParamText
pascal void Retro68ParamText(ConstStr255Param p0, ConstStr255Param p1,
                             ConstStr255Param p2, ConstStr255Param p3)
{
    static Str255      copy[4];
    ConstStr255Param   in[4];
    int                i, j;

    in[0] = p0; in[1] = p1; in[2] = p2; in[3] = p3;
    for (i = 0; i < 4; i++)
    {
        if (!in[i])
        {
            copy[i][0] = 0;
            continue;
        }
        memcpy(copy[i], in[i], in[i][0] + 1);
        for (j = 1; j <= copy[i][0]; j++)
            if (copy[i][j] == '\n')
                copy[i][j] = '\r';
    }
    ParamText(copy[0], copy[1], copy[2], copy[3]);
}

/* DirCreate: File Manager glue Multiversal lacks (via the FSSpec call). */
OSErr DirCreate(short vRefNum, long parentDirID, ConstStr255Param directoryName,
                long *createdDirID)
{
    FSSpec spec;
    OSErr  err = FSMakeFSSpec(vRefNum, parentDirID, directoryName, &spec);

    if (err != fnfErr)
        return err ? err : dupFNErr;        /* already there */
    return FSpDirCreate(&spec, smSystemScript, createdDirID);
}

/* SetEntries can move memory (the Color Manager rebuilds its inverse
   tables), and Lion passed it (**ctab).ctTable from an unlocked handle:
   when the table moved during the call, SetEntries read garbage and the
   machine took a bus error in system code.  Lock the table around it. */
void MacSetEntries(short start, short count, CTabHandle ctab)
{
    SignedByte state = HGetState((Handle)ctab);

    HLock((Handle)ctab);
    SetEntries(start, count, (**ctab).ctTable);
    HSetState((Handle)ctab, state);
}
