#include "Processes.r"

/* 68K memory partition.  The zone allocator takes what it is given, and a
   bigger zone keeps more WAD lumps cached: 16 MB preferred, 6 MB minimum. */
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
    16 * 1024 * 1024,
    6 * 1024 * 1024
};
