#ifndef ASSET_EXTRACTOR_H
#define ASSET_EXTRACTOR_H

#include <stdbool.h>
#include <stddef.h>

bool assetextract_init(const char* rom_dest_path);
void* assetextract_get_asset(const char* name, size_t* size);

#endif