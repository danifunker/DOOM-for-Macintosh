/*
 * MUS -> Standard MIDI File conversion, so QuickTime can play the music
 * lumps in WADs (add-on WADs rarely came with Mac MIDI files).
 *
 * MUS is id's compact MIDI-like format: 16 channels (15 = percussion),
 * 140 ticks per second.  The result is a format 0 file with 70 ticks per
 * quarter note at 120 bpm, i.e. 140 ticks per second.  Same approach as
 * Chocolate DOOM's mus2mid.c (Ben Ryves, Simon Howard; GPL).
 */
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

typedef struct
{
    byte  *data;
    long   len, size;
    int    failed;
} outbuf_t;

static void Put(outbuf_t *o, byte b)
{
    if (o->len >= o->size)
    {
        long  n = o->size ? o->size * 2 : 16384;
        byte *d = (byte *) realloc(o->data, n);

        if (!d)
        {
            o->failed = 1;
            return;
        }
        o->data = d;
        o->size = n;
    }
    o->data[o->len++] = b;
}

static void PutVarLen(outbuf_t *o, unsigned long v)
{
    byte  buf[5];
    int   n = 0;

    buf[n++] = v & 0x7F;
    while ((v >>= 7) != 0)
        buf[n++] = (v & 0x7F) | 0x80;
    while (n--)
        Put(o, buf[n]);
}

/* MUS controller numbers 1-9 -> MIDI controllers (0 is program change) */
static const byte controllermap[10] = { 0, 0, 1, 7, 10, 11, 91, 93, 64, 67 };
/* MUS system events 10-14 -> MIDI channel mode controllers */
static const byte systemmap[15] = { 0,0,0,0,0,0,0,0,0,0, 120, 123, 126, 127, 121 };

/*
 * Converts a MUS lump.  Returns a malloc'd MIDI file (free() it) and its
 * length, or NULL if the data isn't MUS.
 */
byte *MUS2MID(const byte *mus, long muslen, long *midlen)
{
    outbuf_t      o;
    const byte   *p, *end;
    int           scorestart, i;
    int           channelmap[16];
    int           nextchannel = 0;
    byte          velocity[16];
    unsigned long delay = 0;
    long          tracklenpos;
    int           done = 0;

    if (muslen < 16 || memcmp(mus, "MUS\x1A", 4))
        return NULL;
    scorestart = mus[6] | (mus[7] << 8);
    if (scorestart >= muslen)
        return NULL;

    memset(&o, 0, sizeof o);
    for (i = 0; i < 16; i++)
    {
        channelmap[i] = -1;
        velocity[i] = 127;
    }

    /* header: format 0, one track, 70 ticks per quarter */
    {
        static const byte head[] = {
            'M','T','h','d', 0,0,0,6, 0,0, 0,1, 0,70,
            'M','T','r','k', 0,0,0,0 };
        for (i = 0; i < sizeof head; i++)
            Put(&o, head[i]);
    }
    tracklenpos = o.len - 4;
    /* tempo: 500000 us per quarter (120 bpm) */
    Put(&o, 0); Put(&o, 0xFF); Put(&o, 0x51); Put(&o, 3);
    Put(&o, 0x07); Put(&o, 0xA1); Put(&o, 0x20);

    p = mus + scorestart;
    end = mus + muslen;
    while (!done && p < end && !o.failed)
    {
        byte  ev, type, last;
        int   ch, mch;

        ev = *p++;
        last = ev & 0x80;
        type = (ev >> 4) & 7;
        ch = ev & 15;

        /* MUS channel 15 is percussion (MIDI 10, i.e. 9); the others take
           MIDI channels in order of first use, skipping 9. */
        if (ch == 15)
            mch = 9;
        else
        {
            if (channelmap[ch] < 0)
            {
                if (nextchannel == 9)
                    nextchannel++;
                channelmap[ch] = nextchannel++ & 15;
            }
            mch = channelmap[ch];
        }

        switch (type)
        {
            case 0:     /* release note */
                if (p >= end) { done = 1; break; }
                PutVarLen(&o, delay); delay = 0;
                Put(&o, 0x80 | mch);
                Put(&o, *p++ & 0x7F);
                Put(&o, 0);
                break;

            case 1:     /* play note, maybe with a new volume */
            {
                byte note;

                if (p >= end) { done = 1; break; }
                note = *p++;
                if (note & 0x80)
                {
                    if (p >= end) { done = 1; break; }
                    velocity[ch] = *p++ & 0x7F;
                }
                PutVarLen(&o, delay); delay = 0;
                Put(&o, 0x90 | mch);
                Put(&o, note & 0x7F);
                Put(&o, velocity[ch]);
                break;
            }

            case 2:     /* pitch wheel: 0-255, 128 = centre */
            {
                unsigned v;

                if (p >= end) { done = 1; break; }
                v = *p++ * 64;
                PutVarLen(&o, delay); delay = 0;
                Put(&o, 0xE0 | mch);
                Put(&o, v & 0x7F);
                Put(&o, (v >> 7) & 0x7F);
                break;
            }

            case 3:     /* system event */
            {
                byte n;

                if (p >= end) { done = 1; break; }
                n = *p++ & 0x7F;
                if (n >= 10 && n <= 14)
                {
                    PutVarLen(&o, delay); delay = 0;
                    Put(&o, 0xB0 | mch);
                    Put(&o, systemmap[n]);
                    Put(&o, 0);
                }
                break;
            }

            case 4:     /* controller */
            {
                byte n, v;

                if (p + 1 >= end) { done = 1; break; }
                n = *p++ & 0x7F;
                v = *p++ & 0x7F;
                if (n > 9)
                    break;              /* unknown: dropped, time carries on */
                PutVarLen(&o, delay); delay = 0;
                if (n == 0)
                {   /* program change */
                    Put(&o, 0xC0 | mch);
                    Put(&o, v);
                }
                else
                {
                    Put(&o, 0xB0 | mch);
                    Put(&o, controllermap[n]);
                    Put(&o, v);
                }
                break;
            }

            case 5:     /* end of measure */
                break;

            case 6:     /* score end */
                done = 1;
                break;

            default:    /* 7: unused, carries no data */
                break;
        }

        if (last && !done)
        {
            unsigned long t = 0;
            byte          b;

            do
            {
                if (p >= end) { done = 1; break; }
                b = *p++;
                t = (t << 7) | (b & 0x7F);
            } while (b & 0x80);
            delay += t;
        }
    }

    /* end of track */
    PutVarLen(&o, delay);
    Put(&o, 0xFF); Put(&o, 0x2F); Put(&o, 0);

    if (o.failed)
    {
        free(o.data);
        return NULL;
    }
    {
        long tl = o.len - tracklenpos - 4;

        o.data[tracklenpos]     = (byte)(tl >> 24);
        o.data[tracklenpos + 1] = (byte)(tl >> 16);
        o.data[tracklenpos + 2] = (byte)(tl >> 8);
        o.data[tracklenpos + 3] = (byte) tl;
    }
    *midlen = o.len;
    return o.data;
}
