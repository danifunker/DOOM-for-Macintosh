# Retro68 build of Ultimate DOOM 1.0.2 (Lion Entertainment Macintosh port).
#
#   cmake -S . -B build \
#     -DCMAKE_TOOLCHAIN_FILE=$RETRO68/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
#   cmake --build build
#
# Targets the 68040 but emits no FPU instructions, so the same binary runs on
# full 68040 Quadras and on FPU-less 68LC040 machines.

cmake_minimum_required(VERSION 3.13)
project(UltimateDOOM C ASM)

set(DOOM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(DOOM_CPU "68040" CACHE STRING "Target CPU passed to -m (68020, 68030, 68040)")
set(DOOM_OPT "-O2" CACHE STRING "Optimisation flags")
option(DOOM_SINGLE_SEGMENT "Link as one CODE segment (no jump-table calls)" OFF)
set(DOOM_VERSION "1.0.2r68" CACHE STRING "Version for the 'vers' resource (tools/release.sh sets it)")

# 'vers' numbers: "1.3.x" -> major 0x01, minor 0x30 (BCD, as the Finder shows it)
string(REGEX MATCH "^([0-9]+)\\.?([0-9]?)\\.?([0-9]?)" _v "${DOOM_VERSION}")
set(_maj "${CMAKE_MATCH_1}")
set(_min "${CMAKE_MATCH_2}")
set(_bug "${CMAKE_MATCH_3}")
if(NOT _maj)
    set(_maj 1)
endif()
if(NOT _min)
    set(_min 0)
endif()
if(NOT _bug)
    set(_bug 0)
endif()
set(DOOM_VERS_MAJOR "0x${_maj}")
set(DOOM_VERS_MINOR "0x${_min}${_bug}")
configure_file(rsrc/UltimateDOOM.r.in ${CMAKE_CURRENT_BINARY_DIR}/UltimateDOOM.r @ONLY)

# ---------------------------------------------------------------------------
# The sources include headers with inconsistent case ("doomdef.h" vs
# DOOMDEF.H).  Build a directory of symlinks under every spelling used.
# ---------------------------------------------------------------------------
set(CI_DIR ${CMAKE_CURRENT_BINARY_DIR}/ci-include)
file(MAKE_DIRECTORY ${CI_DIR})
file(GLOB _hdrs
    ${DOOM_ROOT}/hdrs/*
    ${DOOM_ROOT}/sys/LionIncludes/*
    ${DOOM_ROOT}/MacIPX_includes/*)
foreach(h ${_hdrs})
    get_filename_component(n ${h} NAME)
    string(TOLOWER ${n} lo)
    string(TOUPPER ${n} up)
    # also "P_DOORS.PROTO.h": upper-case name with a lower-case extension
    string(REGEX REPLACE "\\.H$" ".h" up_h ${up})
    foreach(alias ${n} ${lo} ${up} ${up_h})
        if(NOT EXISTS ${CI_DIR}/${alias})
            file(CREATE_LINK ${h} ${CI_DIR}/${alias} SYMBOLIC)
        endif()
    endforeach()
endforeach()
# Any remaining spelling actually used in an #include gets its own link.
file(GLOB _srcs ${DOOM_ROOT}/src/* ${DOOM_ROOT}/hdrs/*)
set(_used)
foreach(f ${_srcs})
    file(STRINGS ${f} _lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"]+[>\"]")
    foreach(l ${_lines})
        string(REGEX REPLACE ".*include[ \t]*[<\"]([^>\"]+)[>\"].*" "\\1" inc "${l}")
        list(APPEND _used ${inc})
    endforeach()
endforeach()
list(REMOVE_DUPLICATES _used)
foreach(h ${_hdrs})
    get_filename_component(n ${h} NAME)
    string(TOLOWER ${n} lo)
    foreach(inc ${_used})
        string(TOLOWER ${inc} inc_lo)
        if(inc_lo STREQUAL lo AND NOT EXISTS ${CI_DIR}/${inc})
            file(CREATE_LINK ${h} ${CI_DIR}/${inc} SYMBOLIC)
        endif()
    endforeach()
endforeach()

set(DOOM_SOURCES
    About.c AM_MAP.C DUTILS.C D_MAIN.C D_NET.C F_FINALE.C G_GAME.C Help.c
    HU_LIB.C HU_STUFF.C INFO.C I_IBM.C I_MAIN.C I_SOUND.C KeyConfig.c
    MacAllocA.c M_MENU.C M_MISC.C Offscreen.c
    P_CEILNG.C P_DOORS.C P_ENEMY.C P_FLOOR.C P_INTER.C P_LIGHTS.C P_MAP.C
    P_MAPUTL.C P_MOBJ.C P_PLATS.C P_PSPR.C P_SETUP.C P_SIGHT.C P_SPEC.C
    P_SWITCH.C P_TELEPT.C P_TICK.C P_USER.C
    R_BSP.C R_DATA.C R_DRAW.C R_MAIN.C R_PLANE.C R_SEGS.C R_THINGS.C
    SOUNDS.C ST_LIB.C ST_STUFF.C S_SOUND.C TABLES.C V_VIDEO.C WI_STUFF.C
    W_WAD.C Z_ZONE.C
    SerialNet.c NetDialogs.c AppleTalkNet.c)
list(TRANSFORM DOOM_SOURCES PREPEND ${DOOM_ROOT}/src/)
# gcc would otherwise compile the upper-case .C files as C++.
set_source_files_properties(${DOOM_SOURCES} PROPERTIES
    LANGUAGE C
    COMPILE_OPTIONS "-xc")

add_application(UltimateDOOM
    ${DOOM_SOURCES}
    src/net_stubs.c
    src/mac_glue.c
    src/mac_bench.c
    src/tcpnet.c
    src/appletalk_glue.c
    src/mac_menus.c
    src/mac_wads.c
    src/deh.c
    src/mus2mid.c
    src/Draw68K.s
    src/Blit68K.s
    rsrc/DoomShell.rsrc.bin
    ${CMAKE_CURRENT_BINARY_DIR}/UltimateDOOM.r
    CREATOR idSW
    TYPE APPL)

target_include_directories(UltimateDOOM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
    ${CI_DIR})

target_compile_options(UltimateDOOM PRIVATE
    -m${DOOM_CPU} -msoft-float
    SHELL:${DOOM_OPT}
    -fomit-frame-pointer
    -ffunction-sections
    -fno-strict-aliasing
    -fcommon
    $<$<COMPILE_LANGUAGE:C>:-std=gnu17>
    $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/compat/retro68_compat.h>
    -Wno-multichar -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-implicit-int -Wno-return-type -Wno-overflow -Wno-attributes -Wno-builtin-declaration-mismatch -Wno-discarded-qualifiers -Wno-endif-labels -Wno-comment
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>)

target_link_options(UltimateDOOM PRIVATE
    -m${DOOM_CPU} -msoft-float
    -Wl,-gc-sections)
if(DOOM_SINGLE_SEGMENT)
    target_link_options(UltimateDOOM PRIVATE -Wl,--mac-single)
endif()
