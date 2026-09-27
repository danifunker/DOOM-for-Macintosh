/*
 * Music for QuickTime: MUS lumps (id's format) and standard MIDI files are
 * turned into compact standard MIDI files QuickTime 2.x can import.
 *
 * MUS is id's compact MIDI-like format: 16 channels (15 = percussion),
 * 140 ticks per second.  It becomes 70 ticks per quarter note at 120 bpm,
 * i.e. 140 ticks per second.  Same approach as Chocolate DOOM's mus2mid.c
 * (Ben Ryves, Simon Howard; GPL).
 *
 * Both kinds of input are read into one list of events, then written as a
 * format 0 file: running status, note-offs as note-ons with velocity 0 (so
 * notes share one status byte), controller changes that change nothing
 * left out.
 *
 * QuickTime 2.x's MIDI import crashes on files of more than about 17,000
 * events (DOOM's E3M4 has 18,000, DOOM II's D_DDTBL2 20,000; splitting the
 * channels into tracks doesn't help).  Songs over kMaxEvents are thinned:
 * continuous controllers (modulation, volume, pan, expression) and pitch
 * bends are limited to one change per "gap" per channel, starting at 1/35
 * second (DOOM's tic) and doubling until the song fits.  Notes are never
 * dropped.  The latest value wins: a change inside the gap is held and
 * written when the gap is over, so fades and bends still end on their
 * exact values (a note may start with a level up to one gap old).
 */
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

#define kMaxEvents  16000

/* ---- output buffer ---- */

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

/* ---- event list ---- */

typedef struct
{
    unsigned long tick;
    byte          len;          /* bytes used in d */
    byte          d[6];         /* status + data, or FF type len data (tempo) */
} event_t;

typedef struct
{
    event_t      *ev;
    long          n, size;
    int           failed;
    unsigned long end;          /* the song's length, trailing rests too */
} evlist_t;

static void Add(evlist_t *l, unsigned long tick, const byte *d, int len)
{
    if (l->n >= l->size)
    {
        long     n = l->size ? l->size * 2 : 4096;
        event_t *e = (event_t *) realloc(l->ev, n * sizeof(event_t));

        if (!e)
        {
            l->failed = 1;
            return;
        }
        l->ev = e;
        l->size = n;
    }
    l->ev[l->n].tick = tick;
    l->ev[l->n].len = len;
    memcpy(l->ev[l->n].d, d, len);
    l->n++;
}

static void Add3(evlist_t *l, unsigned long tick, byte a, byte b, byte c, int len)
{
    byte d[3];

    d[0] = a;
    d[1] = b;
    d[2] = c;
    Add(l, tick, d, len);
}

/* ---- thinning and writing ---- */

/* Held values: slot 0-3 = controllers 1, 7, 10, 11; slot 4 = pitch bend. */
static int Slot(const event_t *e)
{
    byte k = e->d[0] >> 4;

    if (k == 0xE)
        return 4;
    if (k == 0xB)
        switch (e->d[1])
        {
            case 1:  return 0;
            case 7:  return 1;
            case 10: return 2;
            case 11: return 3;
        }
    return -1;
}

typedef struct
{
    outbuf_t      o;
    byte          running;
    unsigned long last;             /* tick of the last event written */
    long          count;
} writer_t;

static void WriteEvent(writer_t *w, const event_t *e, unsigned long tick)
{
    w->count++;
    PutVarLen(&w->o, tick - w->last);
    w->last = tick;
    if (e->d[0] == 0xFF)
    {
        int i;

        w->running = 0;             /* meta events cancel running status */
        for (i = 0; i < e->len; i++)
            Put(&w->o, e->d[i]);
        return;
    }
    if (e->d[0] != w->running)
        Put(&w->o, e->d[0]);
    w->running = e->d[0];
    Put(&w->o, e->d[1]);
    if (e->len > 2)
        Put(&w->o, e->d[2]);
}

/* Writes the list as a format 0 file with "division" ticks per quarter,
   thinning with "gap" ticks (0: no thinning).  With count_only it just
   counts the events it would write. */
static long Write(const evlist_t *l, int division, unsigned long gap,
                  int count_only, outbuf_t *out)
{
    writer_t       w;
    event_t        held[16][5];
    byte           isheld[16][5];
    unsigned long  sent[16][5];
    signed char    ctrl[16][128];
    long           i, tracklenpos = 0;
    int            c, s;

    memset(&w, 0, sizeof w);
    memset(isheld, 0, sizeof isheld);
    memset(sent, 0, sizeof sent);
    memset(ctrl, -1, sizeof ctrl);

    if (!count_only)
    {
        static const byte head[] = {
            'M','T','h','d', 0,0,0,6, 0,0, 0,1, 0,0,
            'M','T','r','k', 0,0,0,0 };
        for (i = 0; i < sizeof head; i++)
            Put(&w.o, head[i]);
        w.o.data[12] = (byte)(division >> 8);
        w.o.data[13] = (byte) division;
        tracklenpos = w.o.len - 4;
    }

#define EMIT(ev, tk)    do { if (count_only) w.count++; else WriteEvent(&w, (ev), (tk)); } while (0)
#define FLUSH(ch, tk, force) \
    for (s = 0; s < 5; s++) \
        if (isheld[ch][s] && ((force) || (tk) - sent[ch][s] >= gap)) \
        { \
            EMIT(&held[ch][s], (tk)); \
            sent[ch][s] = (tk); \
            isheld[ch][s] = 0; \
        }

    for (i = 0; i < l->n; i++)
    {
        const event_t *e = &l->ev[i];
        unsigned long  tk = e->tick;
        int            ch = e->d[0] & 15;

        if (gap)
            for (c = 0; c < 16; c++)
            {
                FLUSH(c, tk, 0);
            }

        if (e->d[0] == 0xFF)
        {
            EMIT(e, tk);
            continue;
        }
        if ((e->d[0] >> 4) == 0xB && e->d[1] < 120)
        {   /* a controller set to the value it has: nothing to write
               (120-127 are commands, such as all notes off) */
            if (ctrl[ch][e->d[1] & 0x7F] == (signed char) e->d[2])
                continue;
            ctrl[ch][e->d[1] & 0x7F] = e->d[2];
        }
        s = gap ? Slot(e) : -1;
        if (s >= 0)
        {
            if (tk - sent[ch][s] >= gap && !isheld[ch][s])
            {
                EMIT(e, tk);
                sent[ch][s] = tk;
            }
            else
            {
                held[ch][s] = *e;       /* the latest value wins */
                isheld[ch][s] = 1;
            }
            continue;
        }
        EMIT(e, tk);
    }
    {
        unsigned long end = l->n ? l->ev[l->n - 1].tick : 0;

        if (l->end > end)
            end = l->end;

        for (c = 0; c < 16; c++)
        {
            FLUSH(c, end, 1);
        }
        if (!count_only)
        {
            PutVarLen(&w.o, end - w.last);
            Put(&w.o, 0xFF); Put(&w.o, 0x2F); Put(&w.o, 0);
        }
    }
#undef FLUSH
#undef EMIT

    if (count_only)
        return w.count;
    if (w.o.failed)
    {
        free(w.o.data);
        return -1;
    }
    {
        long tl = w.o.len - tracklenpos - 4;

        w.o.data[tracklenpos]     = (byte)(tl >> 24);
        w.o.data[tracklenpos + 1] = (byte)(tl >> 16);
        w.o.data[tracklenpos + 2] = (byte)(tl >> 8);
        w.o.data[tracklenpos + 3] = (byte) tl;
    }
    *out = w.o;
    return w.count;
}

/* Writes the list, thinned as much as it takes to fit kMaxEvents.
   "tic" is 1/35 second in the list's ticks. */
static byte *Finish(evlist_t *l, int division, unsigned long tic, long *midlen)
{
    outbuf_t      o;
    unsigned long gap = 0;

    if (l->failed)
        return NULL;
    if (tic < 1)
        tic = 1;
    if (Write(l, division, 0, 1, NULL) > kMaxEvents)
        for (gap = tic; gap < tic * 64; gap *= 2)
            if (Write(l, division, gap, 1, NULL) <= kMaxEvents)
                break;
    if (Write(l, division, gap, 0, &o) < 0)
        return NULL;
    *midlen = o.len;
    return o.data;
}

/* ---- MUS ---- */

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
    evlist_t      l;
    const byte   *p, *end;
    int           scorestart, i;
    int           channelmap[16];
    int           nextchannel = 0;
    byte          velocity[16];
    unsigned long now = 0;
    int           done = 0;
    byte         *mid;
    static const byte tempo[6] = { 0xFF, 0x51, 3, 0x07, 0xA1, 0x20 };   /* 120 bpm */

    if (muslen < 16 || memcmp(mus, "MUS\x1A", 4))
        return NULL;
    scorestart = mus[6] | (mus[7] << 8);
    if (scorestart >= muslen)
        return NULL;

    memset(&l, 0, sizeof l);
    for (i = 0; i < 16; i++)
    {
        channelmap[i] = -1;
        velocity[i] = 127;
    }
    Add(&l, 0, tempo, 6);

    p = mus + scorestart;
    end = mus + muslen;
    while (!done && p < end && !l.failed)
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
                Add3(&l, now, 0x90 | mch, *p++ & 0x7F, 0, 3);   /* velocity 0: off */
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
                Add3(&l, now, 0x90 | mch, note & 0x7F, velocity[ch], 3);
                break;
            }

            case 2:     /* pitch wheel: 0-255, 128 = centre */
            {
                unsigned v;

                if (p >= end) { done = 1; break; }
                v = *p++ * 64;
                Add3(&l, now, 0xE0 | mch, v & 0x7F, (v >> 7) & 0x7F, 3);
                break;
            }

            case 3:     /* system event */
            {
                byte n;

                if (p >= end) { done = 1; break; }
                n = *p++ & 0x7F;
                if (n >= 10 && n <= 14)
                    Add3(&l, now, 0xB0 | mch, systemmap[n], 0, 3);
                break;
            }

            case 4:     /* controller */
            {
                byte n, v;

                if (p + 1 >= end) { done = 1; break; }
                n = *p++ & 0x7F;
                v = *p++ & 0x7F;
                if (n > 9)
                    break;              /* unknown: dropped */
                if (n == 0)
                    Add3(&l, now, 0xC0 | mch, v, 0, 2);     /* program change */
                else
                    Add3(&l, now, 0xB0 | mch, controllermap[n], v, 3);
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
            now += t;
        }
    }

    l.end = now;
    mid = Finish(&l, 70, 4, midlen);    /* 140 ticks per second: a tic is 4 */
    free(l.ev);
    return mid;
}

/* ---- standard MIDI files ---- */

static unsigned long Get32(const byte *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | (p[2] << 8) | p[3];
}

/* Reads a variable-length number; NULL past the end. */
static const byte *GetVarLen(const byte *p, const byte *end, unsigned long *v)
{
    *v = 0;
    while (p < end)
    {
        byte b = *p++;

        *v = (*v << 7) | (b & 0x7F);
        if (!(b & 0x80))
            return p;
    }
    return NULL;
}

/* Stable merge sort by tick: events at the same tick keep their order. */
static int SortByTick(event_t *a, long n)
{
    event_t *tmp, *src = a, *dst;
    long     width, i;

    if (n < 2)
        return 1;
    if (!(tmp = (event_t *) malloc(n * sizeof(event_t))))
        return 0;
    dst = tmp;
    for (width = 1; width < n; width *= 2)
    {
        for (i = 0; i < n; i += 2 * width)
        {
            long lo = i, mid = i + width, hi = i + 2 * width, x, y, k;

            if (mid > n) mid = n;
            if (hi > n) hi = n;
            for (x = lo, y = mid, k = lo; k < hi; k++)
                dst[k] = (x < mid && (y >= hi || src[x].tick <= src[y].tick))
                         ? src[x++] : src[y++];
        }
        { event_t *t = src; src = dst; dst = t; }
    }
    if (src != a)
        memcpy(a, src, n * sizeof(event_t));
    free(tmp);
    return 1;
}

/*
 * Reads a standard MIDI file (format 0 or 1) into one list of events,
 * merged in time; tempo changes are kept, other meta events and sysex are
 * left out.  Returns 0 if it isn't a MIDI file this can read.
 */
static int ReadMidi(const byte *mid, long len, evlist_t *l, int *division, long *events)
{
    const byte *p = mid, *end = mid + len;
    int         tracks, t;
    long        total = 0;

    memset(l, 0, sizeof *l);
    if (len < 14 || memcmp(p, "MThd", 4) || Get32(p + 4) < 6)
        return 0;
    tracks = (p[10] << 8) | p[11];
    *division = (p[12] << 8) | p[13];
    if (*division & 0x8000)
        return 0;                           /* SMPTE time: left alone */
    p += 8 + Get32(p + 4);

    for (t = 0; t < tracks && p + 8 <= end; t++)
    {
        const byte   *q, *tend;
        unsigned long tick = 0, v;
        byte          running = 0;

        if (memcmp(p, "MTrk", 4))
            break;
        q = p + 8;
        tend = q + Get32(p + 4);
        if (tend > end)
            tend = end;
        p = tend;

        while (q < tend)
        {
            byte st;

            if (!(q = GetVarLen(q, tend, &v)) || q >= tend)
                break;
            tick += v;
            st = *q;
            if (st & 0x80)
            {
                q++;
                if (st < 0xF0)
                    running = st;
            }
            else
                st = running;
            total++;

            if (st == 0xFF)
            {
                byte type;

                if (q >= tend)
                    break;
                type = *q++;
                if (!(q = GetVarLen(q, tend, &v)) || q + v > tend)
                    break;
                if (type == 0x51 && v == 3)
                {
                    byte d[6] = { 0xFF, 0x51, 3, q[0], q[1], q[2] };
                    Add(l, tick, d, 6);
                }
                q += v;
                if (type == 0x2F)
                {
                    if (tick > l->end)
                        l->end = tick;
                    break;
                }
            }
            else if (st == 0xF0 || st == 0xF7)
            {
                if (!(q = GetVarLen(q, tend, &v)) || q + v > tend)
                    break;
                q += v;                     /* sysex: left out */
            }
            else if (st >= 0x80)
            {
                int n = ((st >> 4) == 0xC || (st >> 4) == 0xD) ? 1 : 2;

                if (q + n > tend)
                    break;
                if ((st >> 4) == 0x8)       /* note off -> note on, velocity 0 */
                    Add3(l, tick, 0x90 | (st & 15), q[0] & 0x7F, 0, 3);
                else
                    Add3(l, tick, st, q[0] & 0x7F, n > 1 ? q[1] & 0x7F : 0, 1 + n);
                q += n;
            }
            else
                break;                      /* data byte without a status */
        }
    }
    if (l->failed)
        return 0;
    if (tracks > 1 && !SortByTick(l->ev, l->n))     /* merge the tracks in time */
    {
        free(l->ev);
        return 0;
    }
    *events = total;
    return 1;
}

/* Number of events in a standard MIDI file (-1 if it can't be read). */
long MIDI_CountEvents(const byte *mid, long len)
{
    evlist_t l;
    int      division;
    long     events;

    if (!ReadMidi(mid, len, &l, &division, &events))
        return -1;
    free(l.ev);
    return events;
}

/*
 * Rewrites a standard MIDI file that has more events than QuickTime can
 * import.  Returns a malloc'd file (free() it), or NULL if the file is fine
 * as it is or can't be read.
 */
byte *MIDI_Fit(const byte *mid, long len, long *midlen)
{
    evlist_t l;
    int      division;
    long     events;
    byte    *out;

    if (!ReadMidi(mid, len, &l, &division, &events))
        return NULL;
    if (events <= kMaxEvents)
    {
        free(l.ev);
        return NULL;
    }
    /* a tic (1/35 s) at 120 bpm: division * 2 ticks per second */
    out = Finish(&l, division, (unsigned long)division * 2 / 35, midlen);
    free(l.ev);
    return out;
}
