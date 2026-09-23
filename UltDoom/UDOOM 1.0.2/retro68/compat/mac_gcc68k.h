/*
 * GCC / Retro68 replacements for the MPW-style inline traps DOOMDEF.H uses
 * for fixed-point math and WAD byte swapping.
 *
 * 68020+ (including the 68040 and 68LC040) implement the 32x32->64 muls.l and
 * 64/32 divs.l forms in hardware, so these are single instructions rather than
 * library calls.
 */
#ifndef MAC_GCC68K_H
#define MAC_GCC68K_H

static inline int FixedMul(int a, int b)
{
    int hi;
    __asm__("muls.l %2,%1:%0\n\t"
            "move.w %1,%0\n\t"
            "swap %0"
            : "+d"(a), "=&d"(hi)
            : "dmi"(b)
            : "cc");
    return a;
}

/* (a << 16) / b with the 64-bit dividend, no overflow checking. */
static inline int FixedDiv2(int a, int b)
{
    int hi = a >> 16;
    int lo = (int)((unsigned)a << 16);
    __asm__("divs.l %2,%1:%0"
            : "+d"(lo), "+d"(hi)
            : "dm"(b)
            : "cc");
    return lo;
}

static inline int FixedDiv(int a, int b)
{
    int aa = a < 0 ? -a : a;
    int ab = b < 0 ? -b : b;
    if ((aa >> 14) >= ab)
        return (a ^ b) < 0 ? (int)0x80000000 : 0x7fffffff;
    return FixedDiv2(a, b);
}

#define DoFixedDiv(a, b) FixedDiv2((a), (b))

#define SHORT(x) ((short)__builtin_bswap16((unsigned short)(x)))
#define LONG(x)  ((long)__builtin_bswap32((unsigned long)(x)))

#endif
