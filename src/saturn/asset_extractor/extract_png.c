#include "asset_extractor.h"
#include "data/dynos.h"

#include <stb/stb_image_write.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef _WIN32
#define mkdir(path) mkdir(path, 0744)
#endif

static struct {
    uint32_t w, h;
    const char* asset;
    const char* dest;
} copy_table[] = {
    { 32, 32, "actors/mario/mario_eyes_center.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/eyes_open.rgba32.png" },
    { 32, 32, "actors/mario/mario_eyes_half_closed.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/eyes_half.rgba32.png" },
    { 32, 32, "actors/mario/mario_eyes_closed.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/eyes_closed.rgba32.png" },
    { 32, 32, "actors/mario/mario_eyes_dead.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/eyes_dead.rgba32.png" },
    { 32, 32, "actors/mario/mario_eyes_left_unused.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/saturn_eye_custom.rgba32.png" },
    { 32, 32, "actors/mario/mario_overalls_button.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/saturn_buttons_creator.rgba32.png" },
    { 32, 32, "actors/mario/mario_mustache.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/saturn_mustache_creator.rgba32.png" },
    { 32, 32, "actors/mario/mario_sideburn.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/saturn_sideburn_mario.rgba32.png" },
    { 32, 32, "actors/mario/mario_logo.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/saturn_cap_mario.rgba32.png" },
    { 64, 32, "actors/mario/mario_metal.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/metal.rgba16.png" },
    { 32, 64, "actors/mario/mario_wing.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/wing_1.rgba16.png" },
    { 32, 64, "actors/mario/mario_wing_tip.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/wing_2.rgba16.png" },
    { 32, 64, "actors/mario/mario_wing.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/mario_wing_rgba16_png.rgba16.png" },
    { 32, 64, "actors/mario/mario_wing_tip.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/mario/mario_wing_tip_rgba16_png.rgba16.png" },
    { 32, 32, "actors/mario/mario_overalls_button.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/expressions/buttons/0-default.png" },
    { 32, 32, "actors/mario/mario_logo.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/expressions/cap/mario.png" },
    { 32, 32, "actors/mario/mario_mustache.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/expressions/mustache/0-default.png" },
    { 32, 32, "actors/mario/mario_sideburn.rgba16", DYNOS_PACKS_FOLDER "/Pluto Mario/expressions/sideburn/0-default.png" },
};

static const char* get_format(const char* input) {
    int pos = strlen(input) - 1;
    while (input[pos] != '.') pos--;
    return input + pos + 1;
}

#pragma GCC diagnostic ignored "-Wmissing-braces"

#define RGBA(...) (typeof(rgba_def)){__VA_ARGS__}
typedef union {
    struct { uint8_t start, end; };
    struct { uint8_t value, is_range; };
} Range;

static uint8_t read_bits(uint8_t* data, size_t pos, uint8_t count) {
    uint8_t value = 0;
    size_t end = pos + count;
    for (; pos < end; pos++)
        value = (value << 1) | ((data[pos / 8] >> (7 - pos % 8)) & 1);
    return value;
}

static uint8_t* rgba32(uint8_t* data, size_t size, const char* format) {
    size_t pixel_size;
    union {
        Range rgba[4];
        struct { Range r, g, b, a; };
    } rgba_def;

    if (strcmp(format, "rgba32") == 0) return data;
    else if (strcmp(format, "rgba16") == 0) pixel_size = 16, rgba_def = RGBA(.r = {0, 5}, .g = {5, 10}, .b = {10, 15}, .a = {15, 16});
    else if (strcmp(format, "ia1")    == 0) pixel_size = 1,  rgba_def = RGBA(.r = {0, 1}, .g = {0,  1}, .b = {0,   1}, .a = {0,   1});
    else if (strcmp(format, "ia4")    == 0) pixel_size = 4,  rgba_def = RGBA(.r = {0, 3}, .g = {0,  3}, .b = {0,   3}, .a = {3,   4});
    else if (strcmp(format, "ia8")    == 0) pixel_size = 8,  rgba_def = RGBA(.r = {0, 4}, .g = {0,  4}, .b = {0,   4}, .a = {4,   8});
    else if (strcmp(format, "ia16")   == 0) pixel_size = 16, rgba_def = RGBA(.r = {0, 8}, .g = {0,  8}, .b = {0,   8}, .a = {8,  16});
    else if (strcmp(format, "i4")     == 0) pixel_size = 4,  rgba_def = RGBA(.r = {0, 4}, .g = {0,  4}, .b = {0,   4}, .a = {255});
    else if (strcmp(format, "i8")     == 0) pixel_size = 8,  rgba_def = RGBA(.r = {0, 8}, .g = {0,  8}, .b = {0,   8}, .a = {255});

    size_t num_pixels = (size * 8) / pixel_size;
    size_t pos = 0, bit_pos = 0;
    uint8_t* buf = malloc(num_pixels * 4);
    for (size_t i = 0; i < num_pixels; i++) {
        for (size_t p = 0; p < 4; p++) {
            if (!rgba_def.rgba[p].is_range) {
                buf[pos++] = rgba_def.rgba[p].value;
                continue;
            }
            uint8_t subpixel = rgba_def.rgba[p].end - rgba_def.rgba[p].start;
            uint8_t value = read_bits(data, bit_pos + rgba_def.rgba[p].start, subpixel);
            buf[pos++] = value * 255 / ((1 << subpixel) - 1);
        }
        bit_pos += pixel_size;
    }

    return buf;
}

char* parentdir(char* path) {
    for (int i = strlen(path) - 1; i >= 0; i-- ) {
        if (path[i] == '/') {
            path[i + 1] = 0;
            return path;
        }
    }
    return path;
}

void mkdir_if_not_exist(char* path) {
    struct stat st;
    if (stat(path, &st) == -1)
        mkdir(path);
}

void mkdirs(char* path) {
    int len = strlen(path);
    for (int i = 1; i < len; i++) {
        if (path[i] != '/') continue;

        path[i] = 0;
        mkdir_if_not_exist(path);
        path[i] = '/';
    }
}

void assetextract_write_pngs() {
    for (size_t i = 0; i < sizeof(copy_table) / sizeof(*copy_table); i++) {
        char path[SYS_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", sys_user_path(), copy_table[i].dest);

        size_t size;
        uint8_t* raw = assetextract_get_asset(copy_table[i].asset, &size);
        uint8_t* rgba = rgba32(raw, size, get_format(copy_table[i].asset));
        stbi_write_png(path, copy_table[i].w, copy_table[i].h, 4, rgba, 0);
        free(rgba);
    }
}
