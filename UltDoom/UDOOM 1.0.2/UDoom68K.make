#   File:       UDoom68K.make
#   Target:     UDoom68K
#   Created:    Thursday, August 31, 1995 03:15:29 AM


MAKEFILE     = UDoom68K.make
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i :hdrs: ¶
		-i :sys:LionIncludes: ¶
		-i :sys:MacHeadersÄ: ¶
		-i HD:MPW:Interfaces:CIncludes:
Sym¥68K      = 
ObjDir¥68K   = ":OBJ68K"

COptions     = {Includes} {Sym¥68K} -model far -mc68020 -b3 -d __MPW_VERSION__ ¶
	-m -mbg off -opt full

Objects¥68K  = ¶
		{ObjDir¥68K}:src:About.c.o ¶
		{ObjDir¥68K}:src:AM_MAP.C.o ¶
		{ObjDir¥68K}:src:AppleTalkNet.c.o ¶
		{ObjDir¥68K}:src:CTBBuffer.c.o ¶
		{ObjDir¥68K}:src:CTBNet.c.o ¶
		{ObjDir¥68K}:src:DUTILS.C.o ¶
		{ObjDir¥68K}:src:D_MAIN.C.o ¶
		{ObjDir¥68K}:src:D_NET.C.o ¶
		{ObjDir¥68K}:src:F_FINALE.C.o ¶
		{ObjDir¥68K}:src:G_GAME.C.o ¶
		{ObjDir¥68K}:src:Help.c.o ¶
		{ObjDir¥68K}:src:HU_LIB.C.o ¶
		{ObjDir¥68K}:src:HU_STUFF.C.o ¶
		{ObjDir¥68K}:src:INFO.C.o ¶
		{ObjDir¥68K}:src:IPXNet.c.o ¶
		{ObjDir¥68K}:src:IPXSetup.c.o ¶
		{ObjDir¥68K}:src:I_IBM.C.o ¶
		{ObjDir¥68K}:src:I_MAIN.C.o ¶
		{ObjDir¥68K}:src:I_SOUND.C.o ¶
		{ObjDir¥68K}:src:KeyConfig.c.o ¶
		{ObjDir¥68K}:src:MacAllocA.c.o ¶
		{ObjDir¥68K}:src:M_MENU.C.o ¶
		{ObjDir¥68K}:src:M_MISC.C.o ¶
		{ObjDir¥68K}:src:NetDialogs.c.o ¶
		{ObjDir¥68K}:src:Offscreen.c.o ¶
		{ObjDir¥68K}:src:P_CEILNG.C.o ¶
		{ObjDir¥68K}:src:P_DOORS.C.o ¶
		{ObjDir¥68K}:src:P_ENEMY.C.o ¶
		{ObjDir¥68K}:src:P_FLOOR.C.o ¶
		{ObjDir¥68K}:src:P_INTER.C.o ¶
		{ObjDir¥68K}:src:P_LIGHTS.C.o ¶
		{ObjDir¥68K}:src:P_MAP.C.o ¶
		{ObjDir¥68K}:src:P_MAPUTL.C.o ¶
		{ObjDir¥68K}:src:P_MOBJ.C.o ¶
		{ObjDir¥68K}:src:P_PLATS.C.o ¶
		{ObjDir¥68K}:src:P_PSPR.C.o ¶
		{ObjDir¥68K}:src:P_SETUP.C.o ¶
		{ObjDir¥68K}:src:P_SIGHT.C.o ¶
		{ObjDir¥68K}:src:P_SPEC.C.o ¶
		{ObjDir¥68K}:src:P_SWITCH.C.o ¶
		{ObjDir¥68K}:src:P_TELEPT.C.o ¶
		{ObjDir¥68K}:src:P_TICK.C.o ¶
		{ObjDir¥68K}:src:P_USER.C.o ¶
		{ObjDir¥68K}:src:R_BSP.C.o ¶
		{ObjDir¥68K}:src:R_DATA.C.o ¶
		{ObjDir¥68K}:src:R_DRAW.C.o ¶
		{ObjDir¥68K}:src:R_MAIN.C.o ¶
		{ObjDir¥68K}:src:R_PLANE.C.o ¶
		{ObjDir¥68K}:src:R_SEGS.C.o ¶
		{ObjDir¥68K}:src:R_THINGS.C.o ¶
		{ObjDir¥68K}:src:SerialNet.c.o ¶
		{ObjDir¥68K}:src:SktListener.a.o ¶
		{ObjDir¥68K}:src:SOUNDS.C.o ¶
		{ObjDir¥68K}:src:ST_LIB.C.o ¶
		{ObjDir¥68K}:src:ST_STUFF.C.o ¶
		{ObjDir¥68K}:src:S_SOUND.C.o ¶
		{ObjDir¥68K}:src:TABLES.C.o ¶
		{ObjDir¥68K}:src:V_VIDEO.C.o ¶
		{ObjDir¥68K}:src:WI_STUFF.C.o ¶
		{ObjDir¥68K}:src:W_WAD.C.o ¶
		{ObjDir¥68K}:src:Z_ZONE.C.o


UDoom68K ÄÄ {¥MondoBuild¥} {Objects¥68K}
	Link ¶
		-o {Targ} -d {Sym¥68K} ¶
		{Objects¥68K} ¶
		-t 'APPL' ¶
		-c 'idSW' ¶
		-model far ¶
		"{Libraries}"MathLib.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"ToolLibs.o ¶
		"{Libraries}"Interface.o


{ObjDir¥68K}:src:About.c.o Ä {¥MondoBuild¥} :src:About.c ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:DoomResources.h ¶
                             :hdrs:doomdef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:ST_STUFF.PROTO.H ¶
                             :hdrs:HU_STUFF.H
	{C} :src:About.c -o {Targ} {COptions} -s "SEG08"

{ObjDir¥68K}:src:AM_MAP.C.o Ä {¥MondoBuild¥} :src:AM_MAP.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:st_stuff.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:am_map.h ¶
                              :hdrs:am_data.h ¶
                              :hdrs:dutils.h ¶
                              :hdrs:AM_MAP.PROTO.H ¶
                              :hdrs:DoomResources.h
	{C} :src:AM_MAP.C -o {Targ} {COptions} -s "SEG02"

{ObjDir¥68K}:src:AppleTalkNet.c.o Ä {¥MondoBuild¥} :src:AppleTalkNet.c ¶
                                    :hdrs:LionDoom.h ¶
                                    :hdrs:DebugSwitches.h ¶
                                    :hdrs:DoomResources.h ¶
                                    :hdrs:PCMacNet.h ¶
                                    :hdrs:doomdef.h ¶
                                    :hdrs:doomdata.h ¶
                                    :hdrs:d_french.h ¶
                                    :hdrs:dstrings.h ¶
                                    :hdrs:info.h ¶
                                    :hdrs:sounds.h ¶
                                    :hdrs:soundst.h ¶
                                    :hdrs:AppleTalkNet.h ¶
                                    HD:MPW:Interfaces:CIncludes:Traps.h ¶
                                    :hdrs:DOOMDEF.H
	{C} :src:AppleTalkNet.c -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:CTBBuffer.c.o Ä {¥MondoBuild¥} :src:CTBBuffer.c ¶
                                 :hdrs:LionDoom.h ¶
                                 :hdrs:Doomdef.h ¶
                                 :hdrs:doomdata.h ¶
                                 :hdrs:d_french.h ¶
                                 :hdrs:dstrings.h ¶
                                 :hdrs:info.h ¶
                                 :hdrs:sounds.h ¶
                                 :hdrs:soundst.h ¶
                                 :hdrs:CTBBuffer.h ¶
                                 :hdrs:NetBuffer.h ¶
                                 :hdrs:doomdef.h ¶
                                 :hdrs:MacPCSwitches.h ¶
                                 :hdrs:DebugSwitches.h
	{C} :src:CTBBuffer.c -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:CTBNet.c.o Ä {¥MondoBuild¥} :src:CTBNet.c ¶
                              :hdrs:CTBNet.h ¶
                              :hdrs:CTBBuffer.h ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:MacPCSwitches.h ¶
                              :hdrs:DebugSwitches.h ¶
                              :hdrs:PCMacNet.h
	{C} :src:CTBNet.c -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:DUTILS.C.o Ä {¥MondoBuild¥} :src:DUTILS.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:dutils.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:DUTILS.PROTO.H
	{C} :src:DUTILS.C -o {Targ} {COptions} -s "SEG04"

{ObjDir¥68K}:src:D_MAIN.C.o Ä {¥MondoBuild¥} :src:D_MAIN.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:PCMacNet.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:dutils.h ¶
                              :hdrs:DebugSwitches.h ¶
                              :hdrs:D_MAIN.PROTO.H
	{C} :src:D_MAIN.C -o {Targ} {COptions} -s "SEG01"

{ObjDir¥68K}:src:D_NET.C.o Ä {¥MondoBuild¥} :src:D_NET.C ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:AppleTalkNet.h ¶
                             :hdrs:SerialNet.h ¶
                             :hdrs:CTBNet.h ¶
                             :hdrs:doomdef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:MacPCSwitches.h ¶
                             :hdrs:DebugSwitches.h ¶
                             :hdrs:NetDialogs.h ¶
                             :hdrs:DoomResources.h ¶
                             :hdrs:D_NET.PROTO.H
	{C} :src:D_NET.C -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:F_FINALE.C.o Ä {¥MondoBuild¥} :src:F_FINALE.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:F_FINALE.PROTO.H ¶
                                :hdrs:hu_stuff.h
	{C} :src:F_FINALE.C -o {Targ} {COptions} -s "SEG02"

{ObjDir¥68K}:src:G_GAME.C.o Ä {¥MondoBuild¥} :src:G_GAME.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:DebugSwitches.h ¶
                              :hdrs:doomresources.h ¶
                              :hdrs:G_GAME.PROTO.H
	{C} :src:G_GAME.C -o {Targ} {COptions} -s "SEG08"

{ObjDir¥68K}:src:Help.c.o Ä {¥MondoBuild¥} :src:Help.c ¶
                            :hdrs:LionDoom.h ¶
                            :hdrs:DoomResources.h ¶
                            :hdrs:doomdef.h ¶
                            :hdrs:doomdata.h ¶
                            :hdrs:d_french.h ¶
                            :hdrs:dstrings.h ¶
                            :hdrs:info.h ¶
                            :hdrs:sounds.h ¶
                            :hdrs:soundst.h ¶
                            :hdrs:ST_STUFF.PROTO.H ¶
                            :hdrs:HU_STUFF.H
	{C} :src:Help.c -o {Targ} {COptions} -s "SEG08"

{ObjDir¥68K}:src:HU_LIB.C.o Ä {¥MondoBuild¥} :src:HU_LIB.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:hu_lib.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:HU_LIB.PROTO.H
	{C} :src:HU_LIB.C -o {Targ} {COptions} -s "SEG07"

{ObjDir¥68K}:src:HU_STUFF.C.o Ä {¥MondoBuild¥} :src:HU_STUFF.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:hu_stuff.h ¶
                                :hdrs:hu_lib.h ¶
                                :hdrs:HU_STUFF.PROTO.H
	{C} :src:HU_STUFF.C -o {Targ} {COptions} -s "SEG07"

{ObjDir¥68K}:src:INFO.C.o Ä {¥MondoBuild¥} :src:INFO.C ¶
                            :hdrs:LionDoom.h ¶
                            :hdrs:doomdef.h ¶
                            :hdrs:doomdata.h ¶
                            :hdrs:d_french.h ¶
                            :hdrs:dstrings.h ¶
                            :hdrs:info.h ¶
                            :hdrs:sounds.h ¶
                            :hdrs:soundst.h
	{C} :src:INFO.C -o {Targ} {COptions} -s "SEG02"

{ObjDir¥68K}:src:IPXNet.c.o Ä {¥MondoBuild¥} :src:IPXNet.c ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:IPXNet.h ¶
                              :hdrs:DoomDef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:DebugSwitches.h
	{C} :src:IPXNet.c -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:IPXSetup.c.o Ä {¥MondoBuild¥} :src:IPXSetup.c ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:IPXNet.h ¶
                                :hdrs:DoomDef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:DebugSwitches.h
	{C} :src:IPXSetup.c -o {Targ} {COptions} -s "SEG03"

{ObjDir¥68K}:src:I_IBM.C.o Ä {¥MondoBuild¥} :src:I_IBM.C ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:DoomResources.h ¶
                             :hdrs:I_MAIN.PROTO.H ¶
                             :hdrs:D_NET.PROTO.H ¶
                             :hdrs:DoomDef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:R_local.h ¶
                             :hdrs:i_sound.h ¶
                             :hdrs:I_SOUND.PROTO.H ¶
                             :hdrs:AppleTalkNet.h ¶
                             :hdrs:SerialNet.h ¶
                             :hdrs:I_IBM.PROTO.H
	{C} :src:I_IBM.C -o {Targ} {COptions}

{ObjDir¥68K}:src:I_MAIN.C.o Ä {¥MondoBuild¥} :src:I_MAIN.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:DoomResources.h ¶
                              :hdrs:CTBNet.h ¶
                              :hdrs:NetDialogs.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:MacPCSwitches.h ¶
                              :hdrs:Offscreen.h ¶
                              :hdrs:I_MAIN.PROTO.H ¶
                              :hdrs:MACALLOCA.PROTO.H ¶
                              :hdrs:r_local.h ¶
                              :hdrs:DebugSwitches.h
	{C} :src:I_MAIN.C -o {Targ} {COptions}

{ObjDir¥68K}:src:I_SOUND.C.o Ä {¥MondoBuild¥} :src:I_SOUND.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:i_sound.h ¶
                               :hdrs:I_SOUND.PROTO.H ¶
                               :hdrs:MacSound.h
	{C} :src:I_SOUND.C -o {Targ} {COptions}

{ObjDir¥68K}:src:KeyConfig.c.o Ä {¥MondoBuild¥} :src:KeyConfig.c ¶
                                 :hdrs:LionDoom.h ¶
                                 :hdrs:DoomResources.h
	{C} :src:KeyConfig.c -o {Targ} {COptions}

{ObjDir¥68K}:src:MacAllocA.c.o Ä {¥MondoBuild¥} :src:MacAllocA.c ¶
                                 :hdrs:LionDoom.h ¶
                                 :hdrs:DoomResources.h ¶
                                 :hdrs:MacAllocA.proto.h
	{C} :src:MacAllocA.c -o {Targ} {COptions}

{ObjDir¥68K}:src:M_MENU.C.o Ä {¥MondoBuild¥} :src:M_MENU.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:hu_stuff.h ¶
                              :hdrs:M_MENU.PROTO.H
	{C} :src:M_MENU.C -o {Targ} {COptions}

{ObjDir¥68K}:src:M_MISC.C.o Ä {¥MondoBuild¥} :src:M_MISC.C ¶
                              :hdrs:LionDoom.h ¶
                              :sys:LionIncludes:Lion.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:hu_stuff.h ¶
                              :hdrs:M_MISC.PROTO.H ¶
                              :hdrs:R_DATA.PROTO.H
	{C} :src:M_MISC.C -o {Targ} {COptions}

{ObjDir¥68K}:src:NetDialogs.c.o Ä {¥MondoBuild¥} :src:NetDialogs.c ¶
                                  :hdrs:LionDoom.h ¶
                                  :hdrs:DoomResources.h ¶
                                  :hdrs:AppleTalkNet.h ¶
                                  :hdrs:SerialNet.h ¶
                                  :hdrs:CTBNet.h ¶
                                  :hdrs:doomdef.h ¶
                                  :hdrs:doomdata.h ¶
                                  :hdrs:d_french.h ¶
                                  :hdrs:dstrings.h ¶
                                  :hdrs:info.h ¶
                                  :hdrs:sounds.h ¶
                                  :hdrs:soundst.h ¶
                                  :hdrs:MacPCSwitches.h ¶
                                  :hdrs:NetDialogs.h
	{C} :src:NetDialogs.c -o {Targ} {COptions}

{ObjDir¥68K}:src:Offscreen.c.o Ä {¥MondoBuild¥} :src:Offscreen.c ¶
                                 :hdrs:LionDoom.h ¶
                                 :hdrs:offscreen.h
	{C} :src:Offscreen.c -o {Targ} {COptions}

{ObjDir¥68K}:src:P_CEILNG.C.o Ä {¥MondoBuild¥} :src:P_CEILNG.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:P_CEILNG.PROTO.H
	{C} :src:P_CEILNG.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_DOORS.C.o Ä {¥MondoBuild¥} :src:P_DOORS.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_DOORS.PROTO.H
	{C} :src:P_DOORS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_ENEMY.C.o Ä {¥MondoBuild¥} :src:P_ENEMY.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:DoomResources.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_ENEMY.PROTO.H
	{C} :src:P_ENEMY.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_FLOOR.C.o Ä {¥MondoBuild¥} :src:P_FLOOR.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_FLOOR.PROTO.H
	{C} :src:P_FLOOR.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_INTER.C.o Ä {¥MondoBuild¥} :src:P_INTER.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_INTER.PROTO.H
	{C} :src:P_INTER.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_LIGHTS.C.o Ä {¥MondoBuild¥} :src:P_LIGHTS.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:P_LIGHTS.PROTO.H
	{C} :src:P_LIGHTS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_MAP.C.o Ä {¥MondoBuild¥} :src:P_MAP.C ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:DoomResources.h ¶
                             :hdrs:doomdef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:p_local.h ¶
                             :hdrs:r_local.h ¶
                             :hdrs:p_spec.h ¶
                             :hdrs:P_MAP.PROTO.H
	{C} :src:P_MAP.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_MAPUTL.C.o Ä {¥MondoBuild¥} :src:P_MAPUTL.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:DoomResources.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:P_MAPUTL.PROTO.H
	{C} :src:P_MAPUTL.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_MOBJ.C.o Ä {¥MondoBuild¥} :src:P_MOBJ.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:P_MOBJ.PROTO.H
	{C} :src:P_MOBJ.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_PLATS.C.o Ä {¥MondoBuild¥} :src:P_PLATS.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_PLATS.PROTO.H
	{C} :src:P_PLATS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_PSPR.C.o Ä {¥MondoBuild¥} :src:P_PSPR.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:P_PSPR.PROTO.H
	{C} :src:P_PSPR.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_SETUP.C.o Ä {¥MondoBuild¥} :src:P_SETUP.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_DOORS.PROTO.h ¶
                               :hdrs:P_SETUP.PROTO.H ¶
                               :hdrs:P_LIGHTS.PROTO.H
	{C} :src:P_SETUP.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_SIGHT.C.o Ä {¥MondoBuild¥} :src:P_SIGHT.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:P_SIGHT.PROTO.H
	{C} :src:P_SIGHT.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_SPEC.C.o Ä {¥MondoBuild¥} :src:P_SPEC.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:P_SPEC.PROTO.H
	{C} :src:P_SPEC.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_SWITCH.C.o Ä {¥MondoBuild¥} :src:P_SWITCH.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:P_SWITCH.PROTO.H
	{C} :src:P_SWITCH.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_TELEPT.C.o Ä {¥MondoBuild¥} :src:P_TELEPT.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:P_TELEPT.PROTO.H
	{C} :src:P_TELEPT.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_TICK.C.o Ä {¥MondoBuild¥} :src:P_TICK.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:P_TICK.PROTO.H
	{C} :src:P_TICK.C -o {Targ} {COptions}

{ObjDir¥68K}:src:P_USER.C.o Ä {¥MondoBuild¥} :src:P_USER.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:P_USER.PROTO.H
	{C} :src:P_USER.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_BSP.C.o Ä {¥MondoBuild¥} :src:R_BSP.C ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:doomdef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:r_local.h ¶
                             :hdrs:R_BSP.PROTO.H
	{C} :src:R_BSP.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_DATA.C.o Ä {¥MondoBuild¥} :src:R_DATA.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:MacAllocA.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:p_local.h ¶
                              :hdrs:p_spec.h ¶
                              :hdrs:R_DATA.PROTO.H
	{C} :src:R_DATA.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_DRAW.C.o Ä {¥MondoBuild¥} :src:R_DRAW.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:r_local.h
	{C} :src:R_DRAW.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_MAIN.C.o Ä {¥MondoBuild¥} :src:R_MAIN.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:DoomResources.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:R_MAIN.PROTO.H
	{C} :src:R_MAIN.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_PLANE.C.o Ä {¥MondoBuild¥} :src:R_PLANE.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:R_PLANE.PROTO.H
	{C} :src:R_PLANE.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_SEGS.C.o Ä {¥MondoBuild¥} :src:R_SEGS.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:R_SEGS.PROTO.H
	{C} :src:R_SEGS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:R_THINGS.C.o Ä {¥MondoBuild¥} :src:R_THINGS.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:R_THINGS.PROTO.H
	{C} :src:R_THINGS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:SerialNet.c.o Ä {¥MondoBuild¥} :src:SerialNet.c ¶
                                 :hdrs:LionDoom.h ¶
                                 :hdrs:SerialNet.h ¶
                                 :hdrs:doomdef.h ¶
                                 :hdrs:doomdata.h ¶
                                 :hdrs:d_french.h ¶
                                 :hdrs:dstrings.h ¶
                                 :hdrs:info.h ¶
                                 :hdrs:sounds.h ¶
                                 :hdrs:soundst.h ¶
                                 :hdrs:MacPCSwitches.h ¶
                                 :hdrs:NetBuffer.h ¶
                                 :hdrs:DebugSwitches.h ¶
                                 :hdrs:PCMacNet.h
	{C} :src:SerialNet.c -o {Targ} {COptions}

{ObjDir¥68K}:src:SktListener.a.o Ä {¥MondoBuild¥} :src:SktListener.a
	{Asm} :src:SktListener.a -o {Targ} {AOptions}

{ObjDir¥68K}:src:SOUNDS.C.o Ä {¥MondoBuild¥} :src:SOUNDS.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h
	{C} :src:SOUNDS.C -o {Targ} {COptions}

{ObjDir¥68K}:src:ST_LIB.C.o Ä {¥MondoBuild¥} :src:ST_LIB.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:st_stuff.h ¶
                              :hdrs:st_lib.h ¶
                              :hdrs:r_local.h ¶
                              :hdrs:ST_LIB.PROTO.H
	{C} :src:ST_LIB.C -o {Targ} {COptions}

{ObjDir¥68K}:src:ST_STUFF.C.o Ä {¥MondoBuild¥} :src:ST_STUFF.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:st_stuff.h ¶
                                :hdrs:st_lib.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:p_local.h ¶
                                :hdrs:p_spec.h ¶
                                :hdrs:am_map.h ¶
                                :hdrs:dutils.h ¶
                                :hdrs:ST_STUFF.PROTO.H
	{C} :src:ST_STUFF.C -o {Targ} {COptions}

{ObjDir¥68K}:src:S_SOUND.C.o Ä {¥MondoBuild¥} :src:S_SOUND.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:p_local.h ¶
                               :hdrs:r_local.h ¶
                               :hdrs:p_spec.h ¶
                               :hdrs:S_SOUND.PROTO.H
	{C} :src:S_SOUND.C -o {Targ} {COptions}

{ObjDir¥68K}:src:TABLES.C.o Ä {¥MondoBuild¥} :src:TABLES.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h
	{C} :src:TABLES.C -o {Targ} {COptions}

{ObjDir¥68K}:src:V_VIDEO.C.o Ä {¥MondoBuild¥} :src:V_VIDEO.C ¶
                               :hdrs:LionDoom.h ¶
                               :hdrs:doomdef.h ¶
                               :hdrs:doomdata.h ¶
                               :hdrs:d_french.h ¶
                               :hdrs:dstrings.h ¶
                               :hdrs:info.h ¶
                               :hdrs:sounds.h ¶
                               :hdrs:soundst.h ¶
                               :hdrs:V_VIDEO.PROTO.H
	{C} :src:V_VIDEO.C -o {Targ} {COptions}

{ObjDir¥68K}:src:WI_STUFF.C.o Ä {¥MondoBuild¥} :src:WI_STUFF.C ¶
                                :hdrs:LionDoom.h ¶
                                :hdrs:wi_data.h ¶
                                :hdrs:wi_stuff.h ¶
                                :hdrs:doomdef.h ¶
                                :hdrs:doomdata.h ¶
                                :hdrs:d_french.h ¶
                                :hdrs:dstrings.h ¶
                                :hdrs:info.h ¶
                                :hdrs:sounds.h ¶
                                :hdrs:soundst.h ¶
                                :hdrs:dutils.h ¶
                                :hdrs:r_local.h ¶
                                :hdrs:WI_STUFF.PROTO.H
	{C} :src:WI_STUFF.C -o {Targ} {COptions}

{ObjDir¥68K}:src:W_WAD.C.o Ä {¥MondoBuild¥} :src:W_WAD.C ¶
                             :hdrs:LionDoom.h ¶
                             :hdrs:doomdef.h ¶
                             :hdrs:doomdata.h ¶
                             :hdrs:d_french.h ¶
                             :hdrs:dstrings.h ¶
                             :hdrs:info.h ¶
                             :hdrs:sounds.h ¶
                             :hdrs:soundst.h ¶
                             :hdrs:DoomResources.h ¶
                             :hdrs:W_WAD.PROTO.H
	{C} :src:W_WAD.C -o {Targ} {COptions}

{ObjDir¥68K}:src:Z_ZONE.C.o Ä {¥MondoBuild¥} :src:Z_ZONE.C ¶
                              :hdrs:LionDoom.h ¶
                              :hdrs:doomdef.h ¶
                              :hdrs:doomdata.h ¶
                              :hdrs:d_french.h ¶
                              :hdrs:dstrings.h ¶
                              :hdrs:info.h ¶
                              :hdrs:sounds.h ¶
                              :hdrs:soundst.h ¶
                              :hdrs:Z_ZONE.PROTO.H
	{C} :src:Z_ZONE.C -o {Targ} {COptions}

