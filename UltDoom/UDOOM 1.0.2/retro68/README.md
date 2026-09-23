# Ultimate DOOM 1.0.2 for 68040 Macs (Retro68 build)

This directory builds Lion Entertainment's Ultimate DOOM 1.0.2 sources
(`../src`, `../hdrs`) with the [Retro68](https://github.com/autc04/Retro68)
GCC cross-toolchain.

The binary targets the 68040 but uses no FPU instructions, so it runs on
full 68040 machines (Quadra 700/800/900/950, Centris 650) and on the
FPU-less 68LC040 (Centris 610, Quadra 605, LC/Performa 475, etc.).

One application plays every IWAD: shareware `DOOM1.WAD`, registered or
Ultimate `DOOM.WAD`, and `DOOM2.WAD`. Put the WAD next to the application,
or drop it on the application in the Finder.

## Requirements (on the Mac)

- 68040 or 68LC040, System 7.1 or later (tested on 7.5.5)
- 32-bit addressing turned on (Memory control panel)
- 8 MB of RAM or more; the application asks for 16 MB and runs in 6 MB
- 256 colours at 640×480 or larger
- QuickTime 2.0 or later, for music (optional)

## Building

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$RETRO68/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build build
```

This produces `build/UltimateDOOM.bin` (MacBinary) and `build/UltimateDOOM.dsk`
(an 800K image containing only the application).

Options (`-D...`):

| Option | Default | Meaning |
| --- | --- | --- |
| `DOOM_CPU` | `68040` | `-m` CPU; `68020`/`68030` also build, untested |
| `DOOM_OPT` | `-O2` | optimisation flags |
| `DOOM_SINGLE_SEGMENT` | `OFF` | link as one CODE segment (measured no faster) |

## Packaging a disk image

`tools/make-disk.sh` uses `rb-cli` from [rusty-backup](https://github.com/danifunker/rusty-backup) to build an
Apple Partition Map SCSI disk image with an Apple driver. Any Quadra ROM, including
QEMU's `q800`, mounts it as a second disk. It contains:

```
DOOM/
  Ultimate DOOM        the application
  DOOM1.WAD            shareware WAD from ../../../DOOM SW 1.0.2/DOOM.sea
  Music/               QuickTime MIDI movies from the same archive
  DOOM Read Me
```

```sh
tools/make-disk.sh                          # -> build/DOOM.hda
tools/make-disk.sh -w ~/wads/DOOM2.WAD      # add your own WAD
tools/make-disk.sh --no-shareware -w ~/wads/DOOM.WAD
DOOM_ARGS="-bench -quit" tools/make-disk.sh # unattended benchmark disk
```

Only the shareware WAD is freely redistributable. For the registered games,
supply your own `DOOM.WAD` or `DOOM2.WAD`.

## Release artefacts

`tools/make-dist.sh` writes to `build/dist/`:

| File | Contents |
| --- | --- |
| `DOOM-68040-shareware.hda` | ready to play: application, music, shareware `DOOM1.WAD` |
| `DOOM-68040-noWAD.hda` | same without a WAD; add your own `DOOM.WAD` / `DOOM2.WAD` |
| `UltimateDOOM-68040.sit.hqx` | the application alone, to drop into an existing Mac DOOM folder |

If no WAD sits next to the application, it asks for one with a standard Open dialog.

## Testing in QEMU

```sh
ROM=F1ACAD13.rom SYSDISK=MacOS755.hda tools/qemu-q800.sh build/dist/DOOM-68040-shareware.hda
```

Turn on **32-bit addressing** in the Memory control panel once and restart; the PRAM
file keeps the setting. Add `-icount shift=5,align=off` for deterministic, repeatable
benchmark timing: the guest clock then advances at about 31 M instructions per second,
roughly a 33 MHz 68040. QEMU doesn't model the 68040's caches, per-instruction cycle
costs or VRAM speed, so use it for before/after comparisons, not absolute numbers.

## Multiplayer

Use the **Multiplayer** menu at any time (it's greyed out while a network game is
running):

- **Host TCP/IP Game…** / **Join TCP/IP Game…**
- **Serial Game (Modem Port)…** / **Serial Game (Printer Port)…**
- **AppleTalk Game…**

Each opens the multiplayer setup dialog with that connection chosen. **Start Game**
leaves whatever is running (demo or single-player game) and starts the network game.
The same dialog is also offered at launch. Under **Connect via**:

| Transport | Players | Notes |
| --- | --- | --- |
| **TCP/IP** | 2–4 | UDP over MacTCP, or Open Transport's MacTCP compatibility. Tick **Host this game** on one Mac; it shows its IP address. The others type that address (`a.b.c.d` or `a.b.c.d:port`). |
| **Serial (Modem) / Serial (Printer)** | 2 | Null-modem cable between the two ports. Lion's original code. |
| **AppleTalk** | 2–4 | LocalTalk or EtherTalk; players find each other automatically (NBP). Lion's original code. |

TCP/IP details:

- The host listens on **UDP port 5029**. For internet play, forward that port on the
  host's router to the host Mac; joiners need no router changes.
- Behind a router, the host Mac only knows its LAN address. For internet games, give
  joiners the router's public address instead.
- Joiners send everything through the host, which relays it, so 3–4 players work
  even when every joiner is behind a different NAT.

The same games can be started from **DOOM Args** without the dialogs:

```
-host 2                     host a 2-player TCP/IP game
-join 192.168.1.20          join it (optionally :port)
-serial modem               serial game on the modem port (or: printer)
-appletalk 3                3-player AppleTalk game
-deathmatch | -altdeath   -skill 1-5   -warp E M   -nomonsters   -respawn
```

Tested in QEMU:

- **TCP/IP:** two Quadra 800s, Mac OS 7.5.5 / MacTCP 2.0.6, with the joiner reaching
  the host through QEMU's NAT and a UDP port forward.
- **Serial:** two Quadras with their modem ports joined through a socket.
- **AppleTalk:** one Quadra on LocalTalk. Socket, name registration, lookup and
  shutdown work. A two-machine game is untested, because QEMU has no LocalTalk and
  its emulated Quadra Ethernet wouldn't switch to EtherTalk, even after Apple's
  Network Software Installer.

## Frame rate and benchmark

The **Control** menu has two new items:

- **Show Frame Rate (⌘F)** toggles an FPS counter (a rolling average over 64 frames)
  in the top-left of the view. The **Q** key also toggles it. The setting isn't saved:
  the counter starts off each launch unless `DOOM Args` contains `-fps`.
- **Benchmark ▸ All Demos (⌘B) / demo1 / demo2 / demo3** plays those demos as
  timedemos: one game tic per rendered frame, as fast as the machine can draw, like PC
  `-timedemo`. The result is shown in an alert and appended to **DOOM Benchmark Log**
  next to the application.

For unattended runs, create a text file called **DOOM Args** next to the
application containing any of:

```
-bench              run the three-demo benchmark at startup
-timedemo demoN     time a single demo at startup
-fps                start with the frame-rate counter on
-quit               quit when the benchmark finishes
```

## What changed for Retro68

- `compat/`: maps the Universal Interfaces names the code uses onto Retro68's
  Multiversal headers. The few QuickTime traps Multiversal lacks come from Apple's
  Universal Headers 2.0a3, which ship in this repository under `DOOM II 1.0 7.14.95/CW5`.
- `compat/mac_gcc68k.h`: `FixedMul`/`FixedDiv` as inline 68020+ `muls.l`/`divs.l`,
  replacing MPW inline traps; WAD byte swaps via GCC builtins.
- `src/Draw68K.s`, `src/Blit68K.s`: Lion's hand-written column/span renderers and
  screen blitters, machine-translated from `../libs/*.asm` by `tools/mpw2gas.py`.
- `rsrc/DoomShell.rsrc.bin`: icons, BNDL/FREF, splash screens, dialogs, menus,
  cursors and balloon help from the shipped shareware application, with its code
  removed by `tools/rsrcfilter.py`.
- `src/tcpnet.c`, `compat/MacTCP.h`: new TCP/IP multiplayer transport (see below).
- `src/appletalk_glue.c`, `compat/AppleTalk.h`: the AppleTalk interface glue MPW's
  `Interface.o` supplied, ported to C from Apple's `nAppleTalk.a`, `piNBP.a` and
  `piMAIN.a`, so Lion's `AppleTalkNet.c` builds unchanged.
- Comm Toolbox (modem) and IPX multiplayer are stubbed out (`src/net_stubs.c`) and
  disabled in the dialog.

## Performance notes

Measured with **Run Benchmark** under `qemu-system-m68k -M q800 -icount shift=5`,
Mac OS 7.5.5, large graphics, default screen size (screenblocks 9):

| Build | demo1 | demo2 | demo3 | average |
| --- | --- | --- | --- | --- |
| `-O2`, multi-segment (default) | 28.2 | 28.2 | 26.9 | **27.9 fps** |
| `-O3` | 28.0 | 28.2 | 26.8 | 27.8 fps |
| `-O2`, single segment | 27.7 | 27.8 | 26.5 | 27.5 fps |

The gametic counts (5026 / 3836 / 2134) match PC DOOM 1.9's `-timedemo`, so
demo playback is in sync.

An instruction-count profile (QEMU `hotblocks` plugin) of a demo3 timedemo breaks
the game's own time down as:

| Function | Share of the game's instructions |
| --- | --- |
| `R_DrawColumn68KLowRes` (walls, sprites; asm) | 22.5% |
| `Blit640Width` (pixel-doubling to the 640×400 window; asm) | 20.6% |
| `R_DrawSpanAsm` (floors, ceilings; asm) | 13.3% |
| `R_RenderSegLoop` | 9.5% |
| everything else | < 3.1% each |

The engine already carried Killough's lump-name hash table (`../src/W_WAD.C`).
This build adds:

- 68040 code generation, with `FixedMul`/`FixedDiv` inlined as single
  `muls.l`/`divs.l` instructions instead of MPW trap glue and a library call;
- Lion's hand-written 68K column, span and blit routines, which the
  CodeWarrior project also used;
- no soft-float in the frame path (the FPS counter was `long double`).

On real hardware the pixel-doubling blit costs more than the profile suggests,
because it writes 256 KB of VRAM per frame. **Options → Small Graphics** avoids it,
and **Control → Graphic Detail** (low detail) halves the column and span work.
