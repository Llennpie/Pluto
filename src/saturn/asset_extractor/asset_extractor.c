#include "asset_extractor.h"
#include "assets.h"
#include "sound.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>

#include "saturn/libs/tinyfiledialogs.h"
#include "saturn/libs/dynamics.h"

#include "game/demo.h"

#define CHECKSUM 0x3ce60709

typedef struct {
    uint32_t unused;
    uint32_t dest_size;
    uint32_t comp_offset;
    uint32_t uncomp_offset;
} MIO0_Header;

typedef enum {
    ROM_SUCCESS,
    ROM_CANNOT_OPEN,
    ROM_INVALID_FILESIZE,
    ROM_CHECKSUM_FAILED,
} ROMError;

map(uint64_t, uint8_t*) MIO0_Cache;

static int curr_asset;
static set(MIO0_Cache) mio0_cache;
static set(Asset) assets;

static uint8_t rom[8*1024*1024];

typedef const Texture* SkyboxTexture[80];
extern SkyboxTexture* sSkyboxTextures[10];
SkyboxTexture bbh_skybox_ptrlist;
SkyboxTexture bitdw_skybox_ptrlist;
SkyboxTexture bitfs_skybox_ptrlist;
SkyboxTexture bits_skybox_ptrlist;
SkyboxTexture ccm_skybox_ptrlist;
SkyboxTexture cloud_floor_skybox_ptrlist;
SkyboxTexture clouds_skybox_ptrlist;
SkyboxTexture ssl_skybox_ptrlist;
SkyboxTexture water_skybox_ptrlist;
SkyboxTexture wdw_skybox_ptrlist;

static uint32_t crc32(const void* data, uint32_t size) {
    const uint8_t* msg = data;
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < size; i++) {
        uint8_t byte = msg[i];
        crc = crc ^ byte;
        for (int32_t j = 7; j >= 0; j--) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

static uint8_t* decompress_mio0(uint8_t* buf) {
    int bytes_written = 0;
    int bit_idx = 0;
    int comp_idx = 0;
    int uncomp_idx = 0;
    MIO0_Header head = {};
    head.dest_size = __builtin_bswap32(*(uint32_t*)&buf[4]);
    head.comp_offset = __builtin_bswap32(*(uint32_t*)&buf[8]);
    head.uncomp_offset = __builtin_bswap32(*(uint32_t*)&buf[12]);
    uint8_t* out = malloc(head.dest_size);
    while (bytes_written < head.dest_size) {
        if ((&buf[16])[bit_idx / 8] & (1 << (7 - (bit_idx % 8)))) {
            out[bytes_written] = buf[head.uncomp_offset + uncomp_idx];
            bytes_written++;
            uncomp_idx++;
        }
        else {
            int idx;
            int length;
            int i;
            unsigned char* vals = &buf[head.comp_offset + comp_idx];
            comp_idx += 2;
            length = ((vals[0] & 0xF0) >> 4) + 3;
            idx = ((vals[0] & 0x0F) << 8) + vals[1] + 1;
            for (i = 0; i < length; i++) {
                out[bytes_written] = out[bytes_written - idx];
                bytes_written++;
            }
        }
        bit_idx++;
    }
    return out;
}

static uint8_t* run_mio0(uint64_t offset, uint64_t asset_offset) {
    if (!mio0_cache) mio0_cache = set_init(MIO0_Cache, compare_u64);

    MIO0_Cache* entry = find(mio0_cache, &offset);
    if (entry) return entry->value + asset_offset;

    uint8_t* buf = decompress_mio0(rom + offset);
    push(mio0_cache) = (MIO0_Cache){ .key = offset, .value = buf };

    return buf + asset_offset;
}

static uint32_t bitfs_table[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 8, 9, 10,11,12,13,14,15,
    8, 9, 16,17,18,19,20,21,22,23,16,17,24,25,26,27,28,29,
    30,31,24,25,32,33,34,35,36,37,38,39,32,33,40,41,42,43,
    44,45,46,47,40,41,48,48,48,48,48,48,48,48,48,48,48,48,
    48,48,48,48,48,48,48,48
};

static void extract_skybox(SkyboxTexture* skybox, uint8_t* buffer, uint64_t size, bool use_bitfs_table) {
    uint32_t offset_table[80];
    if (use_bitfs_table) for (int i = 0; i < 80; i++)
        offset_table[i] = bitfs_table[i] * 0x800;
    else {
        uint32_t* table = (uint32_t*)(buffer + (size - 8 * 10 * 4));
        uint32_t base = __builtin_bswap32(table[0]);
        for (int i = 0; i < 80; i++) {
            offset_table[i] = __builtin_bswap32(table[i]) - base;
        }
    }
    for (int i = 0; i < 80; i++) {
        (*skybox)[i] = buffer + offset_table[i];
    }
}

static ROMError assetextract_read_rom(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s for reading: %s\n", filename, strerror(errno));
        return ROM_CANNOT_OPEN;
    }
    fseek(f, 0, SEEK_END);

    // is the file 8 MB?
    if (ftell(f) != sizeof(rom)) {
        fclose(f);
        return ROM_INVALID_FILESIZE;
    }

    fseek(f, 0, SEEK_SET);
    fread(rom, sizeof(rom), 1, f);
    fclose(f);

    return crc32(rom, sizeof(rom)) == CHECKSUM ? ROM_SUCCESS : ROM_CHECKSUM_FAILED;
}

static void assetextract_run() {
    uint64_t start = clock();
    
    if (!assets) {
        assets = set_init(Asset, compare_str);
        assetextract_populate_assets(&assets);
    }

    uint8_t *ctl_buf, *tbl_buf;
    size_t   ctl_len,  tbl_len;
    smart set(Sound_SampleAsset) samples = set_init(Sound_SampleAsset, compare_u32);
    foreach (*asset, assets) {
        printf("extracting %s\n", asset->name);

        switch (asset->type) {
            case AssetType_CTL: ctl_buf = rom + asset->offset, ctl_len = asset->size; break;
            case AssetType_TBL: tbl_buf = rom + asset->offset, tbl_len = asset->size; break;
            case AssetType_Sound: push(samples) = (Sound_SampleAsset){ .name = asset->name, .loc = asset->offset }; break;
            case AssetType_Tiled: extract_skybox(sSkyboxTextures[asset->index], run_mio0(asset->offset, 0), asset->size, strcmp(asset->name, "textures/skyboxes/bitfs") == 0); break;
            case AssetType_MIO0: asset->data = memcpy(malloc(asset->size), run_mio0(asset->offset, asset->mio0_offset), asset->size); break;
            case AssetType_Raw: asset->data = rom + asset->offset; break;
            case AssetType_Demo: asset->data = memcpy(
                (char*)&gDemoInputs + gDemoInputs.entries[asset->index].offset,
                rom + asset->offset,  gDemoInputs.entries[asset->index].size
            ); break;
            case AssetType_Dummy: break;
        }
    }

    Asset* asset_sound_player = find(assets, &(char*){"sound/sequences/00_sound_player.m64"});
    asset_sound_player->data = (uint8_t*)sound_player_sequence_data;
    asset_sound_player->size = sound_player_sequence_data_size;

    printf("reassembling sound\n");
    Asset* asset_ctl = find(assets, &(char*){"sound/sound_data.ctl"});
    Asset* asset_tbl = find(assets, &(char*){"sound/sound_data.tbl"});
    Asset* asset_seq = find(assets, &(char*){"sound/sequences.bin"});
    Asset* asset_bnk = find(assets, &(char*){"sound/bank_sets"});
    sound_reassemble(samples,
        ctl_buf, ctl_len, tbl_buf, tbl_len,
        &asset_ctl->data, &asset_ctl->size,
        &asset_tbl->data, &asset_tbl->size,
        &asset_seq->data, &asset_seq->size,
        &asset_bnk->data, &asset_bnk->size
    );

    uint64_t end = clock();
    printf("done in %g ms\n", (double)(end - start) / CLOCKS_PER_SEC * 1000);
}

bool assetextract_init(const char* rom_dest_path) {
    if (access(rom_dest_path, R_OK) == 0 && assetextract_read_rom(rom_dest_path) == ROM_SUCCESS) goto run;

    if (!tinyfd_messageBox("Pluto",
        "For legal purposes, Pluto needs an SM64 ROM in order to run.\n"
        "Please input an unmodified, US ROM to continue.",
        "okcancel", "info", 0
    )) return false;

    char* file = NULL;
    while (true) {
        file = tinyfd_openFileDialog("SM64 ROM Input", NULL, 1, (const char*[]){"*.z64"}, "SM64 ROM", false);
        if (!file) return false;

        const char* message = NULL;
        switch (assetextract_read_rom(file)) {
            case ROM_SUCCESS: break;
            case ROM_CANNOT_OPEN: message = "Unable to read the ROM."; break;
            case ROM_INVALID_FILESIZE: message = "The ROM needs to be 8192 kB big.\nMake sure you are using a non-extended ROM."; break;
            case ROM_CHECKSUM_FAILED: message = "This is not a valid SM64 ROM.\nIs it in the .z64 format?"; break;
        }

        if (!message) break;
        tinyfd_messageBox("Pluto", message, "ok", "error", 0);
    }

    FILE* f = fopen(rom_dest_path, "wb");
    if (f) {
        fwrite(rom, sizeof(rom), 1, f);
        fclose(f);
    }
    else fprintf(stderr, "could not open %s for writing: %s\n", rom_dest_path, strerror(errno));

    run:
    assetextract_run();
    return true;
}

void* assetextract_get_asset(const char* name, size_t* size) {
    Asset* asset = find(assets, &name);
    if (asset) {
        if (size) *size = asset->size;
        return asset->data;
    }
    return NULL;
}
