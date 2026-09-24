# RESUME: Mac DOOM (Lion Ultimate DOOM 1.0.2) Retro68 port for 68040 Quadras

Paste this whole file as the first message of a new session to continue.

## Who / how the user works

- The user owns the repo `~/repos/DOOM-for-Macintosh` (fork of izne/DOOM-for-Macintosh; git remote
  `git@github.com:izne/DOOM-for-Macintosh.git`). Work is on branch **`retro68-port`**, not pushed.
- **Show QEMU in a visible GTK window** while driving it (the user watches). A memory note exists.
- Use **`rb-cli`** (`~/.local/bin/rb-cli`) for disk images, never `rusty-backup` (that's the GUI; it hangs).
- Commits end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Target hardware: **Quadra 800 (68040/33) and 68LC040 machines, 32 MB RAM minimum.** Later the user
  will test on a physical Q800 and on their own MiSTer Quadra 800 FPGA core (`~/MacQuadra800_eth`).

## Decisions already made by the user

- **Full Boom/MBF is a separate project** (branch `prboom-68k`, notes in `RESUME-prboom-68k.md` on
  that branch). This branch stays on Lion's engine: vanilla + limit-removing + DeHackEd/BEX.
  WADs whose levels use Boom/MBF specials get an alert saying some things won't work.
- Shareware: no PWAD restriction in code (user: "skip the shareware parts... just ship the
  shareware WAD"). Data-driven checks stay (episodes that don't exist, plasma/BFG graphics).
- WAD music: converted on first use (no prompt) and cached as `MIDI/<WAD>/<TRACK>.MID`.
- FPS toggle stays runtime-only (never saved in prefs).
- **Always stop the emulated Mac gracefully** (`tools/qemu-shutdown.py` / testenv
  `shutdown.py`), never `qm.py quit` while the Mac is running; disk caches otherwise don't reach
  the image.
- Release pipeline: both `tools/release.sh` and GitHub Actions (`.github/workflows/retro68.yml`).
  Nothing pushed yet; pushing the branch / tags needs the user's OK.

## >>> WHERE WE STOPPED (2026-09-23, night) <<<

The user is testing `~/Desktop/DOOM-Quadra-full.hda` (make-full-disk.sh, "-musiclog") on the
real Quadra 800. Fixed this session (all tested in QEMU):
- WAD music: QuickTime 2.x can't open a plain .MID (-2048); `MacWads_MidiToMovie` converts it once
  to a .MID! movie (ConvertFileToMovieFile). User heard music on the Quadra. Music log uses the
  File Manager now (fopen "a" lost lines).
- Control menu F-key items (Detail, Gamma ⌘G, Sound...) close DOOM's menu first.
- Crash: R_MakeSpans used leftover `bottom[]` of empty columns (0xFFFF → row 65535, bus error in
  R_DrawSpan040). Matches the user's MacsBug crash on the Quadra (confirm with them).
- WAD reload: P_ForgetZoneArrays (stale grow arrays, "Z_Free ... without ZONEID"), colormap
  Mac-palette remap redone (black showed white), missing PNAMES patches tolerated.
- Don't malloc during rendering: Retro68 malloc = NewPtr, which moves handles (stash@{0} = the
  failed attempt).

Waiting on the user: Quadra results (benchmark ⌘B numbers at large/small graphics, low detail;
real-hardware FPS was ~12.4 with the counter), whether the demo1 crash is gone.
Open: counter quirks (stale history on toggle, 63/64 frames); red progress bar remnant below the
status bar after a reload; `-mouse` doesn't check "Mouse" in the Options menu; MacsBug test system
`~/doom-mac-testenv/qemu/sys755-macsbug.hda` (SYSBASE=...). Then UMAPINFO flow testing.

### Personal full disk (never commit or distribute)

- GOG ISO was copied by the user to `~/Desktop/DaniGOGGames.iso`. Extracted to
  `~/doom-mac-testenv/gog/`: `dos/` (classic DOS IWADs: DOOM.WAD Ultimate 1.9 md5 c4fe9fd9...,
  DOOM2 1.9 25e1459c..., TNT 4e158d99..., PLUTONIA 75c8cf89...), `kex/` (2024 re-release:
  nerve, masterlevels, sigil, sigil2, id1, iddm1, and KEX IWADs), `master/` (20 original
  Master Levels WADs + TXT).
- `~/doom-mac-testenv/gog/make-full-disk.sh [out.hda] ["DOOM Args"]` builds a 200 MB disk:
  DOS IWADs + NERVE/MASTERLEVELS/SIGIL/SIGIL2 at the root, "Master Levels" folder.
  E.g. `make-full-disk.sh ~/doom-mac-testenv/qemu/doom-master.hda "-file SIGIL.WAD -warp 5 1 -musiclog"`.
- Supported: NERVE, masterlevels.wad, SIGIL, SIGIL II (all vanilla maps + UMAPINFO).
  Not supported on Lion's engine: `id1.wad` (Legacy of Rust, ID24), `iddm1.wad` (Boom specials).
- The 20 separate Master Levels WADs have no names (vanilla behaviour); the combined
  MASTERLEVELS.WAD has everything.

## Open work (priority order)

1. Music (above), then UMAPINFO flow testing, then give the user the full disk.
2. **Performance: ~30-35 fps on a Quadra 800.** QEMU icount 28.0 fps (was 27.7). Done: 68040
   span drawer `R_DrawSpan040` (src/Fast68K.s). Every speed tweak goes behind a runtime switch
   (user's request): Benchmark > "Lion's Original Drawers" / `-classic`; log records it.
   Next ideas: addx column drawer (icount can't show 68040 cycle gains; MAME's Quadra could,
   but it stops on a "known problems" notice that needs one keypress in its window, and the
   user doesn't want their desktop driven with xdotool), scanline blit mode, MOVE16 blit.
   Profile data from a single-segment build: `~/doom-mac-testenv/prof/` (hotblocks.txt; map
   PCs with build-prof/UltimateDOOM.code.bin.gdb and "MacBench_ReadArgs at" in the log).
3. The user tests 1.1.0-beta1 on the real Quadra (sent: build-release/release/*).
4. DOOM II splash/About artwork; push branch + tag a release when the user says so.

## Done this session (2026-09-23, after the first resume)

- SIZE 24 MB / 12 MB; zone up to 24 MB.
- `gamemission` (doom/doom2/tnt/plut) from the base WAD name: TNT/Plutonia level names and
  finale texts (from linuxdoom-1.10), Final DOOM teleporter z quirk.
- MAXWADFILES 32 (one define in DOOMDEF.H); WADs opened read-only; W_CloseWadFiles fixed.
- Limit removal: visplanes (hashed), openings, drawsegs, vissprites, intercepts, spechit,
  plats, ceilings, buttons, scrolling lines, anims, switches, brain targets; savegame buffer
  sized from the level. `P_GrowArray` helper in P_SPEC.C.
- Map loading: unsigned indices, int blockmap (>64 KB), REJECT padding, missing
  textures/flats/unknown things tolerated, texture-name hash.
- Boom: ANIMATED/SWITCHES lumps, F_/S_ marker coalescing (FF_START/SS_START), PWAD sprite
  replacement; DeePsea tall patches; `R_DrawColumnHeight` for walls not 128 tall.
- DeHackEd/BEX: `retro68/src/deh.c` (DEH_Init after W_InitMultipleFiles; DEH_String used for
  messages, level names, finale texts, cast names; misc values hooked). Tested in QEMU with
  `~/doom-mac-testenv/wadtest/deh/TEST.DEH` (150% health, 99 bullets, "E1M1: DEH WORKS").
- Music: `retro68/src/mus2mid.c`; lookup order in `MacWads_FindMusic` (mac_wads.c).
- Mouse: captured only while playing a level; DOOM's menu/pause/title/demos release it
  (I_StartTic `gMouseCaptured`). `-mouse` in DOOM Args.
- UMAPINFO (`retro68/src/umapinfo.c`) + E5/E6, `S_ChangeMusicByName`, dynamic episode menu,
  WI level-name text, FMI finale; `make-disk.sh --no-music`; `qemu-shutdown.py`.
- Build: `-w` was silently disabling `-Werror=implicit-function-declaration`; replaced by
  targeted `-Wno-*` flags; `-std=gnu17` (the CI image's GCC 16 defaults to C23).

## Repo layout / what exists

Source tree: `UltDoom/UDOOM 1.0.2/` (`src/`, `hdrs/`, Mac Roman encoding, converted CR→LF in commit
15f4c51). The port lives in `UltDoom/UDOOM 1.0.2/retro68/`:

| Path | Purpose |
|---|---|
| `CMakeLists.txt` | Build. Options `DOOM_CPU` (68040), `DOOM_OPT` (-O2), `DOOM_SINGLE_SEGMENT` (OFF). Generates `build/ci-include/` case-insensitive header symlinks. Force-includes `compat/retro68_compat.h`; C files get `-xc`; `-Werror=implicit-function-declaration`. |
| `compat/retro68_compat.h` | Universal Interfaces names/constants on top of Multiversal; old routine names; QuickTime traps; MixedMode no-ops; ParamText LF→CR wrapper; AppleTalk/Serial/CTB bits. |
| `compat/mac_gcc68k.h` | Inline 68020+ FixedMul/FixedDiv (muls.l/divs.l), SHORT/LONG via bswap. |
| `compat/MacTCP.h`, `compat/AppleTalk.h` | MacTCP UDP subset; Apple UH 2.0a3 AppleTalk.h. |
| `src/Draw68K.s`, `src/Blit68K.s` | Lion's asm column/span drawers and blitters, translated by `tools/mpw2gas.py`. |
| `src/mac_glue.c` | p2cstr/c2pstr, HGetVol, KillIO, ParamText wrapper. |
| `src/mac_bench.c` | FPS toggle (runtime only, never saved), Benchmark submenu, `DOOM Args` parsing, results log. |
| `src/mac_menus.c` | Multiplayer menu; mid-session net start (MacMenus_Poll at top of D_DoomLoop). |
| `src/mac_wads.c` | WADs menu, reload in place (Z_Reset + re-init), -file, per-WAD MIDI lookup, CRC-32 WAD signature. |
| `src/tcpnet.c` | TCP/IP (UDP over MacTCP) transport: star topology via host, lobby JOIN/WELCOME/START/ACK/REJECT, port 5029. |
| `src/appletalk_glue.c` | MPW Interface.o AppleTalk glue ported from Apple's System 7.1 sources (`~/repos/supermario/.../Libs/InterfaceSrcs/nAppleTalk.a`, `piNBP.a`, `piMAIN.a`). |
| `src/net_stubs.c` | CTB/IPX stubs. |
| `rsrc/DoomShell.rsrc.bin` | Resources from the shipped shareware app (`DOOM SW 1.0.2/DOOM.sea`) minus CODE/DATA/cfrg/SIZE/vers (`tools/rsrcfilter.py`). |
| `rsrc/UltimateDOOM.r` | SIZE (16 MB / 6 MB) and vers 1.0.2r68. |
| `tools/make-disk.sh` | rb-cli: HFS → APM + SCSI driver disk; `-w WAD`, `-m MIDIFOLDER`, `--no-shareware`, `--no-music`, `-a app.bin`, `DOOM_ARGS=...`, `BUILD_DIR=`. Works with old (`new --fs`) and new (`new volume hfs`) rb-cli. |
| `tools/make-dist.sh` | `build/dist/`: DOOM-68040-shareware.hda, DOOM-68040-noWAD.hda, UltimateDOOM-68040.sit.hqx. |
| `tools/qemu-q800.sh` | Reproducible QEMU launcher (ROM/SYSDISK/PRAM env vars). |
| `tools/qemu-shutdown.py` | Graceful shutdown: quits DOOM, Finder Special > Shut Down (closed-loop on screenshots). |
| `tools/release.sh` | Versioned release build into `build-release/release/`; used by CI. |
| `src/deh.c`, `src/mus2mid.c` | DeHackEd/BEX; MUS to MIDI. |
| `README.md` | Full user docs (build, packaging, benchmark, multiplayer, WADs and music, performance notes). |

Build:
```sh
cd "UltDoom/UDOOM 1.0.2/retro68"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$HOME/repos/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build build && tools/make-dist.sh
```

Commits on `retro68-port` (newest first): f854f35 alert line breaks · cab461b WADs menu/reload/MIDI/WAD check ·
27fabc6 Multiplayer menu + Benchmark submenu · e46ecb7 AppleTalk · 3ea523f serial · 31dc8e1 TCP/IP ·
8e4291d packaging/docs · 54c0aab FPS + benchmark · 9f7f4c4 Retro68 build · 15f4c51 CR→LF.

## Features done (all tested in QEMU unless noted)

- Builds for 68040 with `-msoft-float`: **zero FPU instructions** (verified by disassembly), so LC040 is OK.
- FPS counter: Control → Show Frame Rate ⌘F / Q key / `-fps`. Runtime only; **don't persist in prefs**
  (the user's explicit decision).
- Benchmark: Control → Benchmark ▸ All Demos ⌘B / demo1 / demo2 / demo3. Alert + "DOOM Benchmark Log".
  Gametic counts 5026/3836/2134 match PC 1.9 timedemo (demo sync is exact).
- DOOM Args file next to the app: `-bench -timedemo demoN -fps -quit -host N -join a.b.c.d[:port]
  -serial modem|printer -appletalk N -deathmatch -altdeath -nomonsters -respawn -skill N -warp E M
  -file A.WAD B.WAD -musiclog`.
- Multiplayer: TCP/IP (tested 2 players through QEMU NAT + hostfwd, via Args and via menus), serial
  (tested 2 QEMUs, modem ports joined by a TCP socket), AppleTalk (single-machine LocalTalk smoke test
  only; **two-machine untested**, QEMU has no LocalTalk and its SONIC refuses EtherTalk even after Apple's
  Network Software Installer). Multiplayer menu starts games mid-session. Open Transport untested (MacTCP
  2.0.6 used; OT's MacTCP compatibility should work).
- WADs: WADs menu Add/Remove with in-place reload; drag-drop fix; -file; TNT/PLUTONIA as DOOM II mode
  (text still DOOM II's; see task 3); per-WAD `MIDI/<WAD>/` music (`.MID!` QuickTime or `.MID` typed
  'Midi' automatically); TCP lobby CRC-32 check of WAD contents (CRCs match zlib's).
- No-WAD start prompts "The WAD file could not be found. Would you like to look for it?" + Open dialog.

## Measurements (QEMU `-icount shift=5,align=off` ≈ 31 MIPS, large graphics, screenblocks 9)

Baseline -O2 multi-seg: demo1 28.2, demo2 28.2, demo3 26.9, **avg 27.9 fps**; -O3 27.8; single-segment 27.5.
icount counts instructions, not 68040 cycles; it can't see bfextu cost, cache, or VRAM speed.
Instruction profile (hotblocks, % of game code): R_DrawColumn68KLowRes 22.5, **Blit640Width 20.6**,
R_DrawSpanAsm 13.3, R_RenderSegLoop 9.5, R_DrawPlanes 3.1, R_MakeSpans 3.0, V_DrawPatch 2.9.
The game's own code was only ~32% of all instructions in that run (it included boot/Finder; ROM + system
code dominate; worth re-profiling with the timedemo alone).

### Performance ideas for task 1 (Quadra 800 target)
- The pixel-doubling blit (640×400 from 320×200, each pixel written 4×, 256 KB VRAM/frame) is likely the
  biggest real-hardware cost. Options: MOVE16 burst writes on 040 (check onboard DAFB video accepts
  it), a 2×-horizontal / line-skip "medium" mode (Lion has Blit320WidthMed / Blit640Skip), writing rows
  once and duplicating with move16.
- Column/span inner loops use `bfextu` (slow on 040; check the MC68040 UM timing tables). Consider
  swapped-word addx fixed-point stepping, and keep loops within the 4 KB I-cache.
- Consider `-mtune`/unroll per function, `__attribute__((hot))`, aligning hot loops to 16 bytes.
- WaitNextEvent is already throttled to every 4th tic (I_IBM.C I_StartTic).
- Sound Manager mixing may cost real CPU on a Q800 (fewer channels option).
- Validate on real hardware; QEMU can't measure cycles. The user's MiSTer core and physical Q800 later.

## QEMU test environment (persistent copy: `~/doom-mac-testenv/`)

- `qemu/` (main, visible window, VNC :17) and `qemu2/` (second Mac, VNC :19): `run.sh` copies a fresh
  system disk each boot (`SYSBASE` env, default `~/MacOS_SampleDisks/MacLC_7-5-5.hda`), copies
  `doom-master.hda` → `doom.hda`, `pram-32bit.img` → `pram.img`, then runs `qemu-system-m68k -M q800 -m 128
  -bios f1acad13.rom -g 640x480x8 -display gtk,zoom-to-fit=on -monitor unix:mon.sock ...` + extra args.
- **32-bit addressing** must be on (the sample disk had it off → only 6 MB free). It's saved in
  `pram-32bit.img` (PRAM byte $8A = 0x65).
- `sys755-net.hda`: 7.5.5 with MacTCP set to Ethernet/BOOTP (QEMU user net gives 10.0.2.15; the host is
  10.0.2.2). `sys755-at.hda`: + NSI 1.5.1 EtherTalk installed (EtherTalk still fails in QEMU).
- `qm.py` = HMP client (`./qm.py "sendkey d" "sleep 1" "shot name"`; screendump PNG).
  `waitfinder.sh` waits for the Trash icon; `launchgame.py` opens the DOOM disk and the app by type-select,
  verifying via menu-bar pixels (x 196–206 has text only in the game's "Control" title);
  `mpmenu.sh host|join` picks Multiplayer items; `opennet.sh` opens the Network control panel;
  `bench.sh LABEL` = icount benchmark (APP= env for another build).
- Net tests: A `-nic user,model=dp83932,hostfwd=udp::5029-:5029`, B `-nic user,model=dp83932`, B joins
  10.0.2.2. Serial: A `-serial tcp::4555,server=on,wait=off`, B `-serial tcp:127.0.0.1:4555`.
- ROM: `~/repos/mame/roms/macqd800.zip` (f1acad13.rom). QEMU 11.1 build with TCG plugins (headless only):
  `/home/dani/nextstep-test/qemu-src/build/qemu-system-m68k` + `build-sparc64/contrib/plugins/libhotblocks.so`
  (see `prof/run.sh`). Map PCs using a single-segment build: the benchmark log prints `MacBench_ReadArgs at
  %p`, and the `-musiclog` log prints `MacWads_SetMusicLog at %p`.
- Crash debugging: run QEMU with `-d int -D int.log`, grep "Access Fault" after a marker, map PC
  (runtime − (runtime_sym − link_sym)) against `nm` of `build*/UltimateDOOM.code.bin.gdb`.
- `wadtest/`: `DOOM.WAD` = shareware data faked as registered (PNAMES cut to 163 + TEXTURE2 with SW1*/SW2*
  switch names) **for local testing only, never distribute**; `TESTMAP.WAD` (TITLEPIC := CREDIT);
  `alt/TESTMAP.WAD` (1 byte changed); `TESTMAP/INTRO.MID!.bin`. `udptest/` = MacTCP UDP proof of concept.

## Gotchas learned (save time)

- User preference: keep answers simple; they got confused by many versions/labels. One
  playable build at a time.
- Always shut the emulated Mac down gracefully (`~/doom-mac-testenv/qemu/shutdown.py`)
  before reading its disk; `bench.sh` now does too. Don't start a 2nd QEMU while one runs.

- Patch Lion's Mac Roman files with Python (latin1); blank lines often hold tabs. A helper with
  whitespace-flexible matching lives in the session scratchpad (`lp.py`); recreate if needed.
- `grep` in this environment is ugrep with `-I` (skips binary files): use Python to search binaries.
- `pkill -f qemu...` from a shell whose command line contains that string kills the shell itself.
- Another project's QEMU (`~/cdv-testenv`, -m 64) may be running; leave it alone.
- Crash mapping: `./qm.py "xp /16i 0xPC"` disassembles guest memory at the faulting PC.
- `W_CheckNumForName` is called before WADs load (splash screen music).

- Source is Mac Roman: use `LC_ALL=C grep -a`, and edit via Python binary read/replace (latin1).
- `"\p..."` literals can't initialise arrays in Retro68 GCC: write `"\011DOOM2.WAD"` (octal length).
- Retro68 uppercases `pascal` function symbol names (GETMMUMODE…). Multiversal declares some traps with
  no glue (GetMMUMode, SetEventMask, KillIO, HGetVol): macros/glue in compat/mac_glue.
- Multiversal NewRoutineDescriptor/DisposeRoutineDescriptor are real `_MixedModeDispatch` traps →
  "unimplemented trap" on 68K; compat now makes them no-ops.
- `AppendMenu` parses `/ ( ; ^ ! <` metacharacters: add placeholders, then `SetMenuItemText`.
- Popup menu (CNTL 550): get its MenuHandle from `(**(PopupPrivateDataHandle)(**ctl).contrlData).mHandle`,
  not GetMHandle (NULL outside tracking).
- MPW `"\n"` was CR; GCC's is LF → ParamText wrapper converts. Other UI text paths with `\n` may need the same.
- I_IBM.C locally `#undef`s `__NO_USE_ASM` (so the asm blitters are used) and used to force
  `__LION_SHOWFRAMERATE 0` (removed).
- Lion's startup order: InitManagers (base WAD search) → DOOM Args (MacBench_ReadArgs, MacWads_ApplyArgs,
  MacBench_ApplyNetArgs) → InitialDialog → GetPlayMode (net lobby) → D_DoomMain (W_InitMultipleFiles…).
  The net lobby runs **before** the WADs load.
- Zone reload needs `S_sfx[i].usefulness = -1` (0 crashes S_UpdateSounds); M_Init now saves/restores
  the MainDef/ReadDef1 fields it adjusts for DOOM II.
- The shareware texture loader tolerates missing patches ≥163 only in shareware mode.
- QEMU ADB mouse: after a big negative move, wait ~2.5 s before further moves; small moves are ~1:1, large
  ones accelerate. Under `-icount`, keystrokes get lost: use the verifying launcher.
- Default movement keys are I/J/K/L (KeyConfig gDoomMoveConfig=1), not arrows.
