/*
 * UMAPINFO support for the Retro68 build of Lion's DOOM.
 *
 * UMAPINFO (Christoph Oelckers' spec, 2017) is the lump modern WADs use to
 * describe their levels: names, which level comes next, skies, music,
 * intermission and finale texts, episode menu entries and boss actions.
 * id's 2024 re-release ships its add-ons with it (No Rest for the Living,
 * the Master Levels, SIGIL), and so do many community WADs.
 *
 * Supported keys: levelname, label, author, levelpic, next, nextsecret,
 * skytexture, music, exitpic, enterpic, partime, nointermission, endgame,
 * endpic, endbunny, endcast, intertext, intertextsecret, interbackdrop,
 * intermusic, episode, bossaction.  Unknown keys (such as the KEX engine's
 * kex_* ones) are skipped.  Every UMAPINFO lump is read, in load order; a
 * later definition of a map replaces the keys it sets.
 *
 * Semantics follow PrBoom+'s implementation (umapinfo.cpp, GPL).
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "p_local.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ------------------------------------------------------------------------ */
/* Data                                                                     */

#define MAXMAPINFO      128
#define MAXBOSSACTIONS  8

static umapinfo_t   sMaps[MAXMAPINFO];
static int          sNumMaps;

umapepisode_t       umi_episodes[MAXUMIEPISODES];
int                 umi_numepisodes;
Boolean             umi_episodesdefined;    /* an "episode" key was seen */

umapinfo_t          *gamemapinfo;           /* the level being played */

/* All strings live in blocks freed together when the WADs change. */
typedef struct block_s { struct block_s *next; } block_t;
static block_t *sBlocks;

static void *Alloc(size_t n)
{
    block_t *b = (block_t *) NewPtrClear(sizeof(block_t) + n);

    if (!b)
        I_Error("UMAPINFO: out of memory");
    b->next = sBlocks;
    sBlocks = b;
    return b + 1;
}

static char *Strdup(const char *s)
{
    return strcpy(Alloc(strlen(s) + 1), s);
}

static void FreeAll(void)
{
    while (sBlocks)
    {
        block_t *n = sBlocks->next;
        DisposePtr((Ptr) sBlocks);
        sBlocks = n;
    }
    memset(sMaps, 0, sizeof sMaps);
    sNumMaps = 0;
    memset(umi_episodes, 0, sizeof umi_episodes);
    umi_numepisodes = 0;
    umi_episodesdefined = false;
    gamemapinfo = NULL;
}

/* ------------------------------------------------------------------------ */
/* Scanner                                                                  */

enum { TK_EOF, TK_IDENT, TK_STRING, TK_NUMBER, TK_CHAR };

typedef struct
{
    const char *p, *end;
    int         type;
    char        text[1024];
    long        number;
    int         ch;
    int         line;
    Boolean     failed;
} scanner_t;

static void SkipSpace(scanner_t *s)
{
    for (;;)
    {
        while (s->p < s->end && isspace((unsigned char) *s->p))
        {
            if (*s->p == '\n')
                s->line++;
            s->p++;
        }
        if (s->p + 1 < s->end && s->p[0] == '/' && s->p[1] == '/')
        {
            while (s->p < s->end && *s->p != '\n')
                s->p++;
            continue;
        }
        if (s->p + 1 < s->end && s->p[0] == '/' && s->p[1] == '*')
        {
            s->p += 2;
            while (s->p + 1 < s->end && !(s->p[0] == '*' && s->p[1] == '/'))
            {
                if (*s->p == '\n')
                    s->line++;
                s->p++;
            }
            s->p += 2;
            continue;
        }
        break;
    }
}

static void Next(scanner_t *s)
{
    int n = 0;

    SkipSpace(s);
    if (s->p >= s->end)
    {
        s->type = TK_EOF;
        return;
    }
    if (*s->p == '"')
    {
        s->p++;
        while (s->p < s->end && *s->p != '"')
        {
            char c = *s->p++;

            if (c == '\\' && s->p < s->end)
            {
                c = *s->p++;
                if (c == 'n')
                    c = '\n';
            }
            if (c == '\r')
                continue;
            if (n < (int) sizeof s->text - 1)
                s->text[n++] = c;
        }
        s->p++;
        s->text[n] = 0;
        s->type = TK_STRING;
        return;
    }
    if (isdigit((unsigned char) *s->p) || (*s->p == '-' && s->p + 1 < s->end &&
                                            isdigit((unsigned char) s->p[1])))
    {
        s->number = strtol(s->p, (char **) &s->p, 0);
        s->type = TK_NUMBER;
        return;
    }
    if (isalpha((unsigned char) *s->p) || *s->p == '_')
    {
        while (s->p < s->end && (isalnum((unsigned char) *s->p) || *s->p == '_'))
        {
            if (n < (int) sizeof s->text - 1)
                s->text[n++] = *s->p;
            s->p++;
        }
        s->text[n] = 0;
        s->type = TK_IDENT;
        return;
    }
    s->ch = *s->p++;
    s->type = TK_CHAR;
}

static Boolean IsChar(scanner_t *s, int c)
{
    return s->type == TK_CHAR && s->ch == c;
}

/* ------------------------------------------------------------------------ */
/* Parser                                                                   */

static const char *const actornames[] =
{
    "DoomPlayer", "ZombieMan", "ShotgunGuy", "Archvile", "ArchvileFire",
    "Revenant", "RevenantTracer", "RevenantTracerSmoke", "Fatso", "FatShot",
    "ChaingunGuy", "DoomImp", "Demon", "Spectre", "Cacodemon", "BaronOfHell",
    "BaronBall", "HellKnight", "LostSoul", "SpiderMastermind", "Arachnotron",
    "Cyberdemon", "PainElemental", "WolfensteinSS", "CommanderKeen",
    "BossBrain", "BossEye", "BossTarget", "SpawnShot", "SpawnFire",
    "ExplosiveBarrel"
};                                  /* in mobjtype_t order (INFO.H) */
#define NUMACTORNAMES (sizeof actornames / sizeof actornames[0])

static void CopyLump(char *dst, const char *src)
{
    int i;

    for (i = 0; i < 8 && src[i]; i++)
        dst[i] = toupper((unsigned char) src[i]);
    dst[i] = 0;
}

/* "E1M1" / "MAP01" -> episode, map; false if neither. */
Boolean UMI_ParseMapName(const char *name, int *episode, int *map)
{
    int e, m;

    if ((name[0] == 'E' || name[0] == 'e') && (name[2] == 'M' || name[2] == 'm') &&
        isdigit((unsigned char) name[1]) && isdigit((unsigned char) name[3]) && !name[4])
    {
        e = name[1] - '0';
        m = name[3] - '0';
    }
    else if (!strncasecmp(name, "MAP", 3) && isdigit((unsigned char) name[3]) &&
             isdigit((unsigned char) name[4]) && !name[5])
    {
        e = 1;
        m = (name[3] - '0') * 10 + name[4] - '0';
    }
    else
        return false;
    if (episode)
        *episode = e;
    if (map)
        *map = m;
    return true;
}

/* A value that is a string, or 'clear'.  Several strings separated by
   commas make lines of one text. */
static char *MultiString(scanner_t *s)
{
    char buf[4096];
    int  n = 0;

    Next(s);
    if (s->type == TK_IDENT && !strcasecmp(s->text, "clear"))
    {
        Next(s);
        return Strdup("-");         /* removes a default text */
    }
    buf[0] = 0;
    while (s->type == TK_STRING)
    {
        int l = strlen(s->text);

        if (n && n < (int) sizeof buf - 1)
            buf[n++] = '\n';
        if (n + l > (int) sizeof buf - 1)
            l = sizeof buf - 1 - n;
        memcpy(buf + n, s->text, l);
        n += l;
        buf[n] = 0;
        Next(s);
        if (!IsChar(s, ','))
            break;
        Next(s);
    }
    return Strdup(buf);
}

/* One "key = value" inside a map block.  Leaves the scanner on the token
   after the value. */
static void Property(scanner_t *s, umapinfo_t *m)
{
    char key[64];

    strncpy(key, s->text, sizeof key - 1);
    key[sizeof key - 1] = 0;
    Next(s);
    if (!IsChar(s, '='))
    {
        s->failed = true;
        return;
    }

    if (!strcasecmp(key, "intertext"))
    {
        m->intertext = MultiString(s);
        return;
    }
    if (!strcasecmp(key, "intertextsecret"))
    {
        m->intertextsecret = MultiString(s);
        return;
    }

    Next(s);

    if (!strcasecmp(key, "levelname") && s->type == TK_STRING)
        m->levelname = Strdup(s->text);
    else if (!strcasecmp(key, "label"))
        m->label = Strdup(s->type == TK_IDENT && !strcasecmp(s->text, "clear") ?
                          "-" : s->text);
    else if (!strcasecmp(key, "author") && s->type == TK_STRING)
        m->author = Strdup(s->text);
    else if (!strcasecmp(key, "levelpic"))
        CopyLump(m->levelpic, s->text);
    else if (!strcasecmp(key, "next"))
        CopyLump(m->nextmap, s->text);
    else if (!strcasecmp(key, "nextsecret"))
        CopyLump(m->nextsecret, s->text);
    else if (!strcasecmp(key, "skytexture"))
        CopyLump(m->skytexture, s->text);
    else if (!strcasecmp(key, "music"))
        CopyLump(m->music, s->text);
    else if (!strcasecmp(key, "exitpic"))
        CopyLump(m->exitpic, s->text);
    else if (!strcasecmp(key, "enterpic"))
        CopyLump(m->enterpic, s->text);
    else if (!strcasecmp(key, "endpic"))
        CopyLump(m->endpic, s->text);
    else if (!strcasecmp(key, "interbackdrop"))
        CopyLump(m->interbackdrop, s->text);
    else if (!strcasecmp(key, "intermusic"))
        CopyLump(m->intermusic, s->text);
    else if (!strcasecmp(key, "partime") && s->type == TK_NUMBER)
        m->partime = 35 * s->number;
    else if (!strcasecmp(key, "nointermission"))
        m->nointermission = !strcasecmp(s->text, "true");
    else if (!strcasecmp(key, "endgame") || !strcasecmp(key, "endbunny") ||
             !strcasecmp(key, "endcast"))
    {
        if (strcasecmp(s->text, "true"))
            strcpy(m->endpic, "-");
        else if (!strcasecmp(key, "endgame"))
            strcpy(m->endpic, "!");
        else if (!strcasecmp(key, "endbunny"))
            strcpy(m->endpic, "$BUNNY");
        else
            strcpy(m->endpic, "$CAST");
    }
    else if (!strcasecmp(key, "episode"))
    {
        umi_episodesdefined = true;
        if (s->type == TK_IDENT && !strcasecmp(s->text, "clear"))
            umi_numepisodes = 0;
        else if (umi_numepisodes < MAXUMIEPISODES)
        {
            umapepisode_t *e = &umi_episodes[umi_numepisodes++];

            memset(e, 0, sizeof *e);
            CopyLump(e->patch, s->text);
            strncpy(e->map, m->mapname, 8);
            if (s->p < s->end)
            {
                Next(s);
                if (IsChar(s, ','))
                {
                    Next(s);
                    strncpy(e->name, s->text, sizeof e->name - 1);
                    Next(s);
                    if (IsChar(s, ','))
                    {
                        Next(s);
                        e->key = tolower((unsigned char) s->text[0]);
                        Next(s);
                    }
                }
                return;             /* already on the next token */
            }
        }
    }
    else if (!strcasecmp(key, "bossaction"))
    {
        if (s->type == TK_IDENT && !strcasecmp(s->text, "clear"))
            m->numbossactions = -1;         /* no boss actions at all */
        else
        {
            int i, type = -1;

            for (i = 0; i < NUMACTORNAMES; i++)
                if (!strcasecmp(s->text, actornames[i]))
                    type = i;
            Next(s);
            if (IsChar(s, ','))
            {
                int special, tag;

                Next(s);
                special = s->number;
                Next(s);
                Next(s);
                tag = s->number;
                if (type >= 0 && (tag != 0 || special == 11 || special == 51 ||
                                  special == 52 || special == 124))
                {
                    if (m->numbossactions < 0)
                        m->numbossactions = 0;
                    if (m->numbossactions < MAXBOSSACTIONS)
                    {
                        if (!m->bossactions)
                            m->bossactions = Alloc(MAXBOSSACTIONS * sizeof(umbossaction_t));
                        m->bossactions[m->numbossactions].type = type;
                        m->bossactions[m->numbossactions].special = special;
                        m->bossactions[m->numbossactions].tag = tag;
                        m->numbossactions++;
                    }
                }
            }
            else
                return;
        }
    }
    /* anything else (kex_*, unknown keys): the value is skipped below */

    /* skip the rest of the value: more comma-separated tokens */
    Next(s);
    while (IsChar(s, ','))
    {
        Next(s);
        Next(s);
    }
}

static umapinfo_t *FindOrAdd(const char *name)
{
    int i;

    for (i = 0; i < sNumMaps; i++)
        if (!strcasecmp(sMaps[i].mapname, name))
            return &sMaps[i];
    if (sNumMaps == MAXMAPINFO)
        return NULL;
    memset(&sMaps[sNumMaps], 0, sizeof sMaps[0]);
    CopyLump(sMaps[sNumMaps].mapname, name);
    return &sMaps[sNumMaps++];
}

static void Parse(const char *text, long length)
{
    scanner_t    s;
    umapinfo_t   dummy;

    memset(&s, 0, sizeof s);
    s.p = text;
    s.end = text + length;
    s.line = 1;
    Next(&s);
    while (s.type != TK_EOF && !s.failed)
    {
        umapinfo_t *m;

        if (s.type != TK_IDENT || strcasecmp(s.text, "map"))
        {
            Next(&s);               /* junk: resynchronise on "map" */
            continue;
        }
        Next(&s);
        m = FindOrAdd(s.text);
        if (!m)
            m = &dummy;
        Next(&s);
        if (!IsChar(&s, '{'))
            continue;
        Next(&s);
        while (s.type != TK_EOF && !IsChar(&s, '}') && !s.failed)
        {
            if (s.type == TK_IDENT)
                Property(&s, m);
            else
                Next(&s);
        }
        Next(&s);
    }
}

/* ------------------------------------------------------------------------ */

/* Called after W_InitMultipleFiles (and DEH_Init). */
void UMI_Init(void)
{
    int i;

    FreeAll();
    for (i = 0; i < numlumps; i++)
        if (!strncasecmp(lumpinfo[i].name, "UMAPINFO", 8) && lumpinfo[i].size > 0)
        {
            char *buf = NewPtr(lumpinfo[i].size);

            if (buf)
            {
                W_ReadLump(i, buf);
                Parse(buf, lumpinfo[i].size);
                DisposePtr(buf);
            }
        }
}

umapinfo_t *UMI_Lookup(int episode, int map)
{
    char name[9];
    int  i;

    if (!sNumMaps)
        return NULL;
    if (commercial)
        sprintf(name, "MAP%02d", map);
    else
        sprintf(name, "E%dM%d", episode, map);
    for (i = 0; i < sNumMaps; i++)
        if (!strcasecmp(sMaps[i].mapname, name))
            return &sMaps[i];
    return NULL;
}

Boolean UMI_Active(void)
{
    return sNumMaps > 0;
}
