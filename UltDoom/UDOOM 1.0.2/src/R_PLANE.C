#include "LionDoom.h"
#include <stdlib.h>

// R_planes.c

#include "doomdef.h"
#include "r_local.h"

#include "R_PLANE.PROTO.H"

planefunction_t		floorfunc = NULL, ceilingfunc = NULL;

//
// sky mapping
//
int			skyflatnum = 0;
int			skytexture = 0;
int			skytexturemid = 0;


//
// opening
//

// Limit removal: visplanes and openings grow as needed.  Vanilla DOOM
// quit with "R_FindPlane: no more visplanes" past 128, and overwrote memory
// when a frame needed more than MAXOPENINGS clip values.
visplane_t		*visplanes = NULL, *lastvisplane = NULL;
visplane_t		*floorplane = NULL, *ceilingplane = NULL;
static int		numvisplanes = 0;
short			*openings = NULL, *lastopening = NULL;
static int		numopenings = 0;

// Visplanes with the same height, flat and light are chained by index
// (the array moves when it grows), so R_FindPlane needn't scan them all.
#define VISPLANEHASH	128
static int		visplanehash[VISPLANEHASH];
#define VPHASH(h,p,l)	((((unsigned)(h) >> 16) * 3 + (unsigned)(p) * 7 + (unsigned)(l)) & (VISPLANEHASH - 1))

/*
================
=
= R_GrowVisplanes
=
= Makes room for one more visplane.  *keep (a visplane pointer the caller
= holds) and floorplane / ceilingplane follow the array if it moves.
=
================
*/

static void R_GrowVisplanes (visplane_t **keep)
{
	int			used = lastvisplane - visplanes;
	int			newnum;
	visplane_t	*n;

	if (visplanes && used < numvisplanes)
		return;
	newnum = numvisplanes ? numvisplanes * 2 : MAXVISPLANES;
	n = (visplane_t *) Z_Malloc(newnum * sizeof(visplane_t), PU_STATIC, NULL);
	if (visplanes)
	{
		memcpy(n, visplanes, used * sizeof(visplane_t));
		if (keep && *keep)
			*keep = n + (*keep - visplanes);
		if (floorplane)
			floorplane = n + (floorplane - visplanes);
		if (ceilingplane)
			ceilingplane = n + (ceilingplane - visplanes);
		Z_Free(visplanes);
	}
	visplanes = n;
	lastvisplane = n + used;
	numvisplanes = newnum;
}

/*
================
=
= R_EnsureOpenings
=
= Makes room for "needed" more clip values.  Drawsegs keep pointers into
= the openings (offset by -x1), so those move with the array.
=
================
*/

void R_EnsureOpenings (int needed)
{
	int			used = lastopening - openings;
	int			newnum;
	short		*n;
	drawseg_t	*ds;
	long		delta;

	if (openings && used + needed <= numopenings)
		return;
	newnum = numopenings ? numopenings : MAXOPENINGS;
	while (newnum < used + needed)
		newnum *= 2;
	n = (short *) Z_Malloc(newnum * sizeof(short), PU_STATIC, NULL);
	if (openings)
	{
		memcpy(n, openings, used * sizeof(short));
		delta = n - openings;
		for (ds = drawsegs; ds < ds_p; ds++)
		{
			if (ds->maskedtexturecol + ds->x1 >= openings &&
				ds->maskedtexturecol + ds->x1 < openings + used)
				ds->maskedtexturecol += delta;
			if (ds->sprtopclip + ds->x1 >= openings &&
				ds->sprtopclip + ds->x1 < openings + used)
				ds->sprtopclip += delta;
			if (ds->sprbottomclip + ds->x1 >= openings &&
				ds->sprbottomclip + ds->x1 < openings + used)
				ds->sprbottomclip += delta;
		}
		Z_Free(openings);
	}
	openings = n;
	lastopening = n + used;
	numopenings = newnum;
}

//
// clip values are the solid pixel bounding the range
// floorclip starts out SCREENHEIGHT
// ceilingclip starts out -1
//
short		floorclip[kHiResScreenWidth];
short		ceilingclip[kHiResScreenWidth];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int			spanstart[kHiResScreenHeight];
int			spanstop[kHiResScreenHeight];

//
// texture mapping
//
lighttable_t	**planezlight = NULL;
fixed_t		planeheight = 0;

fixed_t		yslope[kHiResScreenHeight];
fixed_t		distscale[kHiResScreenWidth];
fixed_t		basexscale = 0, baseyscale = 0;

fixed_t		cachedheight[kHiResScreenHeight];
fixed_t		cacheddistance[kHiResScreenHeight];
fixed_t		cachedxstep[kHiResScreenHeight];
fixed_t		cachedystep[kHiResScreenHeight];


/*
================
=
= R_InitSkyMap
=
= Called whenever the view size changes
=
================
*/

void R_InitSkyMap (void)
{
	skyflatnum = R_FlatNumForName ("F_SKY1");
	skytexturemid = 100*FRACUNIT;
}


/*
====================
=
= R_InitPlanes
=
= Only at game startup
====================
*/

void R_InitPlanes (void)
{
}


/*
================
=
= R_MapPlane
=
global vars:

planeheight
ds_source
basexscale
baseyscale
viewx
viewy

BASIC PRIMITIVE
================
*/

void R_MapPlane (int y, int x1, int x2)
{
	angle_t		angle;
	fixed_t		distance, length;
	unsigned	index;
	
#ifdef RANGECHECK
	if (x2 < x1 || x1<0 || x2>=viewwidth || (unsigned)y>viewheight)
		I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
#endif

	if (planeheight != cachedheight[y])
	{
		cachedheight[y] = planeheight;
		distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
		ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
		ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
	}
	else
	{
		distance = cacheddistance[y];
		ds_xstep = cachedxstep[y];
		ds_ystep = cachedystep[y];
	}
	
	length = FixedMul (distance, distscale[x1]);
	angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;
	ds_xfrac = viewx + FixedMul(finecosine[angle], length);
	ds_yfrac = -viewy - FixedMul(finesine[angle], length);
	
	if (fixedcolormap)
		ds_colormap = fixedcolormap;
	else
	{
		index = distance >> LIGHTZSHIFT;
		if (index >= MAXLIGHTZ )
			index = MAXLIGHTZ-1;
		ds_colormap = planezlight[index];
	}
	
	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;
	
	if ((gHiRes != 1) || (!(ds_y & 0x01)))
		spanfunc ();		// high or low detail
}

//=============================================================================

/*
====================
=
= R_ClearPlanes
=
= At begining of frame
====================
*/

void R_ClearPlanes (void)
{
	int		i;
	angle_t	angle;
	
//
// opening / clipping determination
//	
	for (i=0 ; i<viewwidth ; i++)
	{
		floorclip[i] = viewheight;
		ceilingclip[i] = -1;
	}

	if (!visplanes)
		R_GrowVisplanes (NULL);
	if (!openings)
		R_EnsureOpenings (MAXOPENINGS);
	lastvisplane = visplanes;
	lastopening = openings;
	for (i = 0; i < VISPLANEHASH; i++)
		visplanehash[i] = -1;
	
//
// texture calculation
//
	memset (cachedheight, 0, sizeof(cachedheight));	
	angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;	// left to right mapping
	
	// scale will be unit scale at SCREENWIDTH/2 distance
	basexscale = FixedDiv (finecosine[angle],centerxfrac);
	baseyscale = -FixedDiv (finesine[angle],centerxfrac);
}



/*
===============
=
= R_FindPlane
=
===============
*/

visplane_t *R_FindPlane (fixed_t height, int picnum, int lightlevel)
{
	visplane_t	*check;
	
	if (picnum == skyflatnum)
	{
		height = 0;			// all skys map together
		lightlevel = 0;
	}
	
	{
		int		hash = VPHASH(height, picnum, lightlevel);
		int		i;

		// The chain runs newest first; the vanilla scan found the oldest
		// match, so take the last one in the chain.
		check = NULL;
		for (i = visplanehash[hash]; i >= 0; i = visplanes[i].hashnext)
			if (height == visplanes[i].height
			&& picnum == visplanes[i].picnum
			&& lightlevel == visplanes[i].lightlevel)
				check = &visplanes[i];
		if (check)
			return check;

		R_GrowVisplanes (NULL);
		check = lastvisplane++;
		check->hashnext = visplanehash[hash];
		visplanehash[hash] = check - visplanes;
	}
	check->height = height;
	check->picnum = picnum;
	check->lightlevel = lightlevel;
	if (gHiRes)
		check->minx = kHiResScreenWidth;
	else
		check->minx = kScreenWidth;
	check->maxx = -1;
// ее Optimize: don't use memset: PowerPC-specific version
	memset (check->top, 0xFF, sizeof(check->top));
	
	return check;
}

/*
===============
=
= R_CheckPlane
=
===============
*/

visplane_t *R_CheckPlane (visplane_t *pl, int start, int stop)
{
	int			intrl, intrh;
	int			unionl, unionh;
	int			x;
	
	if (start < pl->minx)
	{
		intrl = pl->minx;
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
		intrl = start;
	}
	
	if (stop > pl->maxx)
	{
		intrh = pl->maxx;
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
		intrh = stop;
	}

	for (x = intrl; x <= intrh; x++)
		if (pl->top[x] != ((unsigned short)0xffff))
			break;

	if (x > intrh)
	{
		pl->minx = unionl;
		pl->maxx = unionh;
		return pl;			// use the same one
	}
	
// make a new visplane (not hashed: R_FindPlane keeps finding the first one)
	R_GrowVisplanes (&pl);
	lastvisplane->hashnext = -1;
	lastvisplane->height = pl->height;
	lastvisplane->picnum = pl->picnum;
	lastvisplane->lightlevel = pl->lightlevel;
	pl = lastvisplane++;
	pl->minx = start;
	pl->maxx = stop;
// ее Optimize: don't use memset, PowerPC-specific version
	memset (pl->top, 0xff, sizeof(pl->top));
	
	return pl;
}



//=============================================================================

/*
================
=
= R_MakeSpans
=
================
*/

void R_MakeSpans (int x, int t1, int b1, int t2, int b2)
{
	while (t1 < t2 && t1<=b1)
	{
		R_MapPlane (t1, spanstart[t1], x - 1);
		t1++;
	}
	while (b1 > b2 && b1>=t1)
	{
		R_MapPlane (b1, spanstart[b1], x - 1);
		b1--;
	}
	
	while (t2 < t1 && t2 <= b2)
	{
		spanstart[t2] = x;
		t2++;
	}
	
	while (b2 > b1 && b2 >= t2)
	{
		spanstart[b2] = x;
		b2--;
	}
}



/*
================
=
= R_DrawPlanes
=
= At the end of each frame
================
*/

void R_DrawPlanes (void)
{
	visplane_t	*pl;
	int			light;
	int			x, stop;
	int			angle;
				

	for (pl = visplanes; pl < lastvisplane; pl++)
	{
		if (pl->minx > pl->maxx)
			continue;
	//
	// sky flat
	//
		if (pl->picnum == skyflatnum)
		{
			dc_iscale = pspriteiscale;
			dc_colormap = colormaps;		// sky is allways drawn full bright
			dc_texturemid = skytexturemid;
			for (x = pl->minx; x <= pl->maxx; x++)
			{
				dc_yl = pl->top[x];
				dc_yh = pl->bottom[x];
				if (dc_yl <= dc_yh)
				{
					angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;
					dc_x = x;
					dc_source = R_GetColumn(skytexture, angle);
					colfunc ();
				}
			}
			continue;
		}
		
	//
	// regular flat
	//
		ds_source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_STATIC);
		planeheight = abs(pl->height - viewz);
		light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;
		if (light >= LIGHTLEVELS)
			light = LIGHTLEVELS - 1;
		if (light < 0)
			light = 0;
		planezlight = zlight[light];

		pl->top[pl->maxx + 1] = 0xffff;
		pl->top[pl->minx - 1] = 0xffff;
		
		stop = pl->maxx + 1;
		
		for (x = pl->minx; x <= stop; x++)
			R_MakeSpans (x, pl->top[x - 1], pl->bottom[x - 1], pl->top[x], pl->bottom[x]);
		
		Z_ChangeTag (ds_source, PU_CACHE);
	}
}
