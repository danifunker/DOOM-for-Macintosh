#include "Processes.r"
#include "MacTypes.r"

resource 'vers' (1) {
    0x01, 0x02, release, 0x00, verUS,
    "1.0.2r68",
    "1.0.2 Retro68 build for 68040 / 68LC040"
};

/* 68K memory partition, sized for a 32 MB Quadra.  The zone allocator takes
   what it is given (up to 24 MB), and a bigger zone keeps more WAD lumps
   cached: 24 MB preferred, 12 MB minimum. */
resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    isHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    24 * 1024 * 1024,
    12 * 1024 * 1024
};
