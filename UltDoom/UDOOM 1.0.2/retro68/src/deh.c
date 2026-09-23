/*
 * DeHackEd and BEX support for the Retro68 build of Lion's DOOM.
 *
 * Reads the patches that 1990s mods were distributed as:
 *
 *   - DeHackEd 1.2 - 3.0 text patches (.DEH): Thing, Frame, Pointer, Ammo,
 *     Weapon, Misc, Cheat (ignored), Sound (ignored) and Text blocks;
 *   - the Boom "BEX" extensions: [STRINGS], [PARS], [CODEPTR], [SPRITES],
 *     [SOUNDS], [MUSIC] and mnemonic thing flags ("Bits = SOLID+SHOOTABLE").
 *
 * Patches come from DEHACKED lumps in the loaded WADs (in load order) and
 * from .DEH / .BEX files added like WADs.  Every table a patch can change is
 * snapshotted on the first load and restored before patching again, so the
 * WADs menu can reload a different set in place.
 *
 * Modelled on Chocolate / Crispy DOOM's DeHackEd code (Simon Howard, Fabian
 * Greffrath; GPL), rewritten for Lion's tables.
 */
#include "LionDoom.h"
#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"
#include "sounds.h"
#include "soundst.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>

extern FSSpec   gWadFiles[MAXWADFILES];
extern int      gNumWads;
extern weaponinfo_t weaponinfo[NUMWEAPONS];
extern int      maxammo[NUMAMMO];
extern int      clipammo[NUMAMMO];
extern int      pars[5][10];
extern int      cpars[32];
extern musicinfo_t S_music[];
extern sfxinfo_t   S_sfx[];

/* ---- Misc block values (DeHackEd "Misc 0"), used by the game code ---- */

int deh_initial_health      = 100;
int deh_initial_bullets     = 50;
int deh_max_health          = 200;      /* health bonus cap */
int deh_max_armor           = 200;      /* armor bonus cap */
int deh_green_armor_class   = 1;
int deh_blue_armor_class    = 2;
int deh_max_soulsphere      = 200;
int deh_soulsphere_health   = 100;
int deh_megasphere_health   = 200;
int deh_god_mode_health     = 100;
int deh_idfa_armor          = 200;
int deh_idfa_armor_class    = 2;
int deh_idkfa_armor         = 200;
int deh_idkfa_armor_class   = 2;
int deh_bfg_cells_per_shot  = 40;
int deh_species_infighting  = 0;

static struct { const char *name; int *value; int def; } misc_settings[] =
{
    { "Initial Health",     &deh_initial_health,     100 },
    { "Initial Bullets",    &deh_initial_bullets,    50 },
    { "Max Health",         &deh_max_health,         200 },
    { "Max Armor",          &deh_max_armor,          200 },
    { "Green Armor Class",  &deh_green_armor_class,  1 },
    { "Blue Armor Class",   &deh_blue_armor_class,   2 },
    { "Max Soulsphere",     &deh_max_soulsphere,     200 },
    { "Soulsphere Health",  &deh_soulsphere_health,  100 },
    { "Megasphere Health",  &deh_megasphere_health,  200 },
    { "God Mode Health",    &deh_god_mode_health,    100 },
    { "IDFA Armor",         &deh_idfa_armor,         200 },
    { "IDFA Armor Class",   &deh_idfa_armor_class,   2 },
    { "IDKFA Armor",        &deh_idkfa_armor,        200 },
    { "IDKFA Armor Class",  &deh_idkfa_armor_class,  2 },
    { "BFG Cells/Shot",     &deh_bfg_cells_per_shot, 40 },
};
#define NUMMISC (sizeof misc_settings / sizeof misc_settings[0])

/* ---- Snapshot of the unpatched tables ---- */

static Boolean      sSaved;
static state_t      sStates[NUMSTATES];
static mobjinfo_t   sMobjinfo[NUMMOBJTYPES];
static weaponinfo_t sWeaponinfo[NUMWEAPONS];
static int          sMaxammo[NUMAMMO], sClipammo[NUMAMMO];
static char        *sSprnames[NUMSPRITES];
static char        *sMusicNames[NUMMUSIC];
static char        *sSfxNames[NUMSFX];
static int          sPars[5][10], sCpars[32];

static void SaveTables(void)
{
    int i;

    memcpy(sStates, states, sizeof sStates);
    memcpy(sMobjinfo, mobjinfo, sizeof sMobjinfo);
    memcpy(sWeaponinfo, weaponinfo, sizeof sWeaponinfo);
    memcpy(sMaxammo, maxammo, sizeof sMaxammo);
    memcpy(sClipammo, clipammo, sizeof sClipammo);
    memcpy(sPars, pars, sizeof sPars);
    memcpy(sCpars, cpars, sizeof sCpars);
    for (i = 0; i < NUMSPRITES; i++)
        sSprnames[i] = sprnames[i];
    for (i = 0; i < NUMMUSIC; i++)
        sMusicNames[i] = S_music[i].name;
    for (i = 0; i < NUMSFX; i++)
        sSfxNames[i] = S_sfx[i].name;
    sSaved = true;
}

static void RestoreTables(void)
{
    int i;

    memcpy(states, sStates, sizeof sStates);
    memcpy(mobjinfo, sMobjinfo, sizeof sMobjinfo);
    memcpy(weaponinfo, sWeaponinfo, sizeof sWeaponinfo);
    memcpy(maxammo, sMaxammo, sizeof sMaxammo);
    memcpy(clipammo, sClipammo, sizeof sClipammo);
    memcpy(pars, sPars, sizeof sPars);
    memcpy(cpars, sCpars, sizeof sCpars);
    for (i = 0; i < NUMSPRITES; i++)
        sprnames[i] = sSprnames[i];
    for (i = 0; i < NUMMUSIC; i++)
        S_music[i].name = sMusicNames[i];
    for (i = 0; i < NUMSFX; i++)
        S_sfx[i].name = sSfxNames[i];
    for (i = 0; i < NUMMISC; i++)
        *misc_settings[i].value = misc_settings[i].def;
    deh_species_infighting = 0;
}

/* ---- Replacement strings (Text blocks and BEX [STRINGS]) ---- */

typedef struct deh_str_s
{
    struct deh_str_s *next;
    const char       *from;
    char             *to;
} deh_str_t;

#define STRHASH 64
static deh_str_t *sStrings[STRHASH];

/* Strings live in a pool that is dropped as a whole on reload. */
typedef struct pool_s { struct pool_s *next; } pool_t;
static pool_t *sPool;

static void *PoolAlloc(size_t n)
{
    pool_t *p = (pool_t *) NewPtr(sizeof(pool_t) + n);

    if (!p)
        I_Error("DeHackEd: out of memory");
    p->next = sPool;
    sPool = p;
    return p + 1;
}

static char *PoolStrdup(const char *s)
{
    return strcpy(PoolAlloc(strlen(s) + 1), s);
}

static void PoolFree(void)
{
    while (sPool)
    {
        pool_t *n = sPool->next;
        DisposePtr((Ptr) sPool);
        sPool = n;
    }
    memset(sStrings, 0, sizeof sStrings);
}

static unsigned StrHash(const char *s)
{
    unsigned h = 0;

    while (*s)
        h = h * 33 + (unsigned char) *s++;
    return h % STRHASH;
}

static int sNumReplacements;

static void AddReplacement(const char *from, const char *to)
{
    unsigned   h = StrHash(from);
    deh_str_t *e;

    for (e = sStrings[h]; e; e = e->next)
        if (!strcmp(e->from, from))
        {
            e->to = PoolStrdup(to);
            return;
        }
    e = PoolAlloc(sizeof *e);
    e->from = PoolStrdup(from);
    e->to = PoolStrdup(to);
    e->next = sStrings[h];
    sStrings[h] = e;
    sNumReplacements++;
}

/* The string to show in place of one of the game's own. */
const char *DEH_String(const char *s)
{
    deh_str_t *e;

    if (!s || !sNumReplacements)
        return s;
    for (e = sStrings[StrHash(s)]; e; e = e->next)
        if (!strcmp(e->from, s))
            return e->to;
    return s;
}

/* BEX [STRINGS] mnemonics: the names of the texts in DSTRINGS.H. */
#define S(n) { #n, n },
static const struct { const char *name; const char *text; } bexstrings[] =
{
    S(PRESSKEY) S(PRESSYN) S(QUITMSG) S(LOADNET) S(QLOADNET) S(QSAVESPOT)
    S(SAVEDEAD) S(QSPROMPT) S(QLPROMPT) S(NEWGAME) S(NIGHTMARE) S(SWSTRING)
    S(MSGOFF) S(MSGON) S(NETEND) S(ENDGAME) S(DETAILHI) S(DETAILLO)
    S(GAMMALVL0) S(GAMMALVL1) S(GAMMALVL2) S(GAMMALVL3) S(GAMMALVL4) S(EMPTYSTRING)
    S(GOTARMOR) S(GOTMEGA) S(GOTHTHBONUS) S(GOTARMBONUS) S(GOTSTIM) S(GOTMEDINEED)
    S(GOTMEDIKIT) S(GOTSUPER) S(GOTBLUECARD) S(GOTYELWCARD) S(GOTREDCARD)
    S(GOTBLUESKUL) S(GOTYELWSKUL) S(GOTREDSKULL) S(GOTINVUL) S(GOTBERSERK)
    S(GOTINVIS) S(GOTSUIT) S(GOTMAP) S(GOTVISOR) S(GOTMSPHERE) S(GOTCLIP)
    S(GOTCLIPBOX) S(GOTROCKET) S(GOTROCKBOX) S(GOTCELL) S(GOTCELLBOX) S(GOTSHELLS)
    S(GOTSHELLBOX) S(GOTBACKPACK) S(GOTBFG9000) S(GOTCHAINGUN) S(GOTCHAINSAW)
    S(GOTLAUNCHER) S(GOTPLASMA) S(GOTSHOTGUN) S(GOTSHOTGUN2)
    S(PD_BLUEO) S(PD_REDO) S(PD_YELLOWO) S(PD_BLUEK) S(PD_REDK) S(PD_YELLOWK)
    S(GGSAVED) S(HUSTR_MSGU)
    S(HUSTR_E1M1) S(HUSTR_E1M2) S(HUSTR_E1M3) S(HUSTR_E1M4) S(HUSTR_E1M5)
    S(HUSTR_E1M6) S(HUSTR_E1M7) S(HUSTR_E1M8) S(HUSTR_E1M9)
    S(HUSTR_E2M1) S(HUSTR_E2M2) S(HUSTR_E2M3) S(HUSTR_E2M4) S(HUSTR_E2M5)
    S(HUSTR_E2M6) S(HUSTR_E2M7) S(HUSTR_E2M8) S(HUSTR_E2M9)
    S(HUSTR_E3M1) S(HUSTR_E3M2) S(HUSTR_E3M3) S(HUSTR_E3M4) S(HUSTR_E3M5)
    S(HUSTR_E3M6) S(HUSTR_E3M7) S(HUSTR_E3M8) S(HUSTR_E3M9)
    S(HUSTR_E4M1) S(HUSTR_E4M2) S(HUSTR_E4M3) S(HUSTR_E4M4) S(HUSTR_E4M5)
    S(HUSTR_E4M6) S(HUSTR_E4M7) S(HUSTR_E4M8) S(HUSTR_E4M9)
    S(HUSTR_1) S(HUSTR_2) S(HUSTR_3) S(HUSTR_4) S(HUSTR_5) S(HUSTR_6) S(HUSTR_7)
    S(HUSTR_8) S(HUSTR_9) S(HUSTR_10) S(HUSTR_11) S(HUSTR_12) S(HUSTR_13)
    S(HUSTR_14) S(HUSTR_15) S(HUSTR_16) S(HUSTR_17) S(HUSTR_18) S(HUSTR_19)
    S(HUSTR_20) S(HUSTR_21) S(HUSTR_22) S(HUSTR_23) S(HUSTR_24) S(HUSTR_25)
    S(HUSTR_26) S(HUSTR_27) S(HUSTR_28) S(HUSTR_29) S(HUSTR_30) S(HUSTR_31)
    S(HUSTR_32)
    S(PHUSTR_1) S(PHUSTR_2) S(PHUSTR_3) S(PHUSTR_4) S(PHUSTR_5) S(PHUSTR_6)
    S(PHUSTR_7) S(PHUSTR_8) S(PHUSTR_9) S(PHUSTR_10) S(PHUSTR_11) S(PHUSTR_12)
    S(PHUSTR_13) S(PHUSTR_14) S(PHUSTR_15) S(PHUSTR_16) S(PHUSTR_17) S(PHUSTR_18)
    S(PHUSTR_19) S(PHUSTR_20) S(PHUSTR_21) S(PHUSTR_22) S(PHUSTR_23) S(PHUSTR_24)
    S(PHUSTR_25) S(PHUSTR_26) S(PHUSTR_27) S(PHUSTR_28) S(PHUSTR_29) S(PHUSTR_30)
    S(PHUSTR_31) S(PHUSTR_32)
    S(THUSTR_1) S(THUSTR_2) S(THUSTR_3) S(THUSTR_4) S(THUSTR_5) S(THUSTR_6)
    S(THUSTR_7) S(THUSTR_8) S(THUSTR_9) S(THUSTR_10) S(THUSTR_11) S(THUSTR_12)
    S(THUSTR_13) S(THUSTR_14) S(THUSTR_15) S(THUSTR_16) S(THUSTR_17) S(THUSTR_18)
    S(THUSTR_19) S(THUSTR_20) S(THUSTR_21) S(THUSTR_22) S(THUSTR_23) S(THUSTR_24)
    S(THUSTR_25) S(THUSTR_26) S(THUSTR_27) S(THUSTR_28) S(THUSTR_29) S(THUSTR_30)
    S(THUSTR_31) S(THUSTR_32)
    S(HUSTR_CHATMACRO1) S(HUSTR_CHATMACRO2) S(HUSTR_CHATMACRO3) S(HUSTR_CHATMACRO4)
    S(HUSTR_CHATMACRO5) S(HUSTR_CHATMACRO6) S(HUSTR_CHATMACRO7) S(HUSTR_CHATMACRO8)
    S(HUSTR_CHATMACRO9) S(HUSTR_CHATMACRO0)
    S(HUSTR_TALKTOSELF1) S(HUSTR_TALKTOSELF2) S(HUSTR_TALKTOSELF3)
    S(HUSTR_TALKTOSELF4) S(HUSTR_TALKTOSELF5) S(HUSTR_MESSAGESENT)
    S(AMSTR_FOLLOWON) S(AMSTR_FOLLOWOFF) S(AMSTR_GRIDON) S(AMSTR_GRIDOFF)
    S(AMSTR_MARKEDSPOT) S(AMSTR_MARKSCLEARED)
    S(STSTR_MUS) S(STSTR_NOMUS) S(STSTR_DQDON) S(STSTR_DQDOFF) S(STSTR_KFAADDED)
    S(STSTR_FAADDED) S(STSTR_NCON) S(STSTR_NCOFF) S(STSTR_BEHOLD) S(STSTR_BEHOLDX)
    S(STSTR_CHOPPERS) S(STSTR_CLEV)
    S(E1TEXT) S(E2TEXT) S(E3TEXT) S(E4TEXT)
    S(C1TEXT) S(C2TEXT) S(C3TEXT) S(C4TEXT) S(C5TEXT) S(C6TEXT)
    S(P1TEXT) S(P2TEXT) S(P3TEXT) S(P4TEXT) S(P5TEXT) S(P6TEXT)
    S(T1TEXT) S(T2TEXT) S(T3TEXT) S(T4TEXT) S(T5TEXT) S(T6TEXT)
    S(CC_ZOMBIE) S(CC_SHOTGUN) S(CC_HEAVY) S(CC_IMP) S(CC_DEMON) S(CC_LOST)
    S(CC_CACO) S(CC_HELL) S(CC_BARON) S(CC_ARACH) S(CC_PAIN) S(CC_REVEN)
    S(CC_MANCU) S(CC_ARCH) S(CC_SPIDER) S(CC_CYBER) S(CC_HERO)
};
#undef S
#define NUMBEXSTRINGS (sizeof bexstrings / sizeof bexstrings[0])

/* ---- Code pointers, by BEX name ---- */

#define A(n) void A_##n();
A(Light0) A(WeaponReady) A(Lower) A(Raise) A(Punch) A(ReFire) A(FirePistol)
A(Light1) A(FireShotgun) A(Light2) A(FireShotgun2) A(CheckReload)
A(OpenShotgun2) A(LoadShotgun2) A(CloseShotgun2) A(FireCGun) A(GunFlash)
A(FireMissile) A(Saw) A(FirePlasma) A(BFGsound) A(FireBFG) A(BFGSpray)
A(Explode) A(Pain) A(PlayerScream) A(Fall) A(XScream) A(Look) A(Chase)
A(FaceTarget) A(PosAttack) A(Scream) A(SPosAttack) A(VileChase) A(VileStart)
A(VileTarget) A(VileAttack) A(StartFire) A(Fire) A(FireCrackle) A(Tracer)
A(SkelWhoosh) A(SkelFist) A(SkelMissile) A(FatRaise) A(FatAttack1)
A(FatAttack2) A(FatAttack3) A(BossDeath) A(CPosAttack) A(CPosRefire)
A(TroopAttack) A(SargAttack) A(HeadAttack) A(BruisAttack) A(SkullAttack)
A(Metal) A(SpidRefire) A(BabyMetal) A(BspiAttack) A(Hoof) A(CyberAttack)
A(PainAttack) A(PainDie) A(KeenDie) A(BrainPain) A(BrainScream) A(BrainDie)
A(BrainAwake) A(BrainSpit) A(SpawnSound) A(SpawnFly) A(BrainExplode)
#undef A

#define A(n) { #n, A_##n },
static const struct { const char *name; void (*func)(); } codeptrs[] =
{
A(Light0) A(WeaponReady) A(Lower) A(Raise) A(Punch) A(ReFire) A(FirePistol)
A(Light1) A(FireShotgun) A(Light2) A(FireShotgun2) A(CheckReload)
A(OpenShotgun2) A(LoadShotgun2) A(CloseShotgun2) A(FireCGun) A(GunFlash)
A(FireMissile) A(Saw) A(FirePlasma) A(BFGsound) A(FireBFG) A(BFGSpray)
A(Explode) A(Pain) A(PlayerScream) A(Fall) A(XScream) A(Look) A(Chase)
A(FaceTarget) A(PosAttack) A(Scream) A(SPosAttack) A(VileChase) A(VileStart)
A(VileTarget) A(VileAttack) A(StartFire) A(Fire) A(FireCrackle) A(Tracer)
A(SkelWhoosh) A(SkelFist) A(SkelMissile) A(FatRaise) A(FatAttack1)
A(FatAttack2) A(FatAttack3) A(BossDeath) A(CPosAttack) A(CPosRefire)
A(TroopAttack) A(SargAttack) A(HeadAttack) A(BruisAttack) A(SkullAttack)
A(Metal) A(SpidRefire) A(BabyMetal) A(BspiAttack) A(Hoof) A(CyberAttack)
A(PainAttack) A(PainDie) A(KeenDie) A(BrainPain) A(BrainScream) A(BrainDie)
A(BrainAwake) A(BrainSpit) A(SpawnSound) A(SpawnFly) A(BrainExplode)
};
#undef A
#define NUMCODEPTRS (sizeof codeptrs / sizeof codeptrs[0])

/* ---- BEX thing flag mnemonics ---- */

static const struct { const char *name; int bits; } bexflags[] =
{
    { "SPECIAL", MF_SPECIAL },          { "SOLID", MF_SOLID },
    { "SHOOTABLE", MF_SHOOTABLE },      { "NOSECTOR", MF_NOSECTOR },
    { "NOBLOCKMAP", MF_NOBLOCKMAP },    { "AMBUSH", MF_AMBUSH },
    { "JUSTHIT", MF_JUSTHIT },          { "JUSTATTACKED", MF_JUSTATTACKED },
    { "SPAWNCEILING", MF_SPAWNCEILING },{ "NOGRAVITY", MF_NOGRAVITY },
    { "DROPOFF", MF_DROPOFF },          { "PICKUP", MF_PICKUP },
    { "NOCLIP", MF_NOCLIP },            { "SLIDE", MF_SLIDE },
    { "FLOAT", MF_FLOAT },              { "TELEPORT", MF_TELEPORT },
    { "MISSILE", MF_MISSILE },          { "DROPPED", MF_DROPPED },
    { "SHADOW", MF_SHADOW },            { "NOBLOOD", MF_NOBLOOD },
    { "CORPSE", MF_CORPSE },            { "INFLOAT", MF_INFLOAT },
    { "COUNTKILL", MF_COUNTKILL },      { "COUNTITEM", MF_COUNTITEM },
    { "SKULLFLY", MF_SKULLFLY },        { "NOTDMATCH", MF_NOTDMATCH },
    { "TRANSLATION", 0x04000000 },      { "TRANSLATION1", 0x04000000 },
    { "TRANSLATION2", 0x08000000 },     { "UNUSED1", 0x08000000 },
    { "UNUSED2", 0x10000000 },          { "UNUSED3", 0x20000000 },
    { "UNUSED4", 0x40000000 },
    { "TRANSLUCENT", 0 },               /* Boom only: drawn solid here */
};
#define NUMBEXFLAGS (sizeof bexflags / sizeof bexflags[0])

/* ------------------------------------------------------------------------ */
/* Parsing                                                                  */

static const char *sName;       /* file or lump being read, for warnings */
static int         sWarnings;

static void Warn(const char *fmt, ...)
{
    (void) fmt;             /* no console on the Mac; just count them */
    sWarnings++;
}

typedef struct
{
    const char *p, *end;
} reader_t;

/* Next line without its line break (and with trailing blanks cut); false
   at the end. */
static Boolean ReadLine(reader_t *r, char *buf, int size)
{
    int n = 0;

    if (r->p >= r->end)
        return false;
    while (r->p < r->end && *r->p != '\n' && *r->p != '\r')
    {
        if (n < size - 1)
            buf[n++] = *r->p;
        r->p++;
    }
    if (r->p < r->end && *r->p == '\r')
        r->p++;
    if (r->p < r->end && *r->p == '\n')
        r->p++;
    while (n > 0 && isspace((unsigned char) buf[n - 1]))
        n--;
    buf[n] = 0;
    return true;
}

/* n characters for a Text block; carriage returns don't count. */
static void ReadChars(reader_t *r, char *out, int n)
{
    while (n > 0 && r->p < r->end)
    {
        if (*r->p != '\r')
        {
            *out++ = *r->p;
            n--;
        }
        r->p++;
    }
    *out = 0;
}

static char *Trim(char *s)
{
    char *e;

    while (isspace((unsigned char) *s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char) e[-1]))
        *--e = 0;
    return s;
}

/* "Key = value" -> key and value, both trimmed. */
static Boolean Assignment(char *line, char **key, char **value)
{
    char *eq = strchr(line, '=');

    if (!eq)
        return false;
    *eq = 0;
    *key = Trim(line);
    *value = Trim(eq + 1);
    return true;
}

static int IntValue(const char *s)
{
    return (int) strtol(s, NULL, 0);
}

/* Thing "Bits": a number, or BEX mnemonics joined by + | , or spaces. */
static int ParseBits(char *value)
{
    char *tok;
    int   bits = 0;

    if (isdigit((unsigned char) value[0]) || value[0] == '-')
        return IntValue(value);
    for (tok = strtok(value, "+|, \t"); tok; tok = strtok(NULL, "+|, \t"))
    {
        int i;

        if (isdigit((unsigned char) tok[0]))
        {
            bits |= IntValue(tok);
            continue;
        }
        for (i = 0; i < NUMBEXFLAGS; i++)
            if (!strcasecmp(tok, bexflags[i].name))
                break;
        if (i < NUMBEXFLAGS)
            bits |= bexflags[i].bits;
        else
            Warn("unknown thing flag %s", tok);
    }
    return bits;
}

static int *ThingField(mobjinfo_t *mi, const char *key)
{
    static const struct { const char *name; int offset; } fields[] =
    {
#define F(n, f) { n, offsetof(mobjinfo_t, f) },
        F("ID #", doomednum)            F("Initial frame", spawnstate)
        F("Hit points", spawnhealth)    F("First moving frame", seestate)
        F("Alert sound", seesound)      F("Reaction time", reactiontime)
        F("Attack sound", attacksound)  F("Injury frame", painstate)
        F("Pain chance", painchance)    F("Pain sound", painsound)
        F("Close attack frame", meleestate) F("Far attack frame", missilestate)
        F("Death frame", deathstate)    F("Exploding frame", xdeathstate)
        F("Death sound", deathsound)    F("Speed", speed)
        F("Width", radius)              F("Height", height)
        F("Mass", mass)                 F("Missile damage", damage)
        F("Action sound", activesound)  F("Bits", flags)
        F("Respawn frame", raisestate)
#undef F
    };
    int i;

    for (i = 0; i < sizeof fields / sizeof fields[0]; i++)
        if (!strcasecmp(key, fields[i].name))
            return (int *)((char *) mi + fields[i].offset);
    return NULL;
}

static void ThingLine(int thing, char *key, char *value)
{
    mobjinfo_t *mi = &mobjinfo[thing];
    int        *field = ThingField(mi, key);

    if (!field)
    {
        Warn("unknown Thing field %s", key);
        return;
    }
    if (field == &mi->flags)
        *field = ParseBits(value);
    else
        *field = IntValue(value);
}

static void FrameLine(int frame, char *key, char *value)
{
    state_t *st = &states[frame];
    int      v = IntValue(value);

    if (!strcasecmp(key, "Sprite number"))
    {
        if (v >= 0 && v < NUMSPRITES)
            st->sprite = v;
    }
    else if (!strcasecmp(key, "Sprite subnumber"))
        st->frame = v;
    else if (!strcasecmp(key, "Duration"))
        st->tics = v;
    else if (!strcasecmp(key, "Next frame"))
    {
        if (v >= 0 && v < NUMSTATES)
            st->nextstate = v;
    }
    else if (!strcasecmp(key, "Unknown 1"))
        st->misc1 = v;
    else if (!strcasecmp(key, "Unknown 2"))
        st->misc2 = v;
    else if (!strcasecmp(key, "Codep Frame"))
    {
        if (v >= 0 && v < NUMSTATES)
            st->action = sStates[v].action;
    }
    else
        Warn("unknown Frame field %s", key);
}

static void WeaponLine(int weapon, char *key, char *value)
{
    weaponinfo_t *wi = &weaponinfo[weapon];
    int           v = IntValue(value);

    if (!strcasecmp(key, "Ammo type"))              wi->ammo = v;
    else if (!strcasecmp(key, "Deselect frame"))    wi->downstate = v;
    else if (!strcasecmp(key, "Select frame"))      wi->upstate = v;
    else if (!strcasecmp(key, "Bobbing frame"))     wi->readystate = v;
    else if (!strcasecmp(key, "Shooting frame"))    wi->atkstate = v;
    else if (!strcasecmp(key, "Firing frame"))      wi->flashstate = v;
    else Warn("unknown Weapon field %s", key);
}

static void AmmoLine(int ammo, char *key, char *value)
{
    if (!strcasecmp(key, "Max ammo"))
        maxammo[ammo] = IntValue(value);
    else if (!strcasecmp(key, "Per ammo"))
        clipammo[ammo] = IntValue(value);
    else
        Warn("unknown Ammo field %s", key);
}

static void MiscLine(char *key, char *value)
{
    int i, v = IntValue(value);

    if (!strcasecmp(key, "Monsters Infight"))
    {   /* DeHackEd wrote 202 for "no" and 221 for "yes" */
        deh_species_infighting = (v == 221);
        return;
    }
    for (i = 0; i < NUMMISC; i++)
        if (!strcasecmp(key, misc_settings[i].name))
        {
            *misc_settings[i].value = v;
            return;
        }
    Warn("unknown Misc field %s", key);
}

/* A Text block: sprite, music and sound names are table entries; anything
   else is a string shown through DEH_String. */
static void TextReplace(const char *from, const char *to)
{
    int i, fl = strlen(from), tl = strlen(to);

    if (fl == 4 && tl == 4)
        for (i = 0; i < NUMSPRITES; i++)
            if (!strncasecmp(sSprnames[i], from, 4))
            {
                sprnames[i] = PoolStrdup(to);
                return;
            }
    if (fl <= 6 && tl <= 6)
    {
        for (i = 1; i < NUMMUSIC; i++)
            if (!strcasecmp(sMusicNames[i], from))
            {
                S_music[i].name = PoolStrdup(to);
                return;
            }
        for (i = 1; i < NUMSFX; i++)
            if (sSfxNames[i] && !strcasecmp(sSfxNames[i], from))
            {
                S_sfx[i].name = PoolStrdup(to);
                return;
            }
    }
    AddReplacement(from, to);
}

/* BEX "\n" escapes and line continuations are joined by the caller. */
static void Unescape(char *s)
{
    char *d = s;

    for (; *s; s++)
    {
        if (*s == '\\' && s[1] == 'n')
        {
            *d++ = '\n';
            s++;
        }
        else if (*s == '\\' && s[1])
            *d++ = *++s;
        else
            *d++ = *s;
    }
    *d = 0;
}

static void BexString(char *key, char *value)
{
    int i;

    Unescape(value);
    for (i = 0; i < NUMBEXSTRINGS; i++)
        if (!strcasecmp(key, bexstrings[i].name))
        {
            AddReplacement(bexstrings[i].text, value);
            return;
        }
    Warn("unknown BEX string %s", key);
}

static void BexPar(char *line)
{
    int a, b, c;
    int n = sscanf(line, "par %d %d %d", &a, &b, &c);

    if (n == 3 && a >= 1 && a <= 4 && b >= 1 && b <= 9)
        pars[a][b] = c;
    else if (n == 2 && a >= 1 && a <= 32)
        cpars[a - 1] = b;
    else if (line[0] && line[0] != '#')
        Warn("bad par line %s", line);
}

static void BexCodePtr(char *key, char *value)
{
    int frame, i;
    const char *name = value;

    if (sscanf(key, "%*s %d", &frame) != 1 && sscanf(key, "%d", &frame) != 1)
        return;
    if (frame < 0 || frame >= NUMSTATES)
        return;
    if (!strncasecmp(name, "A_", 2))
        name += 2;
    if (!strcasecmp(name, "NULL"))
    {
        states[frame].action = NULL;
        return;
    }
    for (i = 0; i < NUMCODEPTRS; i++)
        if (!strcasecmp(name, codeptrs[i].name))
        {
            states[frame].action = codeptrs[i].func;
            return;
        }
    Warn("unknown code pointer %s", value);     /* MBF's, most likely */
}

static void BexRename(char **table, char **orig, int first, int count,
                      char *key, char *value)
{
    int i;

    for (i = first; i < count; i++)
        if (orig[i] && !strcasecmp(orig[i], key))
        {
            table[i] = PoolStrdup(value);
            return;
        }
}

enum { sec_none, sec_thing, sec_frame, sec_pointer, sec_weapon, sec_ammo,
       sec_misc, sec_ignore, sec_strings, sec_pars, sec_codeptr,
       sec_sprites, sec_sounds, sec_music };

static void ParsePatch(const char *text, long length, const char *name)
{
    reader_t r;
    char     line[512];
    int      section = sec_none, index = 0;
    char    *key, *value;

    r.p = text;
    r.end = text + length;
    sName = name;

    while (ReadLine(&r, line, sizeof line))
    {
        char *l = Trim(line);
        int   a, b;

        if (!*l || *l == '#')
            continue;

        /* BEX sections */
        if (*l == '[')
        {
            if (!strncasecmp(l, "[STRINGS]", 9))      section = sec_strings;
            else if (!strncasecmp(l, "[PARS]", 6))    section = sec_pars;
            else if (!strncasecmp(l, "[CODEPTR]", 9)) section = sec_codeptr;
            else if (!strncasecmp(l, "[SPRITES]", 9)) section = sec_sprites;
            else if (!strncasecmp(l, "[SOUNDS]", 8))  section = sec_sounds;
            else if (!strncasecmp(l, "[MUSIC]", 7))   section = sec_music;
            else                                      section = sec_ignore;
            continue;
        }

        /* DeHackEd block headers */
        if (sscanf(l, "Thing %d", &a) == 1 && !strchr(l, '='))
        {
            section = (a >= 1 && a <= NUMMOBJTYPES) ? sec_thing : sec_ignore;
            index = a - 1;
            continue;
        }
        if (sscanf(l, "Frame %d", &a) == 1 && !strchr(l, '='))
        {
            section = (a >= 0 && a < NUMSTATES) ? sec_frame : sec_ignore;
            index = a;
            continue;
        }
        if (!strncasecmp(l, "Pointer", 7) && !strchr(l, '='))
        {
            /* "Pointer 12 (Frame 34)": the frame in brackets is changed */
            char *f = strstr(l, "(Frame");
            if (!f)
                f = strstr(l, "(frame");
            section = sec_ignore;
            if (f && sscanf(f + 6, "%d", &b) == 1 && b >= 0 && b < NUMSTATES)
            {
                section = sec_pointer;
                index = b;
            }
            continue;
        }
        if (sscanf(l, "Weapon %d", &a) == 1 && !strchr(l, '='))
        {
            section = (a >= 0 && a < NUMWEAPONS) ? sec_weapon : sec_ignore;
            index = a;
            continue;
        }
        if (sscanf(l, "Ammo %d", &a) == 1 && !strchr(l, '='))
        {
            section = (a >= 0 && a < NUMAMMO) ? sec_ammo : sec_ignore;
            index = a;
            continue;
        }
        if (!strncasecmp(l, "Misc", 4) && !strchr(l, '='))
        {
            section = sec_misc;
            continue;
        }
        if ((!strncasecmp(l, "Sound", 5) || !strncasecmp(l, "Cheat", 5) ||
             !strncasecmp(l, "Sprite", 6)) && !strchr(l, '='))
        {
            section = sec_ignore;
            continue;
        }
        if (sscanf(l, "Text %d %d", &a, &b) == 2)
        {
            char *from = NewPtr(a + 1), *to = NewPtr(b + 1);

            if (from && to)
            {
                ReadChars(&r, from, a);
                ReadChars(&r, to, b);
                TextReplace(from, to);
            }
            if (from) DisposePtr(from);
            if (to) DisposePtr(to);
            section = sec_none;
            continue;
        }
        if (section == sec_pars)
        {
            BexPar(l);
            continue;
        }

        if (!Assignment(l, &key, &value))
            continue;       /* "Doom version = 21" etc. land below */

        switch (section)
        {
            case sec_thing:   ThingLine(index, key, value); break;
            case sec_frame:   FrameLine(index, key, value); break;
            case sec_pointer:
                if (!strcasecmp(key, "Codep Frame"))
                    FrameLine(index, key, value);
                break;
            case sec_weapon:  WeaponLine(index, key, value); break;
            case sec_ammo:    AmmoLine(index, key, value); break;
            case sec_misc:    MiscLine(key, value); break;
            case sec_strings:
            {   /* a value ending in "\" continues on the next line */
                char  full[2048];
                char  more[512];

                strncpy(full, value, sizeof full - 1);
                full[sizeof full - 1] = 0;
                while (strlen(full) && full[strlen(full) - 1] == '\\' &&
                       ReadLine(&r, more, sizeof more))
                {
                    full[strlen(full) - 1] = 0;
                    strncat(full, Trim(more), sizeof full - strlen(full) - 1);
                }
                BexString(key, full);
                break;
            }
            case sec_codeptr: BexCodePtr(key, value); break;
            case sec_sprites:
                BexRename(sprnames, sSprnames, 0, NUMSPRITES, key, value);
                break;
            case sec_sounds:
            {
                int i;
                for (i = 1; i < NUMSFX; i++)
                    if (sSfxNames[i] && !strcasecmp(sSfxNames[i], key))
                        S_sfx[i].name = PoolStrdup(value);
                break;
            }
            case sec_music:
            {
                int i;
                for (i = 1; i < NUMMUSIC; i++)
                    if (!strcasecmp(sMusicNames[i], key))
                        S_music[i].name = PoolStrdup(value);
                break;
            }
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------------------ */

static Boolean IsPatchFile(const unsigned char *pname)
{
    int n = pname[0];

    return n > 4 && pname[n - 3] == '.' &&
        ((toupper(pname[n - 2]) == 'D' && toupper(pname[n - 1]) == 'E' &&
          toupper(pname[n]) == 'H') ||
         (toupper(pname[n - 2]) == 'B' && toupper(pname[n - 1]) == 'E' &&
          toupper(pname[n]) == 'X'));
}

/* True for .DEH / .BEX names: W_AddFile leaves these to DEH_Init. */
Boolean DEH_IsPatchFile(const unsigned char *pname)
{
    return IsPatchFile(pname);
}

static void ApplyFile(const FSSpec *spec)
{
    short  ref;
    long   eof, count;
    char  *buf;

    if (FSpOpenDF(spec, fsRdPerm, &ref) != noErr)
        return;
    if (GetEOF(ref, &eof) == noErr && eof > 0 && (buf = NewPtr(eof)) != NULL)
    {
        count = eof;
        if (FSRead(ref, &count, buf) == noErr)
            ParsePatch(buf, count, "file");
        DisposePtr(buf);
    }
    FSClose(ref);
}

int DEH_NumPatches;

/* Called after W_InitMultipleFiles, before the menus, renderer and play
   code read the tables. */
void DEH_Init(void)
{
    int i;

    if (!sSaved)
        SaveTables();
    else
        RestoreTables();
    PoolFree();
    sNumReplacements = 0;
    sWarnings = 0;
    DEH_NumPatches = 0;

    /* DEHACKED lumps, in load order (Boom) */
    for (i = 0; i < numlumps; i++)
        if (!strncasecmp(lumpinfo[i].name, "DEHACKED", 8) && lumpinfo[i].size > 0)
        {
            char *buf = NewPtr(lumpinfo[i].size);

            if (buf)
            {
                W_ReadLump(i, buf);
                ParsePatch(buf, lumpinfo[i].size, "DEHACKED");
                DisposePtr(buf);
                DEH_NumPatches++;
            }
        }

    /* .DEH / .BEX files added like WADs */
    for (i = 0; i < gNumWads; i++)
        if (IsPatchFile(gWadFiles[i].name))
        {
            ApplyFile(&gWadFiles[i]);
            DEH_NumPatches++;
        }
}
