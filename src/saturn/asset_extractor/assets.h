#ifndef ASSETEXTRACT_ASSETS_H
#define ASSETEXTRACT_ASSETS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef STRUCT_ONLY
#include "saturn/libs/dynamics.h"
#endif

typedef struct {
    const char* name;

    enum {
        AssetType_Raw,
        AssetType_MIO0,
        AssetType_Tiled,
        AssetType_Demo,
        AssetType_Sound,
        AssetType_TBL,
        AssetType_CTL,
        AssetType_Dummy,
    } type;
    uint64_t offset;
    union { uint64_t mio0_offset, index; };
    uint64_t size;
    uint8_t* data;
} Asset;

#ifndef STRUCT_ONLY
void assetextract_populate_assets(set(Asset)* assets);
#endif

#endif