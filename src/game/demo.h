#ifndef DEMO_H
#define DEMO_H

#include "types.h"
#include <stddef.h>

extern struct DemoInputObj {
    u32 numEntries;
    const void *addrPlaceholder;
    struct OffsetSizePair entries[7];
    u8 bbh[988];
    u8 ccm[1320];
    u8 hmc[980];
    u8 jrb[620];
    u8 wf[672];
    u8 pss[748];
    u8 bitdw[1412];
} gDemoInputs; 

#endif